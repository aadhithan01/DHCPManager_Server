#include "dhcpmgr_rbus_apis.h"


#define  MAC_ADDR_SIZE 18
#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))
static rbusHandle_t rbusHandle;
char *componentName = "DHCPMANAGER";

static rbusError_t DhcpMgr_Publish_Events(char *statestr,char * paramName,char * eventName)
{
    if(statestr == NULL)
    {
        printf("Invalid state string\n");
        return RBUS_ERROR_BUS_ERROR;
    }
    
    int rc = -1;
    rbusEvent_t event;
    rbusObject_t rdata;
    rbusValue_t paramNameVal;

    //Set the event name and value
    rbusObject_Init(&rdata, NULL);
    rbusValue_Init(&paramNameVal);
    rbusValue_SetString(paramNameVal, (char*)statestr);
    rbusObject_SetValue(rdata, paramName, paramNameVal);

    event.name = eventName;
    event.data = rdata;
    event.type = RBUS_EVENT_GENERAL;

    rbusError_t rt = rbusEvent_Publish(rbusHandle, &event);

    if( rt != RBUS_ERROR_SUCCESS && rt != RBUS_ERROR_NOSUBSCRIBERS)
    {
        printf("%s %d - Event %s Publish Failed \n", __FUNCTION__, __LINE__,eventName );
    }
    else
    {
        printf("%s %d - Event %s Published \n", __FUNCTION__, __LINE__,eventName );
        rc = 0;
    }

    rbusValue_Release(paramNameVal);
    rbusObject_Release(rdata);

    return rc;
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
            snprintf(stateStr, sizeof(stateStr), "Preparingv4");
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_STARTINGv4:
            snprintf(stateStr, sizeof(stateStr), "Startingv4");
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_STOPPINGv4:
            snprintf(stateStr, sizeof(stateStr), "Stoppingv4");
            snprintf(paramName,sizeof(paramName),"DHCP_server_v4");
            break;
        case DHCPS_STATE_PREPARINGv6:
            snprintf(stateStr, sizeof(stateStr), "Preparingv6");
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        case DHCPS_STATE_STARTINGv6:
            snprintf(stateStr, sizeof(stateStr), "Startingv6");
            snprintf(paramName,sizeof(paramName),"DHCP_server_v6");
            break;
        case DHCPS_STATE_STOPPINGv6:
            snprintf(stateStr, sizeof(stateStr), "Stoppingv6");
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

int rbus_GetLanDHCPConfig(const char** payload)
{
    printf("%s %d: Fetching LAN DHCP Configurations\n", __FUNCTION__, __LINE__);
    rbusValue_t value = NULL;

    // Get the value from LanManager
    rbusError_t rc = rbus_get(rbusHandle, "Device.LanManager.DhcpConfig", &value);
    if (rc != RBUS_ERROR_SUCCESS)
    {
        printf("%s %d: Failed to fetch LAN DHCP Configurations, rbus_get returned %d\n", __FUNCTION__, __LINE__, rc);
        return -1;
    }

    // Extract the payload from the value
    if (value == NULL)
    {
        printf("%s %d: Payload not found in the response value\n", __FUNCTION__, __LINE__);
        return -1;
    }

    *payload = rbusValue_GetString(value, NULL);
    if (*payload == NULL)
    {
        printf("%s %d: Failed to retrieve payload string\n", __FUNCTION__, __LINE__);
        return -1;
    }

    // Parse the payload and populate lanConfigs
    printf("%s %d: Received Payload: %s\n", __FUNCTION__, __LINE__, *payload);
    // Add your parsing logic here to populate lanConfigs and LanConfig_count

    return 0;
}

rbusError_t DhcpMgr_Event_SetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options)
{
    (void)handle;
    (void)options;
    printf("%s %d: Event Set Handler called\n",__FUNCTION__, __LINE__);
    const char* paramName = rbusProperty_GetName(property);
    rbusValue_t paramValue = rbusProperty_GetValue(property);
    const char* paramValueStr = rbusValue_GetString(paramValue, NULL);

    if (strcmp(paramName, "Device.DHCP.Server.v4.Event") == 0 && strcmp(paramValueStr, "start") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v4.Event' and value is 'start'\n", __FUNCTION__, __LINE__);
        EventHandler_MainFSM(DM_EVENT_STARTv4);
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v4.Event") == 0 && strcmp(paramValueStr, "stop") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v4.Event' and value is 'stop'\n", __FUNCTION__, __LINE__);
        EventHandler_MainFSM(DM_EVENT_STOPv4);
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v4.Event") == 0 && strcmp(paramValueStr, "restart") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v4.Event' and value is 'restart'\n", __FUNCTION__, __LINE__);
        EventHandler_MainFSM(DM_EVENT_RESTARTv4);
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v6.Event") == 0 && strcmp(paramValueStr, "start") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v6.Event' and value is 'start'\n", __FUNCTION__, __LINE__);
        EventHandler_MainFSM(DM_EVENT_STARTv6);
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v6.Event") == 0 && strcmp(paramValueStr, "stop") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v6.Event' and value is 'stop'\n", __FUNCTION__, __LINE__);
        EventHandler_MainFSM(DM_EVENT_STOPv6);
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v6.Event") == 0 && strcmp(paramValueStr, "restart") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v6.Event' and value is 'restart'\n", __FUNCTION__, __LINE__);
        EventHandler_MainFSM(DM_EVENT_RESTARTv6);
    }
    else
    {
        printf("%s %d: Parameter or value did not match\n", __FUNCTION__, __LINE__);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusDataElement_t DhcpMgrRbusDataElements[] = {
    {DHCPMGR_SERVERv4_EVENT,  RBUS_ELEMENT_TYPE_PROPERTY, {NULL, DhcpMgr_Event_SetHandler, NULL, NULL, NULL, NULL}},
    {DHCPMGR_SERVERv6_EVENT,  RBUS_ELEMENT_TYPE_PROPERTY, {NULL, DhcpMgr_Event_SetHandler, NULL, NULL, NULL, NULL}},
    {DHCPMGR_SERVER_READY,    RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, NULL, NULL}},
    {DHCPMGR_SERVER_STATE,    RBUS_ELEMENT_TYPE_EVENT, {NULL, NULL, NULL, NULL, NULL, NULL}}
};

int DhcpMgr_Rbus_Init()
{
    printf("%s %d: rbus init called\n",__FUNCTION__, __LINE__);
    int rc = -1;
    rc = rbus_open(&rbusHandle, componentName);
    if (rc != RBUS_ERROR_SUCCESS)
    {
        printf("DhcpManager_Rbus_Init rbus initialization failed\n");
        return rc;
    }

    
    // Register data elements
    rc = rbus_regDataElements(rbusHandle, ARRAY_SZ(DhcpMgrRbusDataElements), DhcpMgrRbusDataElements);
    //have to close the RBUS handle at exit

    return 0;
}

