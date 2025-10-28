#ifndef DHCPMGR_RBUS_APIS_H
#define DHCPMGR_RBUS_APIS_H

#include <rbus/rbus.h>
#include "sm_DhcpMgr.h"

#define DHCPMGR_SERVERv4_EVENT "Device.DHCP.Server.v4.Event"
#define DHCPMGR_SERVERv6_EVENT "Device.DHCP.Server.v6.Event"
#define DHCPMGR_SERVER_READY "Device.DHCP.Server.StateReady"
#define DHCPMGR_SERVER_STATE "Device.DHCP.Server.State"
#define LAN_DHCP_CONFIG "Device.LanManager.DhcpConfig"

int DhcpMgr_Rbus_Init();
int dhcp_server_signal_state_machine_ready();
int dhcp_server_publish_state(DHCPS_State state);
int rbus_GetLanDHCPConfig(const char** payload);
rbusError_t DhcpMgr_Event_SetHandler(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* options);


#endif