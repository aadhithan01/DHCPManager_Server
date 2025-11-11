#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <rbus/rbus.h>
#include <cjson/cJSON.h>

#define DHCPMGR_SERVERv4_EVENT "Device.DHCP.Server.v4.Event"
#define DHCPMGR_SERVERv6_EVENT "Device.DHCP.Server.v6.Event"
#define DHCPMGR_SERVER_READY "Device.DHCP.Server.StateReady"
#define DHCPMGR_SERVER_STATE "Device.DHCP.Server.State"
#define DHCP_CONFIG_PARAM "Device.LanManager.DhcpConfig"
#define  MAC_ADDR_SIZE 18
#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))

static rbusHandle_t handle = NULL;
static volatile int keepRunning = 1;

// Helper function to create a single DHCP entry
static cJSON* create_dhcp_entry(
    int networkBridgeType,
    int userBridgeCategory,
    const char *alias,
    int stpEnable,
    int igdEnable,
    int bridgeLifeTime,
    const char *bridgeName,
    int dhcpv4_enable,
    const char *dhcpv4_start,
    const char *dhcpv4_end,
    int dhcpv4_lease,
    const char *ipv6_prefix,
    int stateful,
    int stateless,
    const char *dhcpv6_start,
    const char *dhcpv6_end,
    int addr_type)
{
    cJSON *entry = cJSON_CreateObject();
    
    // Bridge Info
    cJSON *bridgeInfo = cJSON_CreateObject();
    cJSON_AddNumberToObject(bridgeInfo, "networkBridgeType", networkBridgeType);
    cJSON_AddNumberToObject(bridgeInfo, "userBridgeCategory", userBridgeCategory);
    cJSON_AddStringToObject(bridgeInfo, "alias", alias);
    cJSON_AddNumberToObject(bridgeInfo, "stpEnable", stpEnable);
    cJSON_AddNumberToObject(bridgeInfo, "igdEnable", igdEnable);
    cJSON_AddNumberToObject(bridgeInfo, "bridgeLifeTime", bridgeLifeTime);
    cJSON_AddStringToObject(bridgeInfo, "bridgeName", bridgeName);
    cJSON_AddItemToObject(entry, "bridgeInfo", bridgeInfo);
    
    // DHCP Config
    cJSON *dhcpConfig = cJSON_CreateObject();
    
    // DHCPv4 Config
    cJSON *dhcpv4Config = cJSON_CreateObject();
    cJSON_AddBoolToObject(dhcpv4Config, "Dhcpv4_Enable", dhcpv4_enable);
    cJSON_AddStringToObject(dhcpv4Config, "Dhcpv4_Start_Addr", dhcpv4_start);
    cJSON_AddStringToObject(dhcpv4Config, "Dhcpv4_End_Addr", dhcpv4_end);
    cJSON_AddNumberToObject(dhcpv4Config, "Dhcpv4_Lease_Time", dhcpv4_lease);
    cJSON_AddItemToObject(dhcpConfig, "dhcpv4Config", dhcpv4Config);
    
    // DHCPv6 Config
    cJSON *dhcpv6Config = cJSON_CreateObject();
    cJSON_AddStringToObject(dhcpv6Config, "Ipv6Prefix", ipv6_prefix);
    cJSON_AddBoolToObject(dhcpv6Config, "StateFull", stateful);
    cJSON_AddBoolToObject(dhcpv6Config, "StateLess", stateless);
    cJSON_AddStringToObject(dhcpv6Config, "Dhcpv6_Start_Addr", dhcpv6_start);
    cJSON_AddStringToObject(dhcpv6Config, "Dhcpv6_End_Addr", dhcpv6_end);
    cJSON_AddNumberToObject(dhcpv6Config, "addrType", addr_type);
    cJSON_AddNullToObject(dhcpv6Config, "customConfig");
    cJSON_AddItemToObject(dhcpConfig, "dhcpv6Config", dhcpv6Config);
    
    cJSON_AddItemToObject(entry, "dhcpConfig", dhcpConfig);
    
    return entry;
}

// Function to construct the complete JSON
static char* construct_dhcp_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "num_entries", 3);
    
    cJSON *dhcpPayload = cJSON_CreateArray();
    
    // Entry 1: br-lan
    cJSON *entry1 = create_dhcp_entry(
        0, 1, "br-lan", 1, 1, -1, "brlan0",
        1, "192.168.1.2", "192.168.1.254", 86400,
        "fd00:1:1::/64", 1, 0, "fd00:1:1::10", "fd00:1:1::ffff", 1
    );
    cJSON_AddItemToArray(dhcpPayload, entry1);
    
    // Entry 2: br-hotspot 1
    cJSON *entry2 = create_dhcp_entry(
        0, 3, "br-hotspot", 0, 0, -1, "brlan1",
        1, "10.0.0.2", "10.0.0.200", 43200,
        "2001:db8:100::/64", 1, 0, "2001:db8:100::20", "2001:db8:100::200", 0
    );
    cJSON_AddItemToArray(dhcpPayload, entry2);
    
    // Entry 3: br-hotspot 2
    cJSON *entry3 = create_dhcp_entry(
        0, 3, "br-hotspot", 0, 0, -1, "br1an2",
        1, "172.168.0.2", "172.168.0.200", 43200,
        "2001:db8:100::/64", 1, 0, "2001:db8:100::20", "2001:db8:100::200", 0
    );
    cJSON_AddItemToArray(dhcpPayload, entry3);
    
    cJSON_AddItemToObject(root, "dhcpPayload", dhcpPayload);
    
    // Convert to string
    char *json_string = cJSON_Print(root);
    
    // Cleanup
    cJSON_Delete(root);
    
    return json_string;
}

static void intHandler(int dummy)
{
    (void)dummy;
    keepRunning = 0;
}


static rbusError_t get_handler(rbusHandle_t handle, rbusProperty_t prop, rbusGetHandlerOptions_t* opts)
{
    (void)handle;
    (void)opts;

    const char* name = rbusProperty_GetName(prop);
    if (strcmp(name, DHCP_CONFIG_PARAM) == 0) {
        // Construct JSON dynamically
        char *json_string = construct_dhcp_json();
        
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetString(value, json_string);
        rbusProperty_SetValue(prop, value);
        rbusValue_Release(value);
        
        printf("Handled get for %s\n", name);
        printf("JSON Response: %s\n", json_string);
        
        // Free the JSON string
        free(json_string);
        
        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusDataElement_t dataElements[1] = {
    {DHCP_CONFIG_PARAM, RBUS_ELEMENT_TYPE_PROPERTY, {get_handler, NULL, NULL, NULL, NULL, NULL}}
};


int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    rbusError_t rc;

    signal(SIGINT, intHandler);

    rc = rbus_open(&handle, "Rbus_client_test_consumer");
    if (rc != RBUS_ERROR_SUCCESS) {
        printf("rbus_open failed: %d\n", rc);
        return -1;
    }

rc = rbus_regDataElements(handle, ARRAY_SZ(dataElements), dataElements);
if (rc != RBUS_ERROR_SUCCESS) {
    printf("Failed to register data element %s: %d\n", DHCP_CONFIG_PARAM, rc);
} else {
    printf("Registered data element for %s\n", DHCP_CONFIG_PARAM);
}

    while (keepRunning) {
        sleep(1);
    }

    printf("Shutting down: unsubscribing and closing rbus\n");
    rbus_close(handle);
    return 0;
}
