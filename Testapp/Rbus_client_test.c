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

static rbusHandle_t handle = NULL;
static volatile int keepRunning = 1;

static void intHandler(int dummy)
{
    (void)dummy;
    keepRunning = 0;
}

/* Helper: set DHCP v4 event property to a string value (e.g., "start") */
static int set_dhcp_v4_event(const char *value)
{
    rbusError_t rc;

    rc = rbus_setStr(handle, DHCPMGR_SERVERv4_EVENT, value);
    if (rc != RBUS_ERROR_SUCCESS) {
        printf("rbus_setStr failed: %d\n", rc);
        return -1;
    }
    printf("Set %s = '%s'\n", DHCPMGR_SERVERv4_EVENT, value);
    return 0;
}

/* event handler for subscribed events */
static void event_handler(rbusHandle_t h, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    (void)subscription;
    (void)h;
    if (!event) return;

    printf("Received event: %s\n", event->name);

    if (strcmp(event->name, DHCPMGR_SERVER_STATE) == 0 || strcmp(event->name, DHCPMGR_SERVER_READY) == 0) {
        if (event->data) {
            rbusObject_t obj = (rbusObject_t)event->data;
            rbusValue_t v = NULL;

            /* Try the several keys used by DhcpMgr */
            v = rbusObject_GetValue(obj, "DHCP_server_state");
            if (!v) v = rbusObject_GetValue(obj, "DHCP_Server_state");
            if (!v) v = rbusObject_GetValue(obj, "DHCP_server_v4");
            if (!v) v = rbusObject_GetValue(obj, "DHCP_server_v6");

            if (v) {
                const char* s = rbusValue_GetString(v, NULL);
                if (s) {
                    printf("State payload: %s\n", s);
                    if (strcmp(s, "Ready") == 0) {
                        printf("State is Ready -> setting v4 event to 'start'\n");
                        set_dhcp_v4_event("start");
                    }
                }
            } else {
                /* No recognized key. If it's the READY event name, set anyway */
                if (strcmp(event->name, DHCPMGR_SERVER_READY) == 0) {
                    printf("READY event received (unknown payload) -> setting v4 event to 'start'\n");
                    set_dhcp_v4_event("start");
                }
            }
        } else {
            /* No data; if ready event name matches, set directly */
            if (strcmp(event->name, DHCPMGR_SERVER_READY) == 0) {
                printf("Received ready notification (no data) -> setting v4 event to 'start'\n");
                set_dhcp_v4_event("start");
            }
        }
    }
}

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    rbusError_t rc;
    char command[256];

    signal(SIGINT, intHandler);

    rc = rbus_open(&handle, "Rbus_client_test_consumer");
    if (rc != RBUS_ERROR_SUCCESS) {
        printf("rbus_open failed: %d\n", rc);
        return -1;
    }

    /* Subscribe to both State property and Ready property when provider publishes them as events */
    rc = rbusEvent_Subscribe(handle, DHCPMGR_SERVER_STATE, event_handler, NULL, 5);
    if (rc != RBUS_ERROR_SUCCESS) {
        printf("Subscribe to %s failed: %d\n", DHCPMGR_SERVER_STATE, rc);
    } else {
        printf("Subscribed to %s\n", DHCPMGR_SERVER_STATE);
    }

    rc = rbusEvent_Subscribe(handle, DHCPMGR_SERVER_READY, event_handler, NULL, 5);
    if (rc != RBUS_ERROR_SUCCESS) {
        printf("Subscribe to %s failed: %d\n", DHCPMGR_SERVER_READY, rc);
    } else {
        printf("Subscribed to %s\n", DHCPMGR_SERVER_READY);
    }

    /* Command-line reader loop */
    printf("Rbus client listening for DHCP server state events...\n");
    while (keepRunning) {
        printf("Enter command (start/stop/quit): ");
        if (fgets(command, sizeof(command), stdin) != NULL) {
            /* Remove trailing newline */
            command[strcspn(command, "\n")] = '\0';

            if (strcmp(command, "quit") == 0) {
                printf("Quitting program...\n");
                break;
            } else if (strcmp(command, "start") == 0) {
                printf("Setting v4 event to 'start'\n");
                set_dhcp_v4_event("start");
            } else if (strcmp(command, "stop") == 0) {
                printf("Setting v4 event to 'stop'\n");
                set_dhcp_v4_event("stop");
            } else if (strcmp(command, "restart") == 0) {
                printf("Setting v4 event to 'restart'\n");
                set_dhcp_v4_event("restart");
            } else {
                printf("Unknown command: %s\n", command);
            }
        }
    }

    printf("Shutting down: unsubscribing and closing rbus\n");
    rbusEvent_Unsubscribe(handle, DHCPMGR_SERVER_STATE);
    rbusEvent_Unsubscribe(handle, DHCPMGR_SERVER_READY);
    rbus_close(handle);
    return 0;
}
