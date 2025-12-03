#include<stdio.h>
#include<limits.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include "dhcp_server_v6_apis.h"

#if ENABLE_TEST
    #define RA_CONF_FILE              "ra.conf"
    #define SERVER_CONF_LOCATION      "server.conf"
#else
    #define RA_CONF_FILE              "/var/ra.conf"
    #define SERVER_CONF_LOCATION      "/etc/dibbler/server.conf"
#endif

//Global declaration
//To write header only once in RA config file
static int raConfigHeader = 0;

struct DHCPV6_TAG
{
    int tag;
    char * cmdstring;
};

struct DHCPV6_TAG tagList[] =
{
    {17, "vendor-spec"},
    {21, "sip-domain"},
    {22, "sip-server"},
    {23, "dns-server"},
    {24, "domain"},
    {27, "nis-server"},
    {28, "nis+-server"},
    {29, "nis-domain"},
    {30, "nis+-domain"},
    {31, "ntp-server"},
    {42, "time-zone"}
};

int executeCmd(char *cmd)
{
    int result;
    result = system(cmd);
    if (0 != result && ECHILD != errno)
    {
        return result;
    }
    return 0;
}

int stopRAService(char *server)
{
    // Find the process ID (PID) of the ra process
    FILE *fp;
    char command[256];
    char output[128];
    int pid;

    // Construct the command to get the PID using pgrep
    snprintf(command, sizeof(command), "pgrep -x %s", server);

    // Open a pipe to run the command and capture its output
    fp = popen(command, "r");
    if (fp == NULL) {
        return 1;
    }

    // Read the output of the command
    if (fgets(output, sizeof(output), fp) == NULL) {
        // No ra process found
        pclose(fp);
        return 1;
    }

    // Close the pipe
    pclose(fp);

    // Convert the output string to an integer (PID)
    pid = atoi(output);

    if (pid <= 0) {
        // Invalid PID
        return 1;
    }

    // Send SIGTERM signal to stop the ra process
    if (kill(pid, SIGTERM) == -1) {
        return 1;
    }
    return 0;
}

// Function to add DHCPv6 options to server.conf
void addOptions(FILE *fp, Dhcpv6Option *options, size_t num_options) {
    if (fp == NULL || options == NULL) {
        return;
    }
 
    if( num_options > 0 )
    {
        // Iterate through the options
        for (size_t i = 0; i < num_options; ++i)
        {
            char *cmdstring = NULL;
            for (size_t j = 0; j < sizeof(tagList) / sizeof(tagList[0]); ++j) {
                if (tagList[j].tag == options[i].option_code) {
                   cmdstring = tagList[j].cmdstring;
                   break;
                }
            }
            if (cmdstring) {
                 fprintf(fp, "   option %s %s\n", cmdstring,options[i].option_data);
            }
        }
    }
}

int createRAConfig(FILE *rAfp,const IPv6Dhcpv6InterfaceConfig *interfaceConfig)
{
    if (!rAfp)
    {
        return -1;
    }

    if (interfaceConfig == NULL)
    {
        return -1;
    }

    static const char *ra_conf_base = \
        "!enable password admin\n"
        "!log stdout\n"
        "log file /var/tmp/zebra.log errors\n"
        "table 255\n";

    //Write header only once in RA config file
    if(!raConfigHeader)
    {
        if (fwrite(ra_conf_base, strlen(ra_conf_base), 1, rAfp) != 1)
        {
            fclose(rAfp);
            return -1;
	}
	raConfigHeader = 1;
    }

    fprintf(rAfp, "interface %s\n", interfaceConfig->interface_name);

    fprintf(rAfp, "   no ipv6 nd suppress-ra\n");

    // Write each prefix
    if(interfaceConfig->ra_params.num_prefix != 0)
    {
        for (size_t prefixIndex = 0; prefixIndex < interfaceConfig->ra_params.num_prefix; ++prefixIndex)
        {
            //write to file if prefix is not empty
            if((strlen(interfaceConfig->ra_params.raPrefix[prefixIndex].prefix) > 0)
                && (interfaceConfig->ra_params.raPrefix[prefixIndex].prefix[0] != '\0'))
            {
                fprintf(rAfp, "   ipv6 nd prefix %s %u %u\n",
                    interfaceConfig->ra_params.raPrefix[prefixIndex].prefix,
                    interfaceConfig->ra_params.raPrefix[prefixIndex].ra_valid_lft, interfaceConfig->ra_params.raPrefix[prefixIndex].ra_preferred_lft);
            }
        }
    }

    fprintf(rAfp, "   ipv6 nd ra-interval %d\n", interfaceConfig->ra_params.ra_interval);
    fprintf(rAfp, "   ipv6 nd ra-lifetime %d\n", interfaceConfig->ra_params.ra_router_lifetime);

    if (interfaceConfig->ra_params.ra_managed_flag)
        fprintf(rAfp, "   ipv6 nd managed-config-flag\n");
    else
        fprintf(rAfp, "   no ipv6 nd managed-config-flag\n");

    if (interfaceConfig->ra_params.ra_other_flag)
        fprintf(rAfp, "   ipv6 nd other-config-flag\n");
    else
        fprintf(rAfp, "   no ipv6 nd other-config-flag\n");

    fprintf(rAfp, "   ipv6 nd router-preference medium\n");
    
    if(interfaceConfig->ra_params.sBoost != NULL)
    {
        if(interfaceConfig->ra_params.sBoost->adv_pvd_enable == 1)
        {
            fprintf(rAfp, "   ipv6 nd pvd_enable\n");
            if (interfaceConfig->ra_params.sBoost->adv_pvd_hflag == 1)
            {
                fprintf(rAfp, "   ipv6 nd pvd_hflag_enable\n");

                if(interfaceConfig->ra_params.sBoost->adv_pvd_delay != 0)
                    fprintf(rAfp, "   ipv6 nd pvd_delay %d\n", interfaceConfig->ra_params.sBoost->adv_pvd_delay);

                if(interfaceConfig->ra_params.sBoost->adv_pvd_seqNum != 0)
                    fprintf(rAfp, "   ipv6 nd pvd_seq_num %d\n", interfaceConfig->ra_params.sBoost->adv_pvd_seqNum);
            }
            else
            {
                fprintf(rAfp, "   ipv6 nd pvd_delay 0\n");
                fprintf(rAfp, "   ipv6 nd pvd_seq_num 0\n");
            }

            if(interfaceConfig->ra_params.sBoost->adv_pvd_fqdn[0] != '\0')
            {
                fprintf(rAfp, "   ipv6 nd pvd_fqdn %s\n", interfaceConfig->ra_params.sBoost->adv_pvd_fqdn);
            }
        }
    }

    // RDNSS entries
    if(interfaceConfig->ra_params.num_rdnss != 0)
    {
        for (size_t i = 0; i < interfaceConfig->ra_params.num_rdnss; ++i)
        {
            fprintf(rAfp, "   ipv6 nd rdnss %s %u\n", interfaceConfig->ra_params.rdnss[i].ipv6_rdns_server,
                interfaceConfig->ra_params.rdnss[i].lifetime);
        }
    }

    fprintf(rAfp, "interface %s\n", interfaceConfig->interface_name);
    fprintf(rAfp, "   ip irdp multicast\n");
    return 0;
}

int dhcpv6_server_create(const IPv6Dhcpv6ServerConfig *config, server_type_t server_type)
{
    FILE *rAfp = NULL;
    FILE *fp = NULL;
    if(server_type == DHCPV6_SERVER_TYPE_STATELESS)
    {
        rAfp = fopen(RA_CONF_FILE, "wb");
        if (!rAfp)
        {
            return -1;
        }
    }

    if(server_type == DHCPV6_SERVER_TYPE_STATEFUL)
    {
        fp = fopen(SERVER_CONF_LOCATION, "w+");

        if (!fp)
        {
            return -1;
        }
    }
 
    if (config == NULL)
    {
        return -1;
    }
    for (size_t i = 0; i < config->num_interfaces; i++)
    {
        // If the server type is not stateful, then skip the interface
        if((config->interfaces[i].server_type != DHCPV6_SERVER_TYPE_STATEFUL)
            && (config->interfaces[i].server_type != DHCPV6_SERVER_TYPE_STATELESS))
        {
            break;
        }
        if (config->interfaces[i].enable_dhcp == true && config->interfaces[i].server_type == DHCPV6_SERVER_TYPE_STATEFUL)
        {
            fprintf(fp, "log-level %d\n", config->interfaces[i].log_level);

            fprintf(fp, "inactive-mode\n");

            // strict RFC compliance rfc3315 Section 13
            fprintf(fp, "drop-unicast\n");

            fprintf(fp, "reconfigure-enabled 1\n");

            if (config->interfaces[i].server_type != DHCPV6_SERVER_TYPE_STATEFUL)
                fprintf(fp, "stateless\n");

            fprintf(fp, "iface %s {\n", config->interfaces[i].interface_name);


            if (config->interfaces[i].rapid_enable)
            {
                fprintf(fp, "   rapid-commit yes\n");
            }

            fprintf(fp, "   preference %d\n", 255);

            if (config->interfaces[i].iana_enable)
            {
                fprintf(fp, "   class {\n");

                fprintf(fp, "       pool %s - %s\n",config->interfaces[i].ipv6_address_pool.startAddress,config->interfaces[i].ipv6_address_pool.endAddress);
            }
            else
            {
                // IANA is disabled, so skip the entire class block
                fprintf(fp, "   # IANA is disabled for this interface\n");
            }

            fprintf(fp, "       T1 %lu\n", (unsigned long)config->interfaces[i].renew_time);
            fprintf(fp, "       T2 %lu\n", (unsigned long)config->interfaces[i].rebind_time);
            fprintf(fp, "       prefered-lifetime %lu\n", (unsigned long)config->interfaces[i].preferred_lifetime);
            fprintf(fp, "       valid-lifetime %lu\n", (unsigned long)config->interfaces[i].valid_lifetime);
            fprintf(fp, "   }\n");

            //Add options dns server info to server.conf file
            if (config->interfaces[i].server_type != DHCPV6_SERVER_TYPE_STATEFUL)
            {
                goto OPTIONS;
            }
OPTIONS:
            addOptions(fp, config->interfaces[i].options, config->interfaces[i].num_options);

            fprintf(fp, "}\n"); // Close the interface block
        }
        if (config->interfaces[i].ra_params.enable_ra == true && config->interfaces[i].server_type == DHCPV6_SERVER_TYPE_STATELESS)
        {
            createRAConfig(rAfp,&config->interfaces[i]); 
        }

    } //Loop end for DhcpV6 config
    if (rAfp != NULL)
    {
        //make the header to 0 so that next time it will write the header
        raConfigHeader = 0;
        fclose(rAfp);
    }
    if (fp != NULL)
    {
        fclose(fp);
    }
    return 0;
}

// Function to start the DHCPv6 server
void dhcpv6_server_start(char *server, server_type_t server_type)
{
#if ENABLE_TEST
    if(server_type == DHCPV6_SERVER_TYPE_STATEFUL)
    {
        printf("Starting  %s server\n",server);
    }
    else if(server_type == DHCPV6_SERVER_TYPE_STATELESS)
    {
        printf("Starting %s server\n",server);
    }
#else
    char cmd[256] = {0};
    if(server != NULL)
    {
        switch(server_type)
        {
            case DHCPV6_SERVER_TYPE_STATEFUL:
            {
                // Parse the server names
                snprintf(cmd,sizeof(cmd), "%s start", server);
                executeCmd(cmd);
                break;
            }
            case DHCPV6_SERVER_TYPE_STATELESS:
            {
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"%s -d -f %s -P 0",server, RA_CONF_FILE);
                executeCmd(cmd);
                break;
            }
            default:
                printf("Invalid server type\n");
                break;
        }
    }
#endif
}

// Function to stop the DHCPv6 server
void dhcpv6_server_stop(char* server,server_type_t server_type)
{
#if ENABLE_TEST
    if(server_type == DHCPV6_SERVER_TYPE_STATEFUL)
    {
        printf("Stopping %s server\n",server);
    }
    else if(server_type == DHCPV6_SERVER_TYPE_STATELESS)
    {
        printf("Stopping %s server\n",server);
    }
#else
    char cmd[256] = {0};
    if(server != NULL)
    {
        switch(server_type)
        {
            case DHCPV6_SERVER_TYPE_STATEFUL:
            {
               // Parse the server names
                snprintf(cmd,sizeof(cmd), "%s stop >/dev/null", server);
                executeCmd(cmd);
                break;
            }
            case DHCPV6_SERVER_TYPE_STATELESS:
            {
                stopRAService(server); 
                break;
            }
            default:
                printf("Invalid server type\n");
                break;
        }
    }
#endif
}
