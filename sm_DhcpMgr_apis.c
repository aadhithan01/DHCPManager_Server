#include "sm_DhcpMgr.h"
#include <cjson/cJSON.h>
#include "dhcpmgr_rbus_apis.h"

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
          \"Ipv6Enable\": true,\
          \"Ipv6Prefix\": \"fd00:1:1::/64\",\
          \"StateFull\": true,\
          \"StateLess\": false,\
          \"Dhcpv6_Start_Addr\": \"fd00:1:1::10\",\
          \"Dhcpv6_End_Addr\": \"fd00:1:1::ffff\",\
          \"LeaseTime\": 86400,\
          \"RenewTime\": 43200,\
          \"RebindTime\": 75600,\
          \"ValidLifeTime\": 172800,\
          \"PreferredLifeTime\": 86400,\
          \"num_options\": 0,\
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
          \"Ipv6Enable\": false,\
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
          \"Ipv6Enable\": false,\
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


int parseDhcpPayloadJson(const char *json, DhcpPayload *payloads, int *count) {
  printf("%s:%d Entering function\n", __FUNCTION__, __LINE__);
  cJSON *root = cJSON_Parse(json);
  if (!root) {
    printf("%s:%d Failed to parse JSON\n", __FUNCTION__, __LINE__);
    return -1;
  }

  *count = cJSON_GetObjectItem(root, "num_entries")->valueint;
  printf("%s:%d Parsed num_entries: %d\n", __FUNCTION__, __LINE__, *count);
  cJSON *array = cJSON_GetObjectItem(root, "dhcpPayload");

  for (int i = 0; i < *count; i++) {
    printf("%s:%d Parsing entry %d\n", __FUNCTION__, __LINE__, i);
    cJSON *item = cJSON_GetArrayItem(array, i);
    cJSON *bridgeInfo = cJSON_GetObjectItem(item, "bridgeInfo");
    cJSON *dhcpConfig = cJSON_GetObjectItem(item, "dhcpConfig");

    payloads[i].bridgeInfo.networkBridgeType = cJSON_GetObjectItem(bridgeInfo, "networkBridgeType")->valueint;
    printf("%s:%d Parsed networkBridgeType: %d\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.networkBridgeType);
    payloads[i].bridgeInfo.userBridgeCategory = cJSON_GetObjectItem(bridgeInfo, "userBridgeCategory")->valueint;
    printf("%s:%d Parsed userBridgeCategory: %d\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.userBridgeCategory);
    strcpy(payloads[i].bridgeInfo.alias, cJSON_GetObjectItem(bridgeInfo, "alias")->valuestring);
    printf("%s:%d Parsed alias: %s\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.alias);
    payloads[i].bridgeInfo.stpEnable = cJSON_GetObjectItem(bridgeInfo, "stpEnable")->valueint;
    printf("%s:%d Parsed stpEnable: %d\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.stpEnable);
    payloads[i].bridgeInfo.igdEnable = cJSON_GetObjectItem(bridgeInfo, "igdEnable")->valueint;
    printf("%s:%d Parsed igdEnable: %d\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.igdEnable);
    payloads[i].bridgeInfo.bridgeLifeTime = cJSON_GetObjectItem(bridgeInfo, "bridgeLifeTime")->valueint;
    printf("%s:%d Parsed bridgeLifeTime: %d\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.bridgeLifeTime);
    strcpy(payloads[i].bridgeInfo.bridgeName, cJSON_GetObjectItem(bridgeInfo, "bridgeName")->valuestring);
    printf("%s:%d Parsed bridgeName: %s\n", __FUNCTION__, __LINE__, payloads[i].bridgeInfo.bridgeName);

    cJSON *dhcpv4 = cJSON_GetObjectItem(dhcpConfig, "dhcpv4Config");
    payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Enable"));
    printf("%s:%d Parsed Dhcpv4_Enable: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Enable);
    strcpy(payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Start_Addr")->valuestring);
    printf("%s:%d Parsed Dhcpv4_Start_Addr: %s\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Start_Addr);
    strcpy(payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr, cJSON_GetObjectItem(dhcpv4, "Dhcpv4_End_Addr")->valuestring);
    printf("%s:%d Parsed Dhcpv4_End_Addr: %s\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
    payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time = cJSON_GetObjectItem(dhcpv4, "Dhcpv4_Lease_Time")->valueint;
    printf("%s:%d Parsed Dhcpv4_Lease_Time: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);

    cJSON *dhcpv6 = cJSON_GetObjectItem(dhcpConfig, "dhcpv6Config");
    strcpy(payloads[i].dhcpConfig.dhcpv6Config.Ipv6Prefix, cJSON_GetObjectItem(dhcpv6, "Ipv6Prefix")->valuestring);
    printf("%s:%d Parsed Ipv6Prefix: %s\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.Ipv6Prefix);
    payloads[i].dhcpConfig.dhcpv6Config.StateFull = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv6, "StateFull"));
    printf("%s:%d Parsed StateFull: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.StateFull);
    payloads[i].dhcpConfig.dhcpv6Config.StateLess = cJSON_IsTrue(cJSON_GetObjectItem(dhcpv6, "StateLess"));
    printf("%s:%d Parsed StateLess: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.StateLess);
    strcpy(payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr, cJSON_GetObjectItem(dhcpv6, "Dhcpv6_Start_Addr")->valuestring);
    printf("%s:%d Parsed Dhcpv6_Start_Addr: %s\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr);
    strcpy(payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr, cJSON_GetObjectItem(dhcpv6, "Dhcpv6_End_Addr")->valuestring);
    printf("%s:%d Parsed Dhcpv6_End_Addr: %s\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr);
    payloads[i].dhcpConfig.dhcpv6Config.addrType = cJSON_GetObjectItem(dhcpv6, "addrType")->valueint;
    printf("%s:%d Parsed addrType: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.addrType);
    payloads[i].dhcpConfig.dhcpv6Config.customConfig = NULL;
    cJSON *leaseTime = cJSON_GetObjectItem(dhcpv6, "LeaseTime");
    if (leaseTime) {
        payloads[i].dhcpConfig.dhcpv6Config.LeaseTime = leaseTime->valueint;
        printf("%s:%d Parsed LeaseTime: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.LeaseTime);
    }
    else
    {
        payloads[i].dhcpConfig.dhcpv6Config.LeaseTime = 0;
        printf("%s:%d LeaseTime not found, set to default: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.LeaseTime);
    }

    cJSON *renewTime = cJSON_GetObjectItem(dhcpv6, "RenewTime");
    if (renewTime) {
        payloads[i].dhcpConfig.dhcpv6Config.RenewTime = renewTime->valueint;
        printf("%s:%d Parsed RenewTime: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.RenewTime);
    }

    cJSON *rebindTime = cJSON_GetObjectItem(dhcpv6, "RebindTime");
    if (rebindTime) {
        payloads[i].dhcpConfig.dhcpv6Config.RebindTime = rebindTime->valueint;
        printf("%s:%d Parsed RebindTime: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.RebindTime);
    }

    cJSON *validLifeTime = cJSON_GetObjectItem(dhcpv6, "ValidLifeTime");
    if (validLifeTime) {
        payloads[i].dhcpConfig.dhcpv6Config.ValidLifeTime = validLifeTime->valueint;
        printf("%s:%d Parsed ValidLifeTime: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.ValidLifeTime);
    }

    cJSON *preferredLifeTime = cJSON_GetObjectItem(dhcpv6, "PreferredLifeTime");
    if (preferredLifeTime) {
        payloads[i].dhcpConfig.dhcpv6Config.PreferredLifeTime = preferredLifeTime->valueint;
        printf("%s:%d Parsed PreferredLifeTime: %d\n", __FUNCTION__, __LINE__, payloads[i].dhcpConfig.dhcpv6Config.PreferredLifeTime);
    }
  }

  cJSON_Delete(root);
  printf("%s:%d Exiting function\n", __FUNCTION__, __LINE__);
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

int check_ipv6_received(DhcpPayload *lanConfigs, int LanConfig_count, bool *statefull_enabled, bool *stateless_enabled)
{
    IPv6Dhcpv6InterfaceConfig interface;

    memset(&interface, 0, sizeof(IPv6Dhcpv6InterfaceConfig));
    for (int i = 0; i < LanConfig_count; i++) {
      if (strcmp(lanConfigs[i].bridgeInfo.bridgeName, "brlan0") == 0 &&
        lanConfigs[i].dhcpConfig.dhcpv6Config.Ipv6Prefix[0] != '\0') 
      {
        *statefull_enabled = lanConfigs[i].dhcpConfig.dhcpv6Config.StateFull;
        *stateless_enabled = lanConfigs[i].dhcpConfig.dhcpv6Config.StateLess;
        return 0; // brlan0 has DHCPv6 enabled
      }
    }
    return -1; // No interfaces with DHCPv6 enabled
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
            snprintf(ppHeadDhcpIf[i]->sAddressPool.cEndAddress, sizeof(ppHeadDhcpIf[i]->sAddressPool.cEndAddress), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_End_Addr);
            snprintf(ppHeadDhcpIf[i]->cSubnetMask, sizeof(ppHeadDhcpIf[i]->cSubnetMask), "%s", pLanConfig[i].dhcpConfig.dhcpv4Config.Subnet_Mask);
            snprintf(ppHeadDhcpIf[i]->cLeaseDuration, sizeof(ppHeadDhcpIf[i]->cLeaseDuration), "%d", pLanConfig[i].dhcpConfig.dhcpv4Config.Dhcpv4_Lease_Time);
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
        strcpy(dhcpOptions, "-q --clear-on-reload --bind-dynamic --add-mac --add-cpe-id=abcdefgh -P 4096 -C /var/dnsmasq.conf --dhcp-authoritative --stop-dns-rebind --log-facility=/home/aadhithan/Desktop/aadhi/DHCP_MGR/dnsmasq.log");
    }
    else if(strcmp(dnsonly, "true") == 0)
    {
        strcpy(dhcpOptions, " -P 4096 -C /var/dnsmasq.conf --log-facility=/home/aadhithan/Desktop/aadhi/DHCP_MGR/dnsmasq.log");
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

int create_dhcpsv6_config(DhcpPayload *lanConfigs)
{
    bool statefull_enabled = false;
    bool stateless_enabled = false;
    int ret = check_ipv6_received(lanConfigs, 3, &statefull_enabled, &stateless_enabled);
    if (ret != 0)
    {
        printf("%s: %d No brlan0 interface with DHCPv6 enabled. Skipping DHCPv6 configuration.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    else
    {
        if (statefull_enabled)
        {
            printf("%s: %d Stateful DHCPv6 is enabled on brlan0.\n", __FUNCTION__, __LINE__);
            IPv6Dhcpv6InterfaceConfig interface;
            IPv6Dhcpv6ServerConfig config;
            config.num_interfaces = 1;
            memset(&interface, 0, sizeof(IPv6Dhcpv6InterfaceConfig));
            snprintf(interface.interface_name, sizeof(interface.interface_name), "brlan0");
            interface.server_type = DHCPV6_SERVER_TYPE_STATEFUL;
            interface.enable_dhcp = true;
            interface.iana_enable = 1; // Enable IANA to add the pool address on this interface

            //Get prefix length from Ipv6Prefix
            char *prefix = lanConfigs[0].dhcpConfig.dhcpv6Config.Ipv6Prefix;
            char *slash = strchr(prefix, '/');
            if (slash != NULL) {
              interface.ipv6_prefix_length = atoi(slash + 1);
            } else {
              printf("%s:%d Invalid IPv6 prefix format. Using default prefix length of 64.\n", __FUNCTION__, __LINE__);
              interface.ipv6_prefix_length = 64;
            }

            snprintf(interface.ipv6_address_pool.startAddress, sizeof(interface.ipv6_address_pool.startAddress), "%s", lanConfigs[0].dhcpConfig.dhcpv6Config.Dhcpv6_Start_Addr);
            printf("%s:%d Start Address: %s\n", __FUNCTION__, __LINE__, interface.ipv6_address_pool.startAddress);
            snprintf(interface.ipv6_address_pool.endAddress, sizeof(interface.ipv6_address_pool.endAddress), "%s", lanConfigs[0].dhcpConfig.dhcpv6Config.Dhcpv6_End_Addr);
            printf("%s:%d End Address: %s\n", __FUNCTION__, __LINE__, interface.ipv6_address_pool.endAddress);
            interface.lease_time = lanConfigs[0].dhcpConfig.dhcpv6Config.LeaseTime;
            printf("%s:%d Lease Time: %d lease time in conf %d\n", __FUNCTION__, __LINE__, interface.lease_time, lanConfigs[0].dhcpConfig.dhcpv6Config.LeaseTime);
            interface.renew_time = lanConfigs[0].dhcpConfig.dhcpv6Config.RenewTime;
            printf("%s:%d Renew Time: %d Renew Time in Conf %d \n", __FUNCTION__, __LINE__, interface.renew_time, lanConfigs[0].dhcpConfig.dhcpv6Config.RenewTime);
            interface.rebind_time = lanConfigs[0].dhcpConfig.dhcpv6Config.RebindTime;
            printf("%s:%d Rebind Time: %d Rebind Time in Conf %d \n", __FUNCTION__, __LINE__, interface.rebind_time, lanConfigs[0].dhcpConfig.dhcpv6Config.RebindTime);
            interface.valid_lifetime = lanConfigs[0].dhcpConfig.dhcpv6Config.ValidLifeTime; 
            printf("%s:%d Valid Lifetime: %d Valid Lifetime in Conf %d \n", __FUNCTION__, __LINE__, interface.valid_lifetime, lanConfigs[0].dhcpConfig.dhcpv6Config.ValidLifeTime);
            interface.preferred_lifetime = lanConfigs[0].dhcpConfig.dhcpv6Config.PreferredLifeTime;
            printf("%s:%d Preferred Lifetime: %d Preferred Lifetime in Conf %d \n", __FUNCTION__, __LINE__, interface.preferred_lifetime, lanConfigs[0].dhcpConfig.dhcpv6Config.PreferredLifeTime);
            interface.log_level = 8; // Set desired log level
            interface.num_options = 0; // No custom options for now
            interface.rapid_enable = false; // Disable rapid commit
            config.interfaces = &interface;
            config.num_interfaces = 1;
            int ret = dhcpv6_server_create(&config, DHCPV6_SERVER_TYPE_STATEFUL);
            if (ret != 0)
            {
                printf("%s:%d Failed to create DHCPv6 server configuration.\n", __FUNCTION__, __LINE__);
                return -1;
            }
        }
    }
    return 0;
}