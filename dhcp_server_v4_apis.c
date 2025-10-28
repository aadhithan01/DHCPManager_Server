#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "dhcp_server_v4_apis.h"
#define LOCAL_DHCP_CONF_FILE    "/tmp/dnsmasq.conf.tmp"
#define DHCP_CONF               "/var/dnsmasq.conf"
/*****************************/
//Local functions
/*****************************/
#if !defined(GTEST)
FILE* fopen_wrapper(const char *path, const char *mode)
{
    // You can add any additional logic here before calling fopen
    FILE *file = fopen(path, mode);
    // You can add any additional logic here after calling fopen
    return file;
}
static int executeCmd(char *pCmd)
{
    int iSystemRes;
    fprintf(stdout, "%s:%d, pCmd:%s\n",__FUNCTION__,__LINE__,pCmd);
    iSystemRes = system(pCmd);
    char cLogCmd[BUFF_LEN_256] = {0};
    snprintf(cLogCmd, sizeof(cLogCmd), "echo iSystemRes:%d >> DHCPMgr.log", iSystemRes);
    system(cLogCmd);

    if (0 != iSystemRes && ECHILD != errno)
    {
        snprintf(cLogCmd, sizeof(cLogCmd), "echo error = %d command didn't execute successfully >> DHCPMgr.log", errno);
        system(cLogCmd);
        return iSystemRes;
    }
    snprintf(cLogCmd, sizeof(cLogCmd), "echo error = %d command execute successfully >> DHCPMgr.log", errno);
    system(cLogCmd);
    return 0;
}

static int extractPid(char * pProcessName)
{
    char cProcessDetails [BUFF_LEN_64] = {0};
    FILE* fpPidOfProcess = NULL;
    int iPid;
    fprintf(stdout, "%s:%d, pProcessName:%s\n",__FUNCTION__,__LINE__,pProcessName);
    if (NULL == pProcessName)
    {
        fprintf(stderr, "NULL parameter passed\n");
        return -1;
    }
    snprintf(cProcessDetails, sizeof(cProcessDetails), "pidof %s", pProcessName);
    fpPidOfProcess = popen("pidof dnsmasq", "r");
    if (NULL == fpPidOfProcess)
    {
        fprintf(stderr, "Failed to open the pipe\n");
        return -1;
    }
    if (fscanf(fpPidOfProcess, "%d", &iPid) == EOF)
    {
        iPid = -1; // Process not found
    }
    pclose(fpPidOfProcess);
    return iPid;
}
static bool getProcessInfo(int iPid, char * pCmdLine, int iCmdLineLen)
{
    char cProcPath[BUFF_LEN_256];
    if (NULL == pCmdLine)
    {
        fprintf (stderr, "%s:%d, NULL parameter passed\n",__FUNCTION__,__LINE__);
        return false;
    }
    snprintf(cProcPath, sizeof(cProcPath), "/proc/%d/cmdline", iPid);
    // Open the cmdline file for the process
    FILE *fpCmdLine = fopen(cProcPath, "r");
    if (NULL != fpCmdLine)
    {
        // Read the command line arguments from the file
        int iBytesRead = fread(pCmdLine, 1, iCmdLineLen, fpCmdLine);
        fclose(fpCmdLine);
        if (iBytesRead > 0)
        {
            // Replace '\0' characters with spaces to separate arguments
            for (int iCount = 0; iCount < iBytesRead; iCount++)
            {
                if (pCmdLine[iCount] == '\0')
                {
                    pCmdLine[iCount] = ' ';
                }
            }
            // Print the process details and arguments
            fprintf(stdout, "Process ID: %d\n", iPid);
            return true;
        }
        else
        {
            fprintf(stderr, "Unable to read command line for process %d\n",iPid);
            return false;
        }
    }
    else
    {
        fprintf(stderr, "Process %d not found\n", iPid);
        return false;
    }
}
static bool copyFile(char *pInputFile, char *pOutputFile)
{
    char cLine[BUFF_LEN_256] = {0};
    FILE *fpOutputFile = NULL, *fpInputFile = NULL;
    fprintf(stdout, "%s:%d, pInputFile:%s, pOutputFile:%s\n",__FUNCTION__,__LINE__,pInputFile,pOutputFile);
    fpInputFile = fopen(pInputFile, "r");
    fpOutputFile = fopen(pOutputFile, "w+");
    if ((NULL != fpInputFile) && (NULL != fpOutputFile))
    {
        while(fgets(cLine, sizeof(cLine), fpInputFile) != NULL)
        {
            fputs(cLine, fpOutputFile);
        }
    }
    else
    {
        fprintf(stderr,"%s:%d, copy of files failed due to error in opening one of the files \n",__FUNCTION__,__LINE__);
        return false;
    }
    if(fpInputFile)
        fclose(fpInputFile);
    if(fpOutputFile)
        fclose(fpOutputFile);
    return true;
}
static bool compareFiles(char *pInputFile1, char *pInputFile2)
{
    FILE *fpInput1 = NULL, *fpInput2 = NULL; /* File Pointer Read, File Pointer Read */
    char *pRetFile1 = NULL, *pRetFile2 = NULL;
    char cFileBuff1[BUFF_LEN_512];
    char cFileBuff2[BUFF_LEN_512];
    int iCmpRes, iLineNum = 0;
    fprintf(stdout, "%s:%d, pInputFile1:%s, pInputFile2:%s\n",__FUNCTION__,__LINE__,pInputFile1,pInputFile2);
    fpInput1 = fopen(pInputFile1, "r");
    if (fpInput1 == NULL)
    {
        fprintf(stderr,"%s:%d, Can't open %s for reading\n", __FUNCTION__,__LINE__,pInputFile1);
        return false;
    }
    fpInput2 = fopen(pInputFile2, "r");
    if (fpInput2 == NULL)
    {
        fclose(fpInput1);
        fprintf(stderr,"%s:%d, Can't open %s for reading\n", __FUNCTION__,__LINE__,pInputFile2);
        return false;
    }
    pRetFile1 = fgets(cFileBuff1, sizeof(cFileBuff1), fpInput1);
    pRetFile2 = fgets(cFileBuff2, sizeof(cFileBuff2), fpInput2);
    while (pRetFile1 != NULL || pRetFile2 != NULL)
    {
        ++iLineNum;
        iCmpRes = strcmp(cFileBuff1, cFileBuff2);
        if (iCmpRes != 0)
        {
            fclose(fpInput1);
            fclose(fpInput2);
            return false;
        }
        pRetFile1 = fgets(cFileBuff1, sizeof(cFileBuff1), fpInput1);
        pRetFile2 = fgets(cFileBuff2, sizeof(cFileBuff2), fpInput2);
    }
    fclose(fpInput1);
    fclose(fpInput2);
    return true;
}
#endif // !defined(GTEST)
bool dhcpServerInit(const GlobalDhcpConfig *pGlobalConfig, DhcpInterfaceConfig **ppDhcpConfigs, int iNumOfInterfaces)
{
    FILE * fpLocalDhcpConf = NULL;
    char cLocalDhcpCfgFile [BUFF_LEN_64] = {0};
    int iCounter;
    snprintf(cLocalDhcpCfgFile,sizeof(cLocalDhcpCfgFile),"%s",LOCAL_DHCP_CONF_FILE);
    fpLocalDhcpConf = fopen_wrapper (cLocalDhcpCfgFile, "w+");
    if (NULL == fpLocalDhcpConf)
    {
        fprintf(stderr,"%s:%d, Failed to open the file\n",__FUNCTION__,__LINE__);
        system("echo Failed to open the local DHCP config file >> DHCPMgr.log");
        return false;
    }
    if (((0 < iNumOfInterfaces) && ((NULL == ppDhcpConfigs) || (NULL == *ppDhcpConfigs))) || (NULL == pGlobalConfig))
    {
        fprintf(stderr, "%s:%d, NULL parameter passed\n",__FUNCTION__,__LINE__);
        fprintf(stderr, "iNumOfInterfaces:%d\n",iNumOfInterfaces);
        system("echo NULL parameter passed >> DHCPMgr.log");
        fclose(fpLocalDhcpConf);
        return false;
    }
    if(true == pGlobalConfig->bDomainNeeded)
    {
        fprintf(fpLocalDhcpConf, "domain-needed\n");
        system("echo domain-needed enabled >> DHCPMgr.log");
    }
    if (true == pGlobalConfig->bBogusPriv)
    {
        fprintf(fpLocalDhcpConf, "bogus-priv\n");
        system("echo bogus-priv enabled >> DHCPMgr.log");
    }
    if ((true == pGlobalConfig->sDomainSpecific.bIsDomainSpecificEnabled) && (0 < pGlobalConfig->sDomainSpecific.iDomainSpecificAddressCount) && (NULL != pGlobalConfig->sDomainSpecific.ppDomainSpecificAddresses))
    {
        system("echo Processing domain-specific addresses >> DHCPMgr.log");
        for (iCounter=0; iCounter < pGlobalConfig->sDomainSpecific.iDomainSpecificAddressCount; iCounter++)
        {
            if (NULL != pGlobalConfig->sDomainSpecific.ppDomainSpecificAddresses[iCounter])
            {
                fprintf (fpLocalDhcpConf, "address=%s\n",pGlobalConfig->sDomainSpecific.ppDomainSpecificAddresses[iCounter]);
                system("echo domain-specific address set >> DHCPMgr.log");
            }
        }
    }
    // Hardcoded DHCP configurations
    fprintf(fpLocalDhcpConf, "dhcp-leasefile=/home/aadhithan/Desktop/aadhi/DHCP_MGR/dnsmasq.leases\n");
    fprintf(fpLocalDhcpConf, "dhcp-hostsfile=/etc/dhcp_static_hosts\n");
    fprintf(fpLocalDhcpConf, "dhcp-optsfile=/var/dhcp_options\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:Plume,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:PP203X,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:HIXE12AWR,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:WNXE12AWR,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:SE401,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:WNXL11BWL,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=vendor:RDKBPOD,43,tag=123\n");
    fprintf(fpLocalDhcpConf, "dhcp-vendorclass=set:extender,WNXL11BWL\n");
    fprintf(fpLocalDhcpConf, "dhcp-option=tag:extender, option:time-offset,-18000\n");
    fprintf(fpLocalDhcpConf, "dhcp-ignore=tag:block_selfWAN\n");
    fprintf(fpLocalDhcpConf, "dhcp-vendorclass=set:block_selfWAN,eRouter1.0\n");
    if ('\0' != pGlobalConfig->cResolvCfgFile[0])
    {
        fprintf(fpLocalDhcpConf, "resolv-file=%s\n",pGlobalConfig->cResolvCfgFile);
        system("echo resolv-file set >> DHCPMgr.log");
    }
    else
    {
        fprintf(fpLocalDhcpConf, "no-resolv\n");
        system("echo no-resolv set >> DHCPMgr.log");
    }
    if ('\0' != pGlobalConfig->cDomain[0])
    {
        fprintf(fpLocalDhcpConf, "domain=%s\n",pGlobalConfig->cDomain);
        system("echo domain set >> DHCPMgr.log");
    }
    if (true == pGlobalConfig->bExpandHosts)
    {
        fprintf(fpLocalDhcpConf, "expand-hosts\n");
        system("echo expand-hosts enabled >> DHCPMgr.log");
    }
    if ('\0' != pGlobalConfig->cDhcpLeaseFile[0])
    {
        fprintf(fpLocalDhcpConf, "dhcp-leasefile=%s\n",pGlobalConfig->cDhcpLeaseFile);
        system("echo dhcp-leasefile set >> DHCPMgr.log");
    }
    if ('\0' != pGlobalConfig->cDhcpHostsFile[0])
    {
        fprintf(fpLocalDhcpConf, "dhcp-hostsfile=%s\n",pGlobalConfig->cDhcpHostsFile);
        system("echo dhcp-hostsfile set >> DHCPMgr.log");
    }
    if ('\0' != pGlobalConfig->cDhcpOptsFile[0])
    {
        fprintf(fpLocalDhcpConf, "dhcp-optsfile=%s\n", pGlobalConfig->cDhcpOptsFile);
        system("echo dhcp-optsfile set >> DHCPMgr.log");
    }
    if ((0 < pGlobalConfig->sDhcpOptions.iVendorIdCount) && (NULL != pGlobalConfig->sDhcpOptions.ppVendorId))
    {
        system("echo Processing vendor IDs >> DHCPMgr.log");
        for (iCounter=0; iCounter < pGlobalConfig->sDhcpOptions.iVendorIdCount; iCounter++)
        {
            if (NULL != pGlobalConfig->sDhcpOptions.ppVendorId[iCounter])
            {
                fprintf (fpLocalDhcpConf, "dhcp-option=%s\n",pGlobalConfig->sDhcpOptions.ppVendorId[iCounter]);
                system("echo dhcp-option set >> DHCPMgr.log");
            }
        }
    }
    if ((0 < pGlobalConfig->sDhcpOptions.iVendorClassCount) && (NULL != pGlobalConfig->sDhcpOptions.ppVendorClass))
    {
        system("echo Processing vendor classes >> DHCPMgr.log");
        for (iCounter=0; iCounter < pGlobalConfig->sDhcpOptions.iVendorClassCount; iCounter++)
        {
            if (NULL != pGlobalConfig->sDhcpOptions.ppVendorClass[iCounter])
            {
                fprintf (fpLocalDhcpConf, "dhcp-vendorclass=%s\n",pGlobalConfig->sDhcpOptions.ppVendorClass[iCounter]);
                system("echo dhcp-vendorclass set >> DHCPMgr.log");
            }
        }
    }
    for (iCounter = 0; iCounter < iNumOfInterfaces; iCounter++)
    {
        system("echo Processing DHCP interface >> DHCPMgr.log");
        DhcpInterfaceConfig *pDhcpConfigs = ppDhcpConfigs[iCounter];
        if (('\0' != pDhcpConfigs->cInputStr[0]) && (!strncmp(pDhcpConfigs->cInputStr, "dns_only", 8)))
        {
            fprintf(fpLocalDhcpConf, "no-dhcp-interface=%s\n",pDhcpConfigs->cGatewayName);
            system("echo no-dhcp-interface set >> DHCPMgr.log");
        }
        if (true == pDhcpConfigs->bIsDhcpEnabled)
        {
            if ('\0' != pDhcpConfigs->cGatewayName[0])
            {
                fprintf (fpLocalDhcpConf, "interface=%s\n", pDhcpConfigs->cGatewayName);
                system("echo interface set >> DHCPMgr.log");
            }
            fprintf(fpLocalDhcpConf, "dhcp-range=");
            if (0 != pDhcpConfigs->sAddressPool.iDhcpTagNum)
            {
                fprintf (fpLocalDhcpConf, "set:%d,", pDhcpConfigs->sAddressPool.iDhcpTagNum);
                system("echo dhcp-range tag set >> DHCPMgr.log");
            }
            if ('\0' != pDhcpConfigs->sAddressPool.cStartAddress[0])
            {
                fprintf (fpLocalDhcpConf, "%s,", pDhcpConfigs->sAddressPool.cStartAddress);
                system("echo dhcp-range start address set >> DHCPMgr.log");
            }
            if ('\0' != pDhcpConfigs->sAddressPool.cEndAddress[0])
            {
                fprintf (fpLocalDhcpConf, "%s,", pDhcpConfigs->sAddressPool.cEndAddress);
                system("echo dhcp-range end address set >> DHCPMgr.log");
            }
            if ('\0' != pDhcpConfigs->cSubnetMask[0])
            {
                fprintf (fpLocalDhcpConf, "%s,", pDhcpConfigs->cSubnetMask);
                system("echo dhcp-range subnet mask set >> DHCPMgr.log");
            }
            if ('\0' != pDhcpConfigs->cLeaseDuration[0])
            {
                fprintf (fpLocalDhcpConf, "%s\n", pDhcpConfigs->cLeaseDuration);
                system("echo dhcp-range lease duration set >> DHCPMgr.log");
            }
        }
        if ((true == pDhcpConfigs->sDhcpNameServer.bDhcpNameServerEnabled) && ('\0' != pDhcpConfigs->sDhcpNameServer.cDhcpNameServerIp[0]))
        {
            fprintf(fpLocalDhcpConf, "%s\n",pDhcpConfigs->sDhcpNameServer.cDhcpNameServerIp);
            system("echo dhcp-name-server set >> DHCPMgr.log");
        }
    }

    if ((true == pGlobalConfig->sRedirectInfo.bIsRedirectionEnabled) && (0 < pGlobalConfig->sRedirectInfo.iRedirectionUrlCount) && (NULL != pGlobalConfig->sRedirectInfo.ppRedirectionUrl))
    {
        system("echo Processing redirection URLs >> DHCPMgr.log");
        for (iCounter = 0; iCounter < pGlobalConfig->sRedirectInfo.iRedirectionUrlCount; iCounter++)
        {
            if(NULL != pGlobalConfig->sRedirectInfo.ppRedirectionUrl[iCounter])
            {
                fprintf (fpLocalDhcpConf, "server=%s\n", pGlobalConfig->sRedirectInfo.ppRedirectionUrl[iCounter]);
                system("echo redirection server set >> DHCPMgr.log");
            }
        }
    }
    fclose(fpLocalDhcpConf);
    system("echo dhcpServerInit completed successfully >> DHCPMgr.log");
    return true;
}
bool dhcpServerStart(const GlobalDhcpConfig *pGlobalConfig)
{
    //DEBUG
    //using system call , i want to print and redirect that output of pGlbDhcpCfg->sCmdArgs.iNumOfArgs  to /rdklogs/logs/DHCPMgr.log
    char cSystemCmd [BUFF_LEN_256]={0};
    char cSystemCmd1 [1024]={0};
    snprintf(cSystemCmd1,sizeof(cSystemCmd1),"echo %d > DHCPMgr.log",pGlobalConfig->sCmdArgs.iNumOfArgs);
    system(cSystemCmd1);
 //using system call , i want to print and redirect that output of pGlbDhcpCfg->sCmdArgs.  ppCmdLineArgs[]   to /rdklogs/logs/DHCPMgr.log
    for (int i = 0; i < pGlobalConfig->sCmdArgs.iNumOfArgs; i++)
    {
        snprintf(cSystemCmd1,sizeof(cSystemCmd1),"echo %s >> DHCPMgr.log",pGlobalConfig->sCmdArgs.ppCmdLineArgs[i]);
        system(cSystemCmd1);
    }
    //DEBUG
        fprintf(stdout, "%s:%d, Entering dhcpServerStart\n",__FUNCTION__,__LINE__);
        system("echo Entering dhcpServerStart >> DHCPMgr.log");
        bool bFileDiff = false;
        bool bRestartDhcpv4 = false;
        int  iStrLen = 0;
        int iPidOfDnsmasq = -1;
        if (NULL == pGlobalConfig)
        {
            fprintf(stderr,"%s:%d, NULL parameter passed\n",__FUNCTION__,__LINE__);
            system("echo NULL parameter passed >> DHCPMgr.log");
            return false;
        }
        iPidOfDnsmasq = extractPid("pidof dnsmasq");
        bFileDiff = compareFiles(DHCP_CONF, LOCAL_DHCP_CONF_FILE);
        if (false == bFileDiff)
        {
            fprintf(stdout,"%s:%d, Files are not identical restart dnsmasq \n",__FUNCTION__,__LINE__);
            system("echo Files are not identical, restarting dnsmasq >> DHCPMgr.log");
            bRestartDhcpv4 = true;
            if (false == copyFile(LOCAL_DHCP_CONF_FILE, DHCP_CONF))
            {
                fprintf(stderr,"%s:%d, Failed to copy the file\n",__FUNCTION__,__LINE__);
                system("echo Failed to copy the file >> DHCPMgr.log");
                return false;
            }
            if(0 != remove(LOCAL_DHCP_CONF_FILE))
            {
                fprintf(stderr,"%s:%d, remove of %s file is not successful error is:%d\n",__FUNCTION__,__LINE__, LOCAL_DHCP_CONF_FILE, errno);
                system("echo Failed to remove the temporary file >> DHCPMgr.log");
                return false;
            }
            if ( -1 < iPidOfDnsmasq)
            {
                if (0 != executeCmd("sudo killall dnsmasq"))
                {
                    fprintf(stderr,"%s:%d, Failed to kill the dnsmasq process\n",__FUNCTION__,__LINE__);
                    system("echo Failed to kill the dnsmasq process >> DHCPMgr.log");
                    return false;
                }
            }
        }
        if(false == bRestartDhcpv4)
        {
            fprintf(stdout, "%s:%d, Dnsmasq process is running with PID:%d\n",__FUNCTION__,__LINE__,iPidOfDnsmasq);
            system("echo Dnsmasq process is running >> DHCPMgr.log");
            if (-1 == iPidOfDnsmasq)
            {
                fprintf (stdout, "%s:%d Dnsmasq process is not running, Start it\n",__FUNCTION__,__LINE__);
                system("echo Dnsmasq process is not running, starting it >> DHCPMgr.log");
                bRestartDhcpv4 = true;
            }
        }
        if(false == bRestartDhcpv4)
        {
            fprintf(stdout, "%s:%d, No need to restart the dnsmasq process\n",__FUNCTION__,__LINE__);
            system("echo No need to restart the dnsmasq process >> DHCPMgr.log");
            return bRestartDhcpv4;
        }
        snprintf(cSystemCmd,sizeof(cSystemCmd),"sudo dnsmasq ");
        iStrLen=strlen(cSystemCmd);
        fprintf(stdout, "%s:%d,  iNumOfArgs:%d\n",__FUNCTION__,__LINE__,pGlobalConfig->sCmdArgs.iNumOfArgs);
        system("echo Preparing to start dnsmasq >> DHCPMgr.log");
        if ((0 < pGlobalConfig->sCmdArgs.iNumOfArgs) && (NULL != pGlobalConfig->sCmdArgs.ppCmdLineArgs))
        {
            fprintf(stdout, "%s:%d, Adding the cmd line args\n",__FUNCTION__,__LINE__);
            system("echo Adding command line arguments >> DHCPMgr.log");
            int iCounter;
            char *pArg = NULL;
            for (iCounter = 0; iCounter < pGlobalConfig->sCmdArgs.iNumOfArgs; iCounter++)
            {
                pArg = pGlobalConfig->sCmdArgs.ppCmdLineArgs[iCounter];
                if (NULL != pArg)
                {
                    fprintf(stdout, "%s:%d, Adding CmdLine Arg[%d] = %s\n",__FUNCTION__,__LINE__,iCounter,pArg);
                    snprintf (cSystemCmd+iStrLen,sizeof(cSystemCmd)-iStrLen,"%s ", pArg);
                    iStrLen=strlen(cSystemCmd);
                }
            }
        }
        fprintf(stdout, "DnsmasqCommand:%s\n",cSystemCmd);

        snprintf(cSystemCmd1, sizeof(cSystemCmd1),"echo \"Starting dnsmasq process %s\" >> DHCPMgr.log", cSystemCmd);
        system(cSystemCmd1);

        if (0 != executeCmd (cSystemCmd))
        {
            fprintf(stderr,"%s:%d, Failed to start the dnsmasq process\n",__FUNCTION__,__LINE__);
            system("echo Failed to start the dnsmasq process >> DHCPMgr.log");
            return false;
        }
        system("echo Dnsmasq_process started successfully >> DHCPMgr.log");
        return bRestartDhcpv4;
}
bool dhcpServerStop(char **ppDhcpInterfaces, int iNumOfInterfaces)
{
    fprintf(stdout, "%s:%d, Entering dhcpServerStop\n",__FUNCTION__,__LINE__);
    FILE * fpLocalDhcpConf = NULL;
    FILE * fpTmpDhcpConf = NULL;
    char cTmpDhcpCfgFile [BUFF_LEN_64] = {0};
    char cLine [BUFF_LEN_256] = {0};
    char cCmdLineArg [BUFF_LEN_2048] = {0};
    int iPidOfDnsmasq = 0;
    int iStrLen = 0;

    if ((0 < iNumOfInterfaces) && ((NULL == ppDhcpInterfaces) || (NULL == *ppDhcpInterfaces)))
    {
        fprintf(stdout, "%s:%d, NULL parameter passed\n", __FUNCTION__, __LINE__);
        fprintf(stderr, "%s:%d, NULL parameter passed\n", __FUNCTION__, __LINE__);
        return false;
    }

    fpLocalDhcpConf = fopen_wrapper(DHCP_CONF, "r+");
    if (NULL == fpLocalDhcpConf)
    {
        fprintf(stdout, "%s:%d, Failed to open the file: %s\n", __FUNCTION__, __LINE__, DHCP_CONF);
        fprintf(stderr, "Failed to open the file :%s\n", DHCP_CONF);
        return false;
    }

    snprintf(cTmpDhcpCfgFile, sizeof(cTmpDhcpCfgFile), "%s%d", LOCAL_DHCP_CONF_FILE, getpid());
    fpTmpDhcpConf = fopen_wrapper(cTmpDhcpCfgFile, "w+");
    if (NULL == fpTmpDhcpConf)
    {
        fprintf(stdout, "%s:%d, Failed to open the temporary file\n", __FUNCTION__, __LINE__);
        fprintf(stderr, "%s:%d, Failed to open the file\n", __FUNCTION__, __LINE__);
        fclose(fpLocalDhcpConf);
        return false;
    }

    iPidOfDnsmasq = extractPid("pidof dnsmasq");
    if (-1 != iPidOfDnsmasq)
    {
        if (false == getProcessInfo(iPidOfDnsmasq, cCmdLineArg, sizeof(cCmdLineArg)))
        {
            fprintf(stdout, "%s:%d, Failed to get the process info\n", __FUNCTION__, __LINE__);
            fprintf(stderr, "%s:%d, Failed to get the process info\n", __FUNCTION__, __LINE__);
        }
    }

    bool bFound = false;
    bool bInterfaceFound = false;
    iStrLen = strlen("interface=");
    while (NULL != (fgets(cLine, sizeof(cLine), fpLocalDhcpConf)))
    {
        fprintf(stdout, "%s:%d, Reading line: %s\n", __FUNCTION__, __LINE__, cLine);
        bFound = false;

        if (strncmp(cLine, "interface=", iStrLen))
        {
            fprintf(stdout, "%s:%d, Line does not start with 'interface=', writing to temporary file\n", __FUNCTION__, __LINE__);
            fprintf(fpTmpDhcpConf, "%s", cLine);
        }
        else
        {
            fprintf(stdout, "%s:%d, Line starts with 'interface= and line is %s'\n", __FUNCTION__, __LINE__, cLine);
            if ((0 < iNumOfInterfaces) && (NULL != ppDhcpInterfaces))
            {
                for (int iCount = 0; iCount < iNumOfInterfaces; iCount++)
                {
                    fprintf(stdout, "%s:%d, Checking interface: %s\n", __FUNCTION__, __LINE__, ppDhcpInterfaces[iCount]);
                    char cIfName[BUFF_LEN_32] = {0};
                    strncpy(cIfName, cLine + iStrLen, strlen(cLine + iStrLen) - 1);
                    if ((NULL != ppDhcpInterfaces[iCount]) && (!strcmp(cIfName, ppDhcpInterfaces[iCount])))
                    {
                        fprintf(stdout, "%s:%d, Found matching interface: %s\n", __FUNCTION__, __LINE__, cIfName);
                        memset(cLine, 0, sizeof(cLine));
                        fgets(cLine, sizeof(cLine), fpLocalDhcpConf);
                        if ((!strncmp(cLine, "dhcp-range=set:", strlen("dhcp-range=set:")) || (!strncmp(cLine, "dhcp-range=", strlen("dhcp-range=")))))
                        {
                            bFound = true;
                            bInterfaceFound = true;
                            fprintf(stdout, "%s:%d, Found dhcp-range for interface\n", __FUNCTION__, __LINE__);
                            break;
                        }
                        else
                        {
                            fprintf(stdout, "%s:%d, Writing non-dhcp-range line to temporary file\n", __FUNCTION__, __LINE__);
                            fprintf(fpTmpDhcpConf, "%s", cLine);
                        }
                    }
                }
                if (false == bFound)
                {
                    fprintf(stdout, "%s:%d, No matching interface found, writing line to temporary file\n", __FUNCTION__, __LINE__);
                    fprintf(fpTmpDhcpConf, "%s", cLine);
                }
            }
            else
            {
                fprintf(stdout, "%s:%d, No interfaces provided, processing line\n", __FUNCTION__, __LINE__);
                memset(cLine, 0, sizeof(cLine));
                fgets(cLine, sizeof(cLine), fpLocalDhcpConf);
                if (0 < strlen(cLine))
                {
                    if ((strncmp(cLine, "dhcp-range=set:", strlen("dhcp-range=set:")) && (strncmp(cLine, "dhcp-range=", strlen("dhcp-range=")))))
                    {
                        fprintf(stdout, "%s:%d, Writing non-dhcp-range line to temporary file\n", __FUNCTION__, __LINE__);
                        fprintf(fpTmpDhcpConf, "%s", cLine);
                    }
                    else
                    {
                        bInterfaceFound = true;
                        fprintf(stdout, "%s:%d, Found dhcp-range line\n", __FUNCTION__, __LINE__);
                    }
                }
            }
        }
    }

    fclose(fpLocalDhcpConf);
    fclose(fpTmpDhcpConf);

    if ((true == bInterfaceFound) && (((0 >= iNumOfInterfaces) || (NULL == ppDhcpInterfaces)) || (0 >= strlen(cCmdLineArg))))
    {
        snprintf(cCmdLineArg, sizeof(cCmdLineArg), "sudo dnsmasq -P 4096 -C %s ", DHCP_CONF);
        fprintf(stdout, "%s:%d, Command line argument prepared: %s\n", __FUNCTION__, __LINE__, cCmdLineArg);
    }

    if ((0 < strlen(cCmdLineArg)) && (true == bInterfaceFound))
    {
        fprintf(stdout, "%s:%d, Restarting dnsmasq process\n", __FUNCTION__, __LINE__);
        if ((-1 < iPidOfDnsmasq) && (0 != executeCmd("sudo killall dnsmasq")))
        {
            fprintf(stdout, "%s:%d, Failed to kill the dnsmasq process\n", __FUNCTION__, __LINE__);
            fprintf(stderr, "%s:%d, Failed to kill the dnsmasq process\n", __FUNCTION__, __LINE__);
            return false;
        }
        //STUB to make dnsmasq use the new config file
/*        if (false == copyFile(cTmpDhcpCfgFile, DHCP_CONF))
        {
            fprintf(stdout, "%s:%d, Failed to copy the file\n", __FUNCTION__, __LINE__);
            fprintf(stderr, "%s:%d, Failed to copy the file\n", __FUNCTION__, __LINE__);
            return false;
        } */
        if (0 != remove(cTmpDhcpCfgFile))
        {
            fprintf(stdout, "%s:%d, Failed to remove the temporary file: %s\n", __FUNCTION__, __LINE__, cTmpDhcpCfgFile);
            fprintf(stderr, "%s:%d, remove of %s file is not successful error is:%d\n", __FUNCTION__, __LINE__, cTmpDhcpCfgFile, errno);
            return false;
        }
        if (0 != executeCmd(cCmdLineArg))
        {
            fprintf(stdout, "%s:%d, Failed to start the dnsmasq process\n", __FUNCTION__, __LINE__);
            fprintf(stderr, "%s:%d, Failed to start the dnsmasq process\n", __FUNCTION__, __LINE__);
            return false;
        }
    }
    else
    {
        fprintf(stdout, "%s:%d, No command line arguments to start the dnsmasq process\n", __FUNCTION__, __LINE__);
        fprintf(stderr, "%s:%d, No command line arguments to start the dnsmasq process\n", __FUNCTION__, __LINE__);
        return false;
    }

    fprintf(stdout, "%s:%d, dhcpServerStop completed successfully\n", __FUNCTION__, __LINE__);
    return true;
}
bool isDhcpConfHasInterface(void)
{
    FILE *fpDhcpConf = NULL;
    char cLine [BUFF_LEN_256] = {0};
    bool bInterfaceFound = false;
    int iStrLen = strlen("interface=");
    fpDhcpConf = fopen_wrapper(DHCP_CONF,"r");
    if (NULL == fpDhcpConf )
    {
        fprintf (stderr, "%s:%d, Failed to open the %s file\n",__FUNCTION__,__LINE__, DHCP_CONF);
        return false;
    }
    memset(cLine,0,sizeof(cLine));
    while (NULL != (fgets(cLine, sizeof(cLine), fpDhcpConf)))
    {
        if ((!strncmp(cLine, "interface=", iStrLen)) && ((int)strlen(cLine) > iStrLen))
        {
            fprintf(stdout, "Interface found in the %s file\n", DHCP_CONF);
            bInterfaceFound = true;
            break;
        }
    }
    fclose(fpDhcpConf);
    if (false == bInterfaceFound)
        fprintf(stdout, "Interfaces NOT found in the %s file\n", DHCP_CONF);
    return bInterfaceFound;
}