#ifndef DHCPMGR_RBUS_APIS_H
#define DHCPMGR_RBUS_APIS_H

#include <rbus/rbus.h>

#define DHCPMGR_SERVERv4_EVENT "Device.DHCP.Server.v4.Event"
#define DHCPMGR_SERVERv6_EVENT "Device.DHCP.Server.v6.Event"
#define DHCPMGR_SERVER_READY "Device.DHCP.Server.StateReady"
#define DHCPMGR_SERVER_STATE "Device.DHCP.Server.State"
#define LAN_DHCP_CONFIG "Device.LanManager.DhcpConfig"

#define MQ_NAME "/lan_sm_queue"

int DhcpMgr_Rbus_Init();
rbusError_t DhcpMgr_Publish_Events(char *statestr,char * paramName,char * eventName);
int rbus_GetLanDHCPConfig(const char** payload);
rbusError_t DhcpMgr_Event_SetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options);
#endif