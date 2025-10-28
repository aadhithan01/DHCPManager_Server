#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <rbus/rbus.h>

#define DHCPMGR_SERVERv4_EVENT "Device.DHCP.Server.v4.Event"
#define DHCPMGR_SERVERv6_EVENT "Device.DHCP.Server.v6.Event"
#define DHCPMGR_SERVER_READY "Device.DHCP.Server.StateReady"
#define DHCPMGR_SERVER_STATE "Device.DHCP.Server.State"
#define DHCP_CONFIG_PARAM "Device.LanManager.DhcpConfig"
#define  MAC_ADDR_SIZE 18
#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))

static const char *json_input =
"{\"num_entries\": 3, \"dhcpPayload\": ["
"{\"bridgeInfo\": {\"networkBridgeType\": 0, \"userBridgeCategory\": 1, \"alias\": \"br-lan\", \"stpEnable\": 1, \"igdEnable\": 1, \"bridgeLifeTime\": -1, \"bridgeName\": \"brlan0\"},"
"\"dhcpConfig\": {\"dhcpv4Config\": {\"Dhcpv4_Enable\": true, \"Dhcpv4_Start_Addr\": \"192.168.1.2\", \"Dhcpv4_End_Addr\": \"192.168.1.254\", \"Dhcpv4_Lease_Time\": 86400},"
"\"dhcpv6Config\": {\"Ipv6Prefix\": \"fd00:1:1::/64\", \"StateFull\": true, \"StateLess\": false, \"Dhcpv6_Start_Addr\": \"fd00:1:1::10\", \"Dhcpv6_End_Addr\": \"fd00:1:1::ffff\", \"addrType\": 1, \"customConfig\": null}}},"
"{\"bridgeInfo\": {\"networkBridgeType\": 0, \"userBridgeCategory\": 3, \"alias\": \"br-hotspot\", \"stpEnable\": 0, \"igdEnable\": 0, \"bridgeLifeTime\": -1, \"bridgeName\": \"brlan1\"},"
"\"dhcpConfig\": {\"dhcpv4Config\": {\"Dhcpv4_Enable\": true, \"Dhcpv4_Start_Addr\": \"10.0.0.2\", \"Dhcpv4_End_Addr\": \"10.0.0.200\", \"Dhcpv4_Lease_Time\": 43200},"
"\"dhcpv6Config\": {\"Ipv6Prefix\": \"2001:db8:100::/64\", \"StateFull\": true, \"StateLess\": false, \"Dhcpv6_Start_Addr\": \"2001:db8:100::20\", \"Dhcpv6_End_Addr\": \"2001:db8:100::200\", \"addrType\": 0, \"customConfig\": null}}},"
"{\"bridgeInfo\": {\"networkBridgeType\": 0, \"userBridgeCategory\": 3, \"alias\": \"br-hotspot\", \"stpEnable\": 0, \"igdEnable\": 0, \"bridgeLifeTime\": -1, \"bridgeName\": \"br1an2\"},"
"\"dhcpConfig\": {\"dhcpv4Config\": {\"Dhcpv4_Enable\": true, \"Dhcpv4_Start_Addr\": \"172.168.0.2\", \"Dhcpv4_End_Addr\": \"172.168.0.200\", \"Dhcpv4_Lease_Time\": 43200},"
"\"dhcpv6Config\": {\"Ipv6Prefix\": \"2001:db8:100::/64\", \"StateFull\": true, \"StateLess\": false, \"Dhcpv6_Start_Addr\": \"2001:db8:100::20\", \"Dhcpv6_End_Addr\": \"2001:db8:100::200\", \"addrType\": 0, \"customConfig\": null}}}"
"]}";

static rbusHandle_t handle = NULL;
static volatile int keepRunning = 1;

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
        rbusValue_t value;
        rbusValue_Init(&value);
        rbusValue_SetString(value, json_input);
        rbusProperty_SetValue(prop, value);
        rbusValue_Release(value);
        printf("Handled get for %s\n", name);
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
