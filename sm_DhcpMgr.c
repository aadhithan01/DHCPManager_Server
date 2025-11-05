#include "sm_DhcpMgr.h"
#include "dhcpmgr_rbus_apis.h"
#include <unistd.h>

#define SERVER_BIN "/usr/sbin/dibbler-server"
static DHCPS_State mainState = DHCPS_STATE_IDLE;

int PrepareConfigv4s(void *payload);
int Startv4s(void *payload);
int Stopv4s();

int PrepareConfigv6s();
int Startv6s();
int Stopv6s();

GlobalDhcpConfig sGlbDhcpCfg;
DhcpInterfaceConfig **ppDhcpCfgs;

const char* DhcpManagerEvent_Names[]={
    "EVENT_CONFIGUREv4",
    "EVENT_CONFIG_CHANGEDv4",
    "EVENT_CONFIG_SAMEv4",
    "EVENT_STARTEDv4",
    "EVENT_STOPv4",
    "EVENT_STOPPEDv4",
    "EVENT_CONFIGUREv6",
    "EVENT_CONFIG_CHANGEDv6",
    "EVENT_CONFIG_SAMEv6",
    "EVENT_STARTEDv6",
    "EVENT_STOPv6",
    "EVENT_STOPPEDv6"
};

DHCPS_SM_Mapping gSM_StateObj[] = {

    // -------------------- Start/Restart v4 --------------------
    { DHCPS_STATE_IDLE, EVENT_CONFIGUREv4, DHCPS_STATE_PREPARINGv4, PrepareConfigv4s },
    { DHCPS_STATE_PREPARINGv4, EVENT_CONFIG_CHANGEDv4, DHCPS_STATE_STARTINGv4, Startv4s },
    { DHCPS_STATE_PREPARINGv4, EVENT_CONFIG_SAMEv4, DHCPS_STATE_IDLE, NULL },
    { DHCPS_STATE_STARTINGv4, EVENT_STARTEDv4, DHCPS_STATE_IDLE, NULL },

    // -------------------- Stop v4 --------------------
    { DHCPS_STATE_IDLE, EVENT_STOPv4, DHCPS_STATE_STOPPINGv4, Stopv4s },
    { DHCPS_STATE_STOPPINGv4, EVENT_STOPPEDv4, DHCPS_STATE_IDLE, NULL },

    // -------------------- Start/Restart v6 --------------------
    { DHCPS_STATE_IDLE, EVENT_CONFIGUREv6, DHCPS_STATE_PREPARINGv6, PrepareConfigv6s },
    { DHCPS_STATE_PREPARINGv6, EVENT_CONFIG_CHANGEDv6, DHCPS_STATE_STARTINGv6, Startv6s },
    { DHCPS_STATE_PREPARINGv6, EVENT_CONFIG_SAMEv6, DHCPS_STATE_IDLE, NULL },
    { DHCPS_STATE_STARTINGv6, EVENT_STARTEDv6, DHCPS_STATE_IDLE, NULL },

    // -------------------- Stop v6 --------------------
    { DHCPS_STATE_IDLE, EVENT_STOPv6, DHCPS_STATE_STOPPINGv6, Stopv6s },
    { DHCPS_STATE_STOPPINGv6, EVENT_STOPPEDv6, DHCPS_STATE_IDLE, NULL },
};

const char* GetEventName(DhcpManagerEvent evt) {
    if (evt >= 0 && evt < sizeof(DhcpManagerEvent_Names)/sizeof(DhcpManagerEvent_Names[0])) {
        return DhcpManagerEvent_Names[evt];
    }
    return "UNKNOWN_EVENT";
}

void DispatchDHCP_SM(DhcpManagerEvent evt, void *payload) {
    printf("[FSM] DispatchDHCP_SM called with event %s\n", GetEventName(evt));
    // Simple stub: find matching state and call action
    for (int i = 0; i < (int)(sizeof(gSM_StateObj)/sizeof(gSM_StateObj[0])); i++) {
        if (gSM_StateObj[i].curState == (int)mainState && gSM_StateObj[i].event == (int)evt) {
            if (gSM_StateObj[i].action) {
                if(gSM_StateObj[i].action(payload) == 0) {
                    printf("[FSM] Action for event %s executed successfully.\n", GetEventName(evt));
                } else {
                    printf("[FSM] Action for event %s failed.\n", GetEventName(evt));
                    return; // Do not change state if action fails
                }
            }
            mainState = gSM_StateObj[i].nextState;
            break;
        }
    }
}

int PrepareConfigv6s(void *payload) {
    bool statefull_enabled = false;
    bool stateless_enabled = false;
    (void)payload;
    int LanConfig_count = 0;
    DhcpPayload lanConfigs[MAX_IFACE_COUNT];
    GetLanDHCPConfig(lanConfigs, &LanConfig_count);
    if (check_ipv6_received(lanConfigs, LanConfig_count, &statefull_enabled, &stateless_enabled) != 0) {
        printf("%s:%d No brlan0 interface with DHCPv6 enabled. Skipping DHCPv6 configuration.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    else
    {
        if (statefull_enabled)
        {
            printf("%s:%d Stateful DHCPv6 is enabled on brlan0.\n", __FUNCTION__, __LINE__);
            //As of now its hardcoded to call create_dhcpsv6_config function for brlan0 only
            int ret = create_dhcpsv6_config(lanConfigs);
            if (ret != 0)
            {
                printf("%s:%d Failed to create DHCPv6 configuration.\n", __FUNCTION__, __LINE__);
                return -1;
            }
        }
    }
    printf("%s:%d Preparing DHCPv6 configuration (stub)\n", __FUNCTION__, __LINE__);
    return 0;
}

int Startv6s(void *payload) {
    (void)payload;
    dhcpv6_server_start(SERVER_BIN, DHCPV6_SERVER_TYPE_STATEFUL);
    printf("%s:%d Starting DHCPv6 server\n", __FUNCTION__, __LINE__);
    return 0;
}

int Stopv6s() {
    // Assuming we have a function to stop the DHCPv6 server
    dhcpv6_server_stop(SERVER_BIN, DHCPV6_SERVER_TYPE_STATEFUL); // Using stateful to stop the server as a stub
    printf("%s:%d Stopping DHCPv6 server\n", __FUNCTION__, __LINE__);
    return 0;
}

int PrepareConfigv4s(void *payload) {
    char *dnsonly = (char *)payload;
    memset(&sGlbDhcpCfg, 0, sizeof(GlobalDhcpConfig));
    int LanConfig_count = 0;
    char dhcpOptions[1024] = {0};
    DhcpPayload lanConfigs[MAX_IFACE_COUNT];

    // Stub function for fetching lanConfigs and LanConfig_count from somewhere
    GetLanDHCPConfig(lanConfigs, &LanConfig_count);
    AllocateDhcpInterfaceConfig(&ppDhcpCfgs, LanConfig_count);
    Add_inf_to_dhcp_config(lanConfigs, MAX_IFACE_COUNT, ppDhcpCfgs, LanConfig_count);
    Construct_dhcp_configurationv4(dhcpOptions, dnsonly);
    sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs = (char **)malloc(sizeof(char *));
    if (sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs == NULL)
    {
        printf("%s:%d Memory allocation for ppCmdLineArgs failed\n", __FUNCTION__, __LINE__);
        return -1;
    }
    sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs[0] = (char *)malloc(1024 * sizeof(char));
    if (sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs[0] == NULL)
    {
        printf("%s:%d Memory allocation for ppCmdLineArgs[0] failed\n", __FUNCTION__, __LINE__);
        free(sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs);
        sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs = NULL;
        return -1;
    }
    memset(sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs[0], 0, 1024);
    snprintf(sGlbDhcpCfg.sCmdArgs.ppCmdLineArgs[0], 1024, "%s", dhcpOptions);
    sGlbDhcpCfg.sCmdArgs.iNumOfArgs = 1;
    dhcpServerInit(&sGlbDhcpCfg, ppDhcpCfgs, LanConfig_count);
    return 0;
}

int Startv4s(void *payload) {
    (void)payload;
    dhcpServerStart(&sGlbDhcpCfg);
    return 0;
}

int Stopv4s() {
    dhcpServerStop(NULL, 0);
    DispatchDHCP_SM(EVENT_CONFIGUREv4, "true");
    dns_only(); //stub function to write dns_only configuration
    DispatchDHCP_SM(EVENT_CONFIG_CHANGEDv4, NULL);
    return 0;
}

static int event_Dispatcher_startv4()
{
    DispatchDHCP_SM(EVENT_CONFIGUREv4, NULL);
    if(mainState == DHCPS_STATE_PREPARINGv4)
    {
        dhcp_server_publish_state(DHCPS_STATE_PREPARINGv4);
        DispatchDHCP_SM(EVENT_CONFIG_CHANGEDv4, NULL);
    }
    else
    {
        printf("Failed to start DHCPv4 server, invalid state transition\n");
        return -1;
    }
    if (mainState == DHCPS_STATE_STARTINGv4)
    {
        dhcp_server_publish_state(DHCPS_STATE_STARTINGv4);
        DispatchDHCP_SM(EVENT_STARTEDv4, NULL);
    }
    else
    {
        printf("Failed to start DHCPv4 server, invalid state transition\n");
        return -1;
    }
    if (mainState != DHCPS_STATE_IDLE)
    {
        printf("Failed to start DHCPv4 server, not in IDLE state\n");
        return -1;
    }
    else
    {
        dhcp_server_publish_state(DHCPS_STATE_IDLE);
    }
    return 0;
}

static int event_Dispatcher_startv6()
{
    DispatchDHCP_SM(EVENT_CONFIGUREv6, NULL);
    if(mainState == DHCPS_STATE_PREPARINGv6)
    {
        dhcp_server_publish_state(DHCPS_STATE_PREPARINGv6);
        DispatchDHCP_SM(EVENT_CONFIG_CHANGEDv6, NULL);
    }
    else
    {
        printf("Failed to start DHCPv6 server, invalid state transition\n");
        return -1;
    }
    if (mainState == DHCPS_STATE_STARTINGv6)
    {
        dhcp_server_publish_state(DHCPS_STATE_STARTINGv6);
        DispatchDHCP_SM(EVENT_STARTEDv6, NULL);
    }
    else
    {
        printf("Failed to start DHCPv6 server, invalid state transition\n");
        return -1;
    }
    if (mainState != DHCPS_STATE_IDLE)
    {
        printf("Failed to start DHCPv6 server, not in IDLE state\n");
        return -1;
    }
    else
    {
        dhcp_server_publish_state(DHCPS_STATE_IDLE);
    }
    return 0;
}

static int event_Dispatcher_stopv6()
{
    DispatchDHCP_SM(EVENT_STOPv6, NULL);
    if(mainState == DHCPS_STATE_STOPPINGv6)
    {
        dhcp_server_publish_state(DHCPS_STATE_STOPPINGv6);
        DispatchDHCP_SM(EVENT_STOPPEDv6, NULL);
    }
    else
    {
        printf("Failed to stop DHCPv6 server, invalid state transition\n");
        return -1;
    }
    if (mainState != DHCPS_STATE_IDLE)
    {
        printf("Failed to stop DHCPv6 server, not in IDLE state\n");
        return -1;
    }
    else
    {
        dhcp_server_publish_state(DHCPS_STATE_IDLE);
    }
    return 0;
}


static int event_Dispatcher_stopv4()
{
    DispatchDHCP_SM(EVENT_STOPv4, NULL);
    if(mainState == DHCPS_STATE_STOPPINGv4)
    {
        dhcp_server_publish_state(DHCPS_STATE_STOPPINGv4);
        DispatchDHCP_SM(EVENT_STOPPEDv4, NULL);
    }
    else
    {
        printf("Failed to stop DHCPv4 server, invalid state transition\n");
        return -1;
    }
    if (mainState != DHCPS_STATE_IDLE)
    {
        printf("Failed to stop DHCPv4 server, not in IDLE state\n");
        return -1;
    }
    else
    {
        dhcp_server_publish_state(DHCPS_STATE_IDLE);
    }
    return 0;
}

int EventHandler_MainFSM(DhcpMgr_DispatchEvent event)
{    
    int retval = 0;
    switch (event)
    {
    case DM_EVENT_STARTv4:
        retval = event_Dispatcher_startv4();
        break;
    case DM_EVENT_STOPv4:
        retval = event_Dispatcher_stopv4();
        break;
    case DM_EVENT_RESTARTv4:
        retval = event_Dispatcher_stopv4();
        if(retval != 0)
        {   
            break;
        }
        retval = event_Dispatcher_startv4();
        break;
    case DM_EVENT_CONF_CHANGEDv4:
//        Conf change is not required to handled
        break;
    case DM_EVENT_STARTv6:
        retval = event_Dispatcher_startv6();
        break;
    case DM_EVENT_STOPv6:
        retval = event_Dispatcher_stopv6();
        break;
    case DM_EVENT_RESTARTv6:
        retval = event_Dispatcher_stopv6();
        if(retval != 0)
        {
            break;
        }
        retval = event_Dispatcher_startv6();
        break;
    case DM_EVENT_CONF_CHANGEDv6:
//        ConfChangedv6s();
        break;
    default:
        break;
    }
    return retval;
}

int main() {
    printf("DHCP Manager starting...\n");
    DhcpMgr_Rbus_Init(); // Initialize rbus
    dhcp_server_signal_state_machine_ready(); // Signal that the state machine is ready
    dhcp_server_publish_state(DHCPS_STATE_IDLE); // Publish initial state

    while(1)
    {
        // Keep the main thread alive to listen for rbus events
        usleep(100000); // Sleep for 100ms
    }

    return 0;
}