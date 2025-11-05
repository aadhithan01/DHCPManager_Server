// dhcp_server_v6_apis.h

#ifndef DHCPV6_SERVER_H
#define DHCPV6_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*dhcpv6 server type*/
typedef enum {
    DHCPV6_SERVER_TYPE_STATEFUL = 1,
    DHCPV6_SERVER_TYPE_STATELESS,    
}server_type_t;

#define INTERFACE_NAME_LENGTH 64
#define IPV6_ADDRESS_LENGTH 128
#define DNS_SERVER_LENGTH 64
#define PREFIX_LENGTH 64

// IPv6 address pool structure
typedef struct {
    char startAddress[IPV6_ADDRESS_LENGTH];  // Start of the IPv6 address pool
    char endAddress[IPV6_ADDRESS_LENGTH];    // End of the IPv6 address pool
} IPv6AddressPool;

// RDNSS entry
typedef struct {
    char ipv6_rdns_server[DNS_SERVER_LENGTH];  // IPv6 address of the rDNS server for this interface 
    uint32_t lifetime;                         // RDNSS lifetime
} IPv6RDNSS;

//Speedboost parameters
typedef struct 
{
    uint32_t adv_pvd_enable;
    uint32_t adv_pvd_hflag;
    uint32_t adv_pvd_delay;
    uint32_t adv_pvd_seqNum;  
    char adv_pvd_fqdn[DNS_SERVER_LENGTH];
} IPV6SpeedBoostParams;

//RA prefix
typedef struct
{ 
    char prefix[IPV6_ADDRESS_LENGTH];
    uint32_t ra_preferred_lft;
    uint32_t ra_valid_lft;
}Ipv6RaNdPrefix;

// Router Advertisement parameters
typedef struct {
    bool enable_ra;                  // Enable or disable Router Advertisement
    int ra_interval;                 // Router Advertisement interval (in seconds)
    char ra_hop_limit;               // Router Advertisement hop limit
    char ra_managed_flag;            // Router Advertisement managed address configuration flag
    char ra_other_flag;              // Router Advertisement other configuration flag
    uint16_t ra_router_lifetime;     // Router Advertisement router lifetime (in seconds) 
    Ipv6RaNdPrefix raPrefix[2];      // Prefix entries
    IPV6SpeedBoostParams *sBoost;    // SpeedBoost entries
    IPv6RDNSS* rdnss;                // RDNSS entries (dynamically allocated)
    size_t num_rdnss;                // Number of RDNSS entries
    size_t num_prefix;               // Number of prefix enteries
} IPv6RouterAdvertisementParams;

// DHCPv6 option structure
typedef struct {
    uint16_t option_code;  // Option code
    uint16_t option_len;   // Option length
    uint8_t* option_data;  // Option data
} Dhcpv6Option;

// DHCPv6 server interface configuration
typedef struct {
    char interface_name[INTERFACE_NAME_LENGTH];                       // Interface name
    int pool_num;                                                     // Nuber of pool
    int server_type;                                                  // Type of server
    int iana_enable;                                                  // IANA ENABLE to add the pool address on this interface
    bool enable_dhcp;                                                 // Enable or disable DHCPv6 server on this interface
    uint8_t ipv6_prefix_length;                                       // IPv6 prefix length
    IPv6AddressPool ipv6_address_pool;                                // IPv6 address pool for dynamic allocation on this interface
    uint32_t lease_time;                                              // Lease time for DHCPv6 assignments on this interface (in seconds)
    uint32_t renew_time;                                              // Time for clients to attempt to renew their lease on this interface (T1 time, in seconds)
    uint32_t rebind_time;                                             // Time for clients to attempt to rebind their lease on this interface (T2 time, in seconds)
    uint32_t valid_lifetime;                                          // Valid lifetime for assigned IPv6 addresses on this interface (in seconds)
    uint32_t preferred_lifetime;                                      // Preferred lifetime for assigned IPv6 addresses on this interface (in seconds)
    IPv6RouterAdvertisementParams ra_params;                          // Router Advertisement parameters for this interface
    Dhcpv6Option *options;                                            // Array of DHCPv6 options specific to this interface
    size_t num_options;                                               // Number of DHCPv6 options specific to this interface
    int log_level;                                                    // Log level for this interface
    bool rapid_enable;                                                // Enable or disable the Rapid Commit option for this interface
} IPv6Dhcpv6InterfaceConfig;

// DHCPv6 server configuration
typedef struct {
    IPv6Dhcpv6InterfaceConfig* interfaces;                           // Array of interface configurations
    size_t num_interfaces;                                           // Number of interfaces
} IPv6Dhcpv6ServerConfig;


// Function to create and initialize a DHCPv6 server
int dhcpv6_server_create(const IPv6Dhcpv6ServerConfig* config, server_type_t server_type);

// Function to start the DHCPv6 server
void dhcpv6_server_start(char* server,server_type_t server_type);

// Function to stop the DHCPv6 server
void dhcpv6_server_stop(char* server,server_type_t server_type);

#endif // DHCPV6_SERVER_H

