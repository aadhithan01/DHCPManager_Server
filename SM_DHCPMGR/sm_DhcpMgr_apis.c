#include "sm_DhcpMgr.h"
#include <cjson/cJSON.h>
#include "dhcpmgr_rbus_apis.h"

const char* DhcpManagersState_Names[]={
    "DHCPS_STATE_IDLE",
    "DHCPS_STATE_PREPARINGv4",
    "DHCPS_STATE_STARTINGv4",
    "DHCPS_STATE_STOPPINGv4",
    "DHCPS_STATE_PREPARINGv6",
    "DHCPS_STATE_STARTINGv6",
    "DHCPS_STATE_STOPPINGv6"
};

const char *json_input = "{\
  \"num_entries\": 3,\
  \"dhcpPayload\": [\
    {\
      \"bridgeInfo\": {\
        \"networkBridgeType\": 0,\
        \"userBridgeCategory\": 1,\
        \"alias\": \"br-lan\",\
        \"stpEnable\": 1,\
        \"igdEnable\": 1,\
        \"bridgeLifeTime\": -1,\
        \"bridgeName\": \"brlan0\"\
      },\
      \"dhcpConfig\": {\
        \"dhcpv4Config\": {\
          \"Dhcpv4_Enable\": true,\
          \"Dhcpv4_Start_Addr\": \"192.168.1.2\",\
          \"Dhcpv4_End_Addr\": \"192.168.1.254\",\
          \"Dhcpv4_Lease_Time\": 86400\
        },\
        \"dhcpv6Config\": {\
          \"Ipv6Prefix\": \"fd00:1:1::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"fd00:1:1::10\",\
          \"Dhcpv6_End_Addr\": \"fd00:1:1::ffff\",\
          \"addrType\": 1,\
          \"customConfig\": null\
        }\
      }\
    },\
    {\
      \"bridgeInfo\": {\
        \"networkBridgeType\": 0,\
        \"userBridgeCategory\": 3,\
        \"alias\": \"br-hotspot\",\
        \"stpEnable\": 0,\
        \"igdEnable\": 0,\
        \"bridgeLifeTime\": -1,\
        \"bridgeName\": \"brlan1\"\
      },\
      \"dhcpConfig\": {\
        \"dhcpv4Config\": {\
          \"Dhcpv4_Enable\": true,\
          \"Dhcpv4_Start_Addr\": \"10.0.0.2\",\
          \"Dhcpv4_End_Addr\": \"10.0.0.200\",\
          \"Dhcpv4_Lease_Time\": 43200\
        },\
        \"dhcpv6Config\": {\
          \"Ipv6Prefix\": \"2001:db8:100::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"2001:db8:100::20\",\
          \"Dhcpv6_End_Addr\": \"2001:db8:100::200\",\
          \"addrType\": 0,\
          \"customConfig\": null\
        }\
      }\
    },\
    {\
      \"bridgeInfo\": {\
        \"networkBridgeType\": 0,\
        \"userBridgeCategory\": 3,\
        \"alias\": \"br-hotspot\",\
        \"stpEnable\": 0,\
        \"igdEnable\": 0,\
        \"bridgeLifeTime\": -1,\
        \"bridgeName\": \"br1an2\"\
      },\
      \"dhcpConfig\": {\
        \"dhcpv4Config\": {\
          \"Dhcpv4_Enable\": true,\
          \"Dhcpv4_Start_Addr\": \"172.168.0.2\",\
          \"Dhcpv4_End_Addr\": \"172.168.0.200\",\
          \"Dhcpv4_Lease_Time\": 43200\
        },\
        \"dhcpv6Config\": {\
          \"Ipv6Prefix\": \"2001:db8:100::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"2001:db8:100::20\",\
          \"Dhcpv6_End_Addr\": \"2001:db8:100::200\",\
          \"addrType\": 0,\
          \"customConfig\": null\
        }\
      }\
    }\
  ]\
}";
//stub function to simulate fetching LAN configurations

void GetDhcpStateName(DHCPS_State state, char *stateName, size_t nameSize) {
    if (state >= 0 && state < sizeof(DhcpManagersState_Names)/sizeof(DhcpManagersState_Names[0])) {
        snprintf(stateName, nameSize, "%s", DhcpManagersState_Names[state]);
    } else {
        snprintf(stateName, nameSize, "UNKNOWN_STATE");
    }
}

int parseDhcpPayloadJson(const char *json, DhcpPayload *payloads, int *count) {
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        printf("Failed to parse JSON\n");
        return -1;
    }

    *count = cJSON_GetObjectItem(root, "num_entries")->valueint;
    cJSON *array = cJSON_GetObjectItem(root, "dhcpPayload");

    for (int i = 0; i < *count; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        cJSON *bridgeInfo = cJSON_GetObjectItem(item, "bridgeInfo");
        cJSON *dhcpConfig = cJSON_GetObjectItem(item, "dhcpConfig");

        payloads[i].bridgeInfo.networkBridgeType = cJSON_GetObjectItem(bridgeInfo, "networkBridgeType")->valueint;
        payloads[i].bridgeInfo.userBridgeCategory = cJSON_GetObjectItem(bridgeInfo, "userBridgeCategory")->valueint;
        strcpy(payloads[i].bridgeInfo.alias, cJSON_GetObjectItem(bridgeInfo, "alias")->valuestring);
        payloads[i].bridgeInfo.stpEnable = cJSON_GetObjectItem(bridgeInfo, "stpEnable")->valueint;
        payloads[i].bridgeInfo.igdEnable = cJSON_GetObjectItem(bridgeInfo, "igdEnable")->valueint;
        payloads[i].bridgeInfo.bridgeLifeTime = cJSON_GetObjectItem(bridgeInfo, "bridgeLifeTime")->valueint;
        strcpy(payloads[i].bridgeInfo.bridgeName, cJSON_GetObjectItem(bridgeInfo, "bridgeName")->valuestring);

        cJSON *dhcpv4 = cJSON_GetObjectItem(dhcpConfig, "dhcpv4Config");
        payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Enable"));
        strcpy(payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Start_Addr")->valuestring);
        strcpy(payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_End_Addr")->valuestring);
        payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time = cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Lease_Time")->valueint;
        strcpy(payloads[i].dhcpConfig.dhcpv4Config.Subnet_Mask, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Subnet")->valuestring);

        cJSON *dhcpv6 = cJSON_GetObjectItem(dhcpConfig, "dhcpv6Config");
        strcpy(payloads[i].dhcpConfig.dhcpv6Config.Ipv6Prefix, cJSON_GetObjectItem(dhcpv6, "Ipv6Prefix")->valuestring);
        payloads[i].dhcpConfig.dhcpv6Config.StateFull = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv6, "StateFull"));
        payloads[i].dhcpConfig.dhcpv6Config.StateLess = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv6, "StateLess"));
        strcpy(payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr, cJSON_GetObjectItem(dhcpv6, "Dhcpv6_Start_Addr")->valuestring);
        strcpy(payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr, cJSON_GetObjectItem(dhcpv6, "Dhcpv6_End_Addr")->valuestring);
        payloads[i].dhcpConfig.dhcpv6Config.addrType = cJSON_GetObjectItem(dhcpv6, "addrType")->valueint;
        payloads[i].dhcpConfig.dhcpv6Config.customConfig = NULL;
    }

    cJSON_Delete(root);
    return 0;
}

void printLanDHCPConfig(DhcpPayload *lanConfigs, int LanConfig_count) {
    for (int i = 0; i < LanConfig_count; i++) {
        printf("Bridge Name: %s\n", lanConfigs[i].bridgeInfo.bridgeName);
        printf("  DHCPv4 Enabled: %s\n", lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable ? "Yes" : "No");
        if (lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable) {
            printf("  DHCPv4 Start Address: %s\n", lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            printf("  DHCPv4 End Address: %s\n", lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            printf("  DHCPv4 Lease Time: %d seconds\n", lanConfigs[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
            printf("  DHCPv4 Subnet Mask: %s\n", lanConfigs[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
        }
        printf("  DHCPv6 Prefix: %s\n", lanConfigs[i].dhcpConfig.dhcpv6Config.Ipv6Prefix);
        printf("  DHCPv6 Stateful: %s\n", lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull ? "Yes" : "No");
        printf("  DHCPv6 Stateless: %s\n", lanConfigs[i].dhcpConfig.dhcpv6Config.StateLess ? "Yes" : "No");
        if (lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull) {
            printf("  DHCPv6 Start Address: %s\n", lanConfigs[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr);
            printf("  DHCPv6 End Address: %s\n", lanConfigs[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr);
        }
        printf("\n");
    }
}

int GetLanDHCPConfig(DhcpPayload *lanConfigs, int *LanConfig_count)
{
    //RBUS call for LAN DHCP Config
    const char* payload = NULL;
    rbus_GetLanDHCPConfig(&payload);
    printf("%s:%d received payload is %s\n",__FUNCTION__,__LINE__,payload);
    parseDhcpPayloadJson(payload, lanConfigs, LanConfig_count);

//    parseDhcpPayloadJson(json_input, lanConfigs, LanConfig_count);
    printLanDHCPConfig(lanConfigs, *LanConfig_count);

    return 0; // Success
}

void Add_inf_to_dhcp_config(DhcpPayload *pLanConfig, int numOfLanConfigs, DhcpInterfaceConfig **ppHeadDhcpIf,int pDhcpIfacesCount)
{
    if (pLanConfig == NULL || ppHeadDhcpIf == NULL || numOfLanConfigs <= 0)
    {
        return;
    }

    for (int i = 0; i < pDhcpIfacesCount; i++)
    {
        ppHeadDhcpIf[i]->bIsDhcpEnabled = pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable;
        snprintf(ppHeadDhcpIf[i]->cGatewayName, sizeof(ppHeadDhcpIf[i]->cGatewayName), "%s", pLanConfig[i].bridgeInfo.bridgeName);

        if (pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable)
        {
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cStartAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cStartAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            printf("Source Start Address: %s\n", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cStartAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cStartAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
            printf("Copied Start Address: %s\n", ppHeadDhcpIf[i]->sAddressPool.cStartAddress);

            printf("Source End Address: %s\n", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cEndAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cEndAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            printf("Copied End Address: %s\n", ppHeadDhcpIf[i]->sAddressPool.cEndAddress);

            printf("Source Subnet Mask: %s\n", pLanConfig[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
            snprintf(ppHeadDhcpIf[i]->cSubnetMask, sizeof(ppHeadDhcpIf[i]->cSubnetMask), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
            printf("Copied Subnet Mask: %s\n", ppHeadDhcpIf[i]->cSubnetMask);

            printf("Source Lease Duration: %d\n", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
            snprintf(ppHeadDhcpIf[i]->cLeaseDuration, sizeof(ppHeadDhcpIf[i]->cLeaseDuration), "%d", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
            printf("Copied Lease Duration: %s\n", ppHeadDhcpIf[i]->cLeaseDuration);
        }
    }
}

void AllocateDhcpInterfaceConfig(DhcpInterfaceConfig ***ppDhcpCfgs,int LanConfig_count)
{
    *ppDhcpCfgs = (DhcpInterfaceConfig **)malloc(LanConfig_count * sizeof(DhcpInterfaceConfig *));
        if (*ppDhcpCfgs == NULL)
        {
            printf("%s:%d Memory allocation for ppDhcpCfgs failed\n", __FUNCTION__, __LINE__);
            return;
        }
        for (int i = 0; i < LanConfig_count; i++)
        {
            (*ppDhcpCfgs)[i] = (DhcpInterfaceConfig *)malloc(sizeof(DhcpInterfaceConfig));
            if ((*ppDhcpCfgs)[i] == NULL)
            {
                printf("%s:%d Memory allocation for ppDhcpCfgs[%d] failed\n", __FUNCTION__, __LINE__, i);
                return;
            }
            memset((*ppDhcpCfgs)[i], 0, sizeof(DhcpInterfaceConfig));
        }
}

int Construct_dhcp_configurationv4(char * dhcpOptions, char * dnsonly)
{
    if (dnsonly == NULL)     
    {
        strcpy(dhcpOptions, "-q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C /var/dnsmasq.conf --dhcp-authoritative --stop-dns-rebind --log-facility=/home/aadhithan/Desktop/aadhi/dnsmasq.log");
    }
    else if(strcmp(dnsonly, "true") == 0)
    {
        strcpy(dhcpOptions, " -P 4096 -C /var/dnsmasq.conf --log-facility=/home/aadhithan/Desktop/aadhi/dnsmasq.log");
    }
    return 0;
}

int dhcp_server_publish_state(DHCPS_State state)
{
    printf("%s %d: rbus publish state called with state %d\n",__FUNCTION__, __LINE__, state);
    char stateStr[16] = {0};
    char paramName[20] = {0};

    switch(state)
    {
        case DHCPS_STATE_IDLE:
            snprintf(stateStr, sizeof(stateStr), "Idle");
            snprintf(paramName,sizeof(paramName),"DHCP_server_state");
            break;
        case DHCPS_STATE_PREPARINGv4:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_STARTINGv4:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_STOPPINGv4:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_PREPARINGv6:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        case DHCPS_STATE_STARTINGv6:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        case DHCPS_STATE_STOPPINGv6:
            GetDhcpStateName(state, stateStr, sizeof(stateStr));
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        default:
            snprintf(stateStr, sizeof(stateStr), "Unknown");
            snprintf(paramName,sizeof(paramName),"DHCP_server_state");
            break;
    }

    if (DhcpMgr_Publish_Events(stateStr, paramName, DHCPMGR_SERVER_STATE) != 0)
    {
        printf("%s %d: Failed to publish state %s\n",__FUNCTION__, __LINE__, stateStr);
        return -1;
    }

    return 0;
}

int dhcp_server_signal_state_machine_ready()
{
    if (DhcpMgr_Publish_Events("Ready", "DHCP_Server_state", DHCPMGR_SERVER_READY) != 0)
    {
        printf("%s %d: Failed to signal state machine ready\n",__FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

void dns_only()
{
    fprintf(stdout, "%s:%d, Writing dns_only configuration\n", __FUNCTION__, __LINE__);
    int iter=0;
        // Stub to comment the interface and dcprange lines in dhcp conf file
    FILE *dhcpConfFile = fopen("/var/dnsmasq.conf", "r+");
    if (dhcpConfFile == NULL) {
        printf("%s:%d Failed to open dhcpd.conf file\n", __FUNCTION__, __LINE__);
        return;
    }
    fprintf(stdout, "%s:%d, Opened dhcpd.conf file successfully\n", __FUNCTION__, __LINE__);
    char line[1024];
    FILE *tempFile = fopen("/tmp/dnsmasq.conf.tmp", "w");
    if (tempFile == NULL) {
        printf("%s:%d Failed to create temporary file\n", __FUNCTION__, __LINE__);
        fclose(dhcpConfFile);
        return;
    }
    fprintf(stdout, "%s:%d, Created temporary file successfully\n", __FUNCTION__, __LINE__);

    while (fgets(line, sizeof(line), dhcpConfFile)) {
        fprintf(stdout, "%s:%d Commented line: %s and iter=%d\n", __FUNCTION__, __LINE__, line, iter);
        if ((strstr(line, "interface")) && iter < 2) {
            fprintf(stdout, "%s:%d interface found %s", __FUNCTION__, __LINE__, line);
            fprintf(tempFile, "#%s", line); // Comment the line
            iter++;
        }
        else if (strstr(line, "dhcp-range") && iter < 3)
        {
            fprintf(stdout, "%s:%d dhcp-range found %s", __FUNCTION__, __LINE__, line);
            fprintf(tempFile, "#%s", line); // Comment the line
        }
        else {
            fprintf(tempFile, "%s", line); // Write the line as is
        }
    }
 /*   if (rename("/var/dnsmasq.conf.tmp", "/var/dnsmasq.conf") != 0) {
        printf("%s:%d Failed to replace dnsmasq.conf file\n", __FUNCTION__, __LINE__);
    } */
    fprintf(stdout, "%s:%d, dns_only configuration written successfully\n", __FUNCTION__, __LINE__);
    fclose(dhcpConfFile);
    fclose(tempFile);
    //stub ends
}

