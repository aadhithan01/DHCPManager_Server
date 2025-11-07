#include "dhcpmgr_rbus_apis.h"
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


#define  MAC_ADDR_SIZE 18
#define  ARRAY_SZ(x) (sizeof(x) / sizeof((x)[0]))
static rbusHandle_t rbusHandle;

typedef enum {
    DM_EVENT_STARTv4,
    DM_EVENT_RESTARTv4,
    DM_EVENT_CONF_CHANGEDv4,
    DM_EVENT_STOPv4,
    DM_EVENT_STARTv6,
    DM_EVENT_RESTARTv6,
    DM_EVENT_CONF_CHANGEDv6,
    DM_EVENT_STOPv6
} DhcpMgr_DispatchEvent;

char *componentName = "DHCPMANAGER";

rbusError_t DhcpMgr_Publish_Events(char *statestr,char * paramName,char * eventName)
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

    DhcpMgr_DispatchEvent mq_event = -1;

    if (strcmp(paramName, "Device.DHCP.Server.v4.Event") == 0 && strcmp(paramValueStr, "start") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v4.Event' and value is 'start'\n", __FUNCTION__, __LINE__);
        mq_event = DM_EVENT_STARTv4; /* map to DhcpManagerEvent */
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v4.Event") == 0 && strcmp(paramValueStr, "stop") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v4.Event' and value is 'stop'\n", __FUNCTION__, __LINE__);
        mq_event = DM_EVENT_STOPv4; /* map to DhcpManagerEvent */
    }
    else if (strcmp(paramName, "Device.DHCP.Server.v4.Event") == 0 && strcmp(paramValueStr, "restart") == 0)
    {
        printf("%s %d: Parameter is 'Device.DHCP.Server.v4.Event' and value is 'restart'\n", __FUNCTION__, __LINE__);
        mq_event = DM_EVENT_RESTARTv4; /* map to DhcpManagerEvent */
    }
    else
    {
        printf("%s %d: Parameter or value did not match\n", __FUNCTION__, __LINE__);
    }

    /* If we mapped an event, send it over the POSIX message queue to the state machine. */
    if (mq_event >= 0) {
        mqd_t mq = mq_open(MQ_NAME, O_WRONLY);
        if (mq == (mqd_t)-1) {
            /* queue not available - try to create it */
            if (errno == ENOENT) {
                struct mq_attr attr;
                attr.mq_flags = 0;
                attr.mq_maxmsg = 10;
                attr.mq_msgsize = sizeof(DhcpMgr_DispatchEvent);
                attr.mq_curmsgs = 0;

                mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0666, &attr);
                if (mq == (mqd_t)-1) {
                    perror("mq_open (create)");
                    printf("%s %d: Failed to create MQ '%s' to send event %d\n", __FUNCTION__, __LINE__, MQ_NAME, mq_event);
                    return RBUS_ERROR_BUS_ERROR;
                }
            } else {
                perror("mq_open");
                printf("%s %d: Failed to open MQ '%s' to send event %d (errno=%d)\n", __FUNCTION__, __LINE__, MQ_NAME, mq_event, errno);
                return RBUS_ERROR_BUS_ERROR;
            }
        }

        if (mq_send(mq, (const char*)&mq_event, sizeof(mq_event), 0) == -1) {
            int errsv = errno;
            perror("mq_send");
            printf("%s %d: mq_send failed with errno %d sending event %d to %s\n", __FUNCTION__, __LINE__, errsv, mq_event, MQ_NAME);
            mq_close(mq);
            return RBUS_ERROR_BUS_ERROR;
        }

        printf("%s %d: Sent event %d to MQ %s\n", __FUNCTION__, __LINE__, mq_event, MQ_NAME);
        mq_close(mq);
        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_SUCCESS;
}

rbusDataElement_t DhcpMgrRbusDataElements[] = {
    {DHCPMGR_SERVERv4_EVENT,  RBUS_ELEMENT_TYPE_PROPERTY, {NULL, DhcpMgr_Event_SetHandler, NULL, NULL, NULL, NULL}},
    {DHCPMGR_SERVERv6_EVENT,  RBUS_ELEMENT_TYPE_PROPERTY, {NULL, NULL, NULL, NULL, NULL, NULL}},
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

