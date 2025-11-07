#include "sm_DhcpMgr.h"
#include "dhcpmgr_rbus_apis.h"
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

static DHCPS_State mainState = DHCPS_STATE_IDLE;

int PrepareConfigv4s(void *payload);
int Startv4s(void *payload);
int Stopv4s();

DhcpManagerEvent PrepareConfigv6s();
DhcpManagerEvent Startv6s();
DhcpManagerEvent Stopv6s();

GlobalDhcpConfig sGlbDhcpCfg;
DhcpInterfaceConfig **ppDhcpCfgs;
static bool Dns_only = false;

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
/*
    // -------------------- Start/Restart v6 --------------------
    { DHCPS_STATE_IDLE, EVENT_CONFIGUREv6, DHCPS_STATE_PREPARINGv6, PrepareConfigv6s },
    { DHCPS_STATE_PREPARINGv6, EVENT_CONFIG_CHANGEDv6, DHCPS_STATE_STARTINGv6, Startv6s },
    { DHCPS_STATE_PREPARINGv6, EVENT_CONFIG_SAMEv6, DHCPS_STATE_IDLE, NULL },
    { DHCPS_STATE_STARTINGv6, EVENT_STARTEDv6, DHCPS_STATE_IDLE, NULL },

    // -------------------- Stop v6 --------------------
    { DHCPS_STATE_IDLE, EVENT_STOPv6, DHCPS_STATE_STOPPINGv6, Stopv6s },
    { DHCPS_STATE_STOPPINGv6, EVENT_STOPPEDv6, DHCPS_STATE_IDLE, NULL }, */

};

/* Post a dispatch event to the central dispatch queue opened by main */
void PostEvent(DhcpManagerEvent evt) {
    /* Post DhcpManagerEvent to the FSM queue (mq_fsm).
     * Previously this sent events to mq_dispatch which is the same
     * queue FSMThread reads from; that produced a feedback loop
     * (dispatch -> post -> dispatch...). Use mq_fsm so the
     * FSM_Dispatch_Thread receives DhcpManagerEvent values.
     */ 
    extern mqd_t mq_fsm;
    if (mq_fsm == (mqd_t)-1) {
        fprintf(stderr, "PostEvent: FSM MQ not initialized\n");
        return;
    }
    if (mq_send(mq_fsm, (const char *)&evt, sizeof(evt), 0) == -1) {
        perror("PostEvent: mq_send");
    } else {
        printf("PostEvent: Event %d sent to FSM message queue %s\n", evt, SM_MQ_NAME);
    }
}

//THIS FUNCTION IS USED TO GET EVENT NAME STRING FROM ENUM VALUE
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

int PrepareConfigv4s(void *payload) {
    (void)payload;
    char *dnsonly = Dns_only ? "true" : NULL;
    memset(&sGlbDhcpCfg, 0, sizeof(GlobalDhcpConfig));
    int LanConfig_count = 0;
    char dhcpOptions[1024] = {0};
    DhcpPayload lanConfigs[MAX_IFACE_COUNT];
    bool serverInitstate = false;
    dhcp_server_publish_state(DHCPS_STATE_PREPARINGv4);

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
    serverInitstate = dhcpServerInit(&sGlbDhcpCfg, ppDhcpCfgs, LanConfig_count);
    if (!serverInitstate) {
        printf("%s:%d DHCP Server Initialization failed\n", __FUNCTION__, __LINE__);
        return -1;
    }
    else 
    {
        if (Dns_only)
        {
            dns_only(); //stub function call to mask the interface in dnsmasq.conf 
            Dns_only = false;
        }
        printf("%s:%d DHCP Server Initialization successful\n", __FUNCTION__, __LINE__);
        PostEvent(EVENT_CONFIG_CHANGEDv4);
    }
    return 0;
}

int Startv4s(void *payload) {
    (void)payload;
    bool serverStartState = false;
    dhcp_server_publish_state(DHCPS_STATE_STARTINGv4);
    serverStartState =  dhcpServerStart(&sGlbDhcpCfg);
    if (!serverStartState) {
        printf("%s:%d DHCP Server Start failed,Moving back to Idle state\n", __FUNCTION__, __LINE__);
        mainState = DHCPS_STATE_IDLE;
        dhcp_server_publish_state(DHCPS_STATE_IDLE);
        return -1;
    }
    else 
    {
        printf("%s:%d DHCP Server Started successfully\n", __FUNCTION__, __LINE__);
    PostEvent(EVENT_STARTEDv4);
        dhcp_server_publish_state(DHCPS_STATE_IDLE);
    }
    return 0;
}

int Stopv4s() {
    dhcp_server_publish_state(DHCPS_STATE_STOPPINGv4);
    dhcpServerStop(NULL, 0);
    Dns_only = true;
    printf("%s:%d DHCP Server Stopped successfully\n", __FUNCTION__, __LINE__);
    PostEvent(EVENT_STOPPEDv4);
    dhcp_server_publish_state(DHCPS_STATE_IDLE);
    PostEvent(EVENT_CONFIGUREv4);
    return 0;
}

// FSM thread: dispatch events
void* FSM_Dispatch_Thread(void* arg) 
{
    (void)arg;
    mqd_t mq;
//    struct mq_attr attr;
    ssize_t n;
    DhcpManagerEvent mq_event;
    char *buf = NULL;

    /* Use the centrally opened FSM MQ descriptor (mq_fsm) */
    mq = mq_fsm;
    if (mq == (mqd_t)-1) {
        fprintf(stderr, "FSM_Dispatch_Thread: mq_fsm not initialized\n");
        return NULL;
    }

    /* use fixed message size for DhcpManagerEvent */
    size_t msgsize = sizeof(DhcpManagerEvent);
    buf = malloc(msgsize);
    if (!buf) {
        perror("FSM_Dispatch_Thread: malloc");
        return NULL;
    }

    printf("FSM_Dispatch_Thread: listening on FSM MQ %s (msgsize=%ld)\n", SM_MQ_NAME, (long)msgsize);

    while (1) {
        n = mq_receive(mq, buf, msgsize, NULL);
        if (n >= 0) {
            if (n >= (ssize_t)sizeof(mq_event)) {
                memcpy(&mq_event, buf, sizeof(mq_event));
                printf("FSM_Dispatch_Thread: received event %d from MQ\n", mq_event);
                DispatchDHCP_SM(mq_event, NULL);
            } else {
                fprintf(stderr, "FSM_Dispatch_Thread: received message too small (%ld bytes)\n", (long)n);
            }
        } else {
            if (errno == EINTR) continue;
            perror("FSM_Dispatch_Thread: mq_receive failed");
            break;
        }
    }

    free(buf);
    /* do not close mq_fsm here; main owns it */
    return NULL;
}

/* FSM thread: read DhcpManagerEvent values from the MQ and dispatch to the DHCP FSM */
void *FSMThread(void *arg)
{
    (void)arg;
    mqd_t mq;
//    struct mq_attr attr;
    ssize_t n;
    DhcpMgr_DispatchEvent evt;
    char *buf = NULL;

    /* Use the centrally opened dispatch MQ descriptor (mq_dispatch) */
    mq = mq_dispatch;
    if (mq == (mqd_t)-1) {
        fprintf(stderr, "FSMThread: mq_dispatch not initialized\n");
        return NULL;
    }

    size_t msgsize = sizeof(DhcpMgr_DispatchEvent);
    buf = malloc(msgsize);
    if (!buf) {
        perror("FSMThread: malloc");
        return NULL;
    }

    printf("FSMThread: listening on Dispatch MQ (msgsize=%ld)\n", (long)msgsize);

    while (1) {
        n = mq_receive(mq, buf, msgsize, NULL);
        if (n >= 0) 
        {
            if (n >= (ssize_t)sizeof(evt)) {
                memcpy(&evt, buf, sizeof(evt));
                printf("FSMThread: received dispatch event %d from MQ\n", evt);
                /* Map DhcpMgr_DispatchEvent -> DhcpManagerEvent for PostEvent */
                DhcpManagerEvent post_evt;
                switch (evt) {
                    case DM_EVENT_STARTv4:
                        post_evt = EVENT_CONFIGUREv4; /* start => configure/start flow */
                        break;
                    case DM_EVENT_RESTARTv4:
                        post_evt = EVENT_CONFIGUREv4; /* restart treated like configure */
                        break;
                    case DM_EVENT_CONF_CHANGEDv4:
                        post_evt = EVENT_CONFIG_CHANGEDv4;
                        break;
                    case DM_EVENT_STOPv4:
                        post_evt = EVENT_STOPv4;
                        break;
                    case DM_EVENT_STARTv6:
                        post_evt = EVENT_CONFIGUREv6;
                        break;
                    case DM_EVENT_RESTARTv6:
                        post_evt = EVENT_CONFIGUREv6;
                        break;
                    case DM_EVENT_CONF_CHANGEDv6:
                        post_evt = EVENT_CONFIG_CHANGEDv6;
                        break;
                    case DM_EVENT_STOPv6:
                        post_evt = EVENT_STOPv6;
                        break;
                    default:
                        fprintf(stderr, "FSMThread: unknown dispatch event %d\n", evt);
                        continue;
                }
                PostEvent(post_evt);
            } 
            else 
            {
                fprintf(stderr, "FSMThread: received message too small (%ld bytes)\n", (long)n);
            }
        } 
        else {
            if (errno == EINTR) continue;
            perror("FSMThread: mq_receive failed");
            break;
        }
    }

    free(buf);
    /* do not close mq_dispatch here; main owns it */
    return NULL;
}