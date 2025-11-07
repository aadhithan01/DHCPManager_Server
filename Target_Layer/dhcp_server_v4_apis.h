// dhcp_server.h

#ifndef DHCP_SERVER_H
#define DHCP_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#define BUFF_LEN_2048  2048
#define BUFF_LEN_1024  1024
#define BUFF_LEN_512   512
#define BUFF_LEN_256   256
#define BUFF_LEN_128   128
#define BUFF_LEN_64    64
#define BUFF_LEN_32    32
#define BUFF_LEN_16    16
#define BUFF_LEN_8     8

#if 0 //As of now not used
// Define DHCP message types
typedef enum {
    DHCP_DISCOVER,
    DHCP_OFFER,
    DHCP_REQUEST,
    DHCP_ACK,
    DHCP_NAK,
    DHCP_RELEASE,
} DhcpMessageType;

// Define log levels
typedef enum {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
} LogLevel;

// DHCP client information
typedef struct {
    uint8_t  uiMacAddress[6];
    uint32_t uiIpAddress;
    // Add other relevant information
} DhcpClientInfo;
#endif
// DHCP address pool
typedef struct {
    char  cStartAddress [BUFF_LEN_32];
    char  cEndAddress   [BUFF_LEN_32];
    int   iDhcpTagNum;
} DhcpAddressPool;

// Redirection information
typedef struct {
    bool  bIsRedirectionEnabled;                      // Enable/disable redirection
    char  cRedirectionIp  [BUFF_LEN_32];              // IP address for redirection
    char  **ppRedirectionUrl; // Redirection URL
    int   iRedirectionUrlCount;
    // Add other redirection details if needed
} RedirectionInfo;

// Domain-specific address information
typedef struct {
    bool bIsDomainSpecificEnabled;    // Enable/disable domain-specific address
    char **ppDomainSpecificAddresses; // Domain-specific IP address
    int  iDomainSpecificAddressCount;
    // Add other domain-specific details if needed
} DomainSpecificAddressInfo;

// DHCP options
typedef struct {
    /*Vendor id will be like
      dhcp-option=vendor:PP203X,43,tag=123
      dhcp-option=vendor:HIXE12AWR,43,tag=123
      dhcp-option=tag:extender, option:time-offset,-28800 */
    char **ppVendorId;  //Vendor ID
    int    iVendorIdCount;
    /*Vendor class will be like
      dhcp-vendorclass=set:extender,WNXL11BWL*/
    char **ppVendorClass; //Vendor class
    int    iVendorClassCount;
    // Add other DHCP options as needed
} DhcpOptions;

// Command line arguments
typedef struct {
    char **ppCmdLineArgs;  // Array of command line arguments
    int iNumOfArgs;        // Number of command line arguments
} CommandLineArgs;

typedef struct {
    bool bDhcpNameServerEnabled;
    char cDhcpNameServerIp   [BUFF_LEN_1024];
}DhcpNameServer;

// Global DHCP server configuration
typedef struct {
    //LogLevel log_level;                        // Log level
    RedirectionInfo sRedirectInfo;             // Redirection information
    DomainSpecificAddressInfo sDomainSpecific; // Domain-specific address information
    DhcpOptions sDhcpOptions;                  // DHCP options
    CommandLineArgs sCmdArgs;                  // Command line arguments
    bool bBogusPriv;                           // Flag for bogus-priv
    bool bExpandHosts;                         // Flag for expand-hosts
    // Add other global configurations as needed
    char cDhcpLeaseFile [BUFF_LEN_64];     // DHCP lease file
    char cDhcpHostsFile [BUFF_LEN_64];     // DHCP hosts file
    char cDhcpOptsFile  [BUFF_LEN_64];     // DHCP options file
    char cDomain        [BUFF_LEN_64];     // Domain
    bool bDomainNeeded;                    // Flag to indicate if domain is needed
    char cResolvCfgFile [BUFF_LEN_64];     //pointer to Resolve config file name
} GlobalDhcpConfig;

// DHCP server configuration for an interface
typedef struct {
    bool bIsDhcpEnabled;  // Enable/disable DHCP for this interface
    char cSubnetMask    [BUFF_LEN_32];
    char cGatewayName   [BUFF_LEN_16];
    char cLeaseDuration [BUFF_LEN_16];
    char cInputStr      [BUFF_LEN_16]; //dns_only
    //const char cGatewayIp     [BUFF_LEN_32];
    DhcpNameServer  sDhcpNameServer;
    DhcpAddressPool sAddressPool;    // IP address pool
    // Add other interface-specific configurations as needed
} DhcpInterfaceConfig;

// DHCP server initialization with configurations for multiple interfaces
bool dhcpServerInit(const GlobalDhcpConfig *pGlobalConfig, DhcpInterfaceConfig **ppDhcpConfigs, int iNumOfInterfaces);

#if 0 // TBD
// DHCP server cleanup
void dhcp_server_cleanup();
#endif
// Start DHCP servers for multiple interfaces
bool dhcpServerStart(const GlobalDhcpConfig *pGlobalConfig);

// Stop DHCP server(s) for a specific interface or all interfaces
//void dhcp_servers_stop(size_t interface_index);
bool dhcpServerStop(char **ppDhcpInterfaces, int iNumOfInterfaces);

//To find the interfaces are present in the dhcp config file or not
bool isDhcpConfHasInterface(void);

#if 0 // TBD
// Handle DHCP messages
void handle_dhcp_message(DhcpMessageType type, const DhcpClientInfo *client_info);
#endif
#endif // DHCP_SERVER_H

