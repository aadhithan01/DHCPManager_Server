//Function to Create RA config
void create_zebra_config()
{
    char server[64] = {0};
    //Copy server name to server and send to target layer api
    strncpy(server, "zebra",sizeof(server)-1 );

    int deviceMode = Get_Device_Mode();
    bool extender_enabled = IsExtenderEnabled();

    //No need of running zebra for radv in extender mode
    if (( DEVICE_MODE_EXTENDER == deviceMode ) && (extender_enabled == true))
    {
        fprintf(stderr, "Device is EXT mode , no need of running zebra for radv\n");
        return;
    }

    char aBridgeMode[8];
    syscfg_get(NULL, "bridge_mode", aBridgeMode, sizeof(aBridgeMode));

    if ((!strcmp(aBridgeMode, "0")) && (!g_lan_ready)) {
        fprintf(stderr, "%s: LAN is not ready !\n", __FUNCTION__);
        return;
    }
    
    //Check if wanfailover is enabled
    bool wanFailoverEnabled = isWanFailoverEnabled();
    if(wanFailoverEnabled == true)
    {
        if ( 0 == checkIfULAEnabled()) 
        {
            gIpv6AddrAssignment=ULA_IPV6;
        }
        else
        {
            gIpv6AddrAssignment=GLOBAL_IPV6;
        }
        checkIfModeIsSwitched();
    }

    if(populateRAConfig() != 0)
    {
        LanManagerError(("populateRAConfig failed"));
        return;
    }
    else
    {
        //Target Layer API to stop the zebra process
        dhcpv6_server_stop(server,DHCPV6_SERVER_TYPE_STATELESS);

        //Target Layer API to start the zebra process
        dhcpv6_server_start(server, DHCPV6_SERVER_TYPE_STATELESS);

    }
}

int populateRAConfig()
{
    char rtmod[16], ra_en[16], dh6s_en[16];
    char ra_interval[8] = {0};
    char name_servs[1024] = {0};
    char dnssl[2560] = {0};
    char dnssl_lft[16];
    unsigned int dnssllft = 0;
    char prefix[64], lan_addr[64];
    char preferred_lft[16], valid_lft[16];
    unsigned int rdnsslft = 3 * RA_INTERVAL; // as defined in RFC
    int i = 0;
    char m_flag[16], o_flag[16], ra_mtu[16];
    char rec[256], val[512];
    char buf[6];
    FILE *responsefd = NULL;
    char *networkResponse = "/var/tmp/networkresponse.txt";
    int iresCode = 0;
    char responseCode[10];
    int inCaptivePortal = 0,inWifiCp=0;
    int  StaticDNSServersEnabled = 0;
    char wan_st[16] = {0};
    int nopt, j = 0; /*RDKB-12965 & CID:-34147*/
    char lan_if[IFNAMSIZ];
    char *start, *tok, *sp;
    memset(prefix,0,sizeof(prefix));
    char default_wan_interface[64] = {0};
    char wan_interface[64] = {0};
    int inRfCaptivePortal = 0;
    char rfCpMode[6] = {0};
    char rfCpEnable[6] = {0};
    bool wanFailoverEnabled = isWanFailoverEnabled();
    char last_broadcasted_prefix[64] ;
    char cmd[100];
    char lastBroadcastCmd[100];
    char out[100];
    char interface_name[32] = {0};
    char *token = NULL; 
    char *pt;
    char pref_rx[16];
    int ifCount = 1; //initialize count 1 for other ipv6 enabled interfaces
    int pref_len = 0;
    errno_t  rc = -1;

    IPv6Dhcpv6InterfaceConfig  interface[10];

    //Initialize the interface structure
    memset(interface, 0, sizeof(interface));

    IPv6Dhcpv6ServerConfig config;
    
    config.num_interfaces = 0;

    char l_cSecWebUI_Enabled[8] = {0};
    syscfg_get(NULL, "SecureWebUI_Enable", l_cSecWebUI_Enabled, sizeof(l_cSecWebUI_Enabled));
    if (!strncmp(l_cSecWebUI_Enabled, "true", 4))	
    {
        syscfg_set_commit("dhcpv6spool00", "X_RDKCENTRAL_COM_DNSServersEnabled", "1");
         
        FILE *fptr = NULL;
        char loc_domain[128] = {0};
        char loc_ip6[256] = {0};
        commonSyseventGet("lan_ipaddr_v6", loc_ip6, sizeof(loc_ip6));
        syscfg_get(NULL, "SecureWebUI_LocalFqdn", loc_domain, sizeof(loc_domain));
        FILE *ptr;
        char buff[10];
        if ((ptr=v_secure_popen("r", "grep %s /etc/hosts",loc_ip6))!=NULL)
        if (NULL != ptr)
        {
            if (NULL == fgets(buff, 9, ptr)) {
                fptr =fopen("/etc/hosts", "a");
                if (fptr != NULL)
                {
                    if ( loc_ip6 != NULL)
                    {
                        if (loc_domain != NULL)
                        {
                            fprintf(fptr, "%s      %s\n",loc_ip6,loc_domain);
                        }
                    }
                    fclose(fptr);
                }
            }
            v_secure_pclose(ptr);
        }
    }
    else
    {
        char l_cDhcpv6_Dns[256] = {0};
        syscfg_get("dhcpv6spool00", "X_RDKCENTRAL_COM_DNSServers", l_cDhcpv6_Dns, sizeof(l_cDhcpv6_Dns));
        if ( '\0' == l_cDhcpv6_Dns[ 0 ] )
        {
            syscfg_set_commit("dhcpv6spool00", "X_RDKCENTRAL_COM_DNSServersEnabled", "0");
        }
    }

    commonSyseventGet("current_wan_ifname", wan_interface, sizeof(wan_interface));
    commonSyseventGet("wan_ifname", default_wan_interface, sizeof(default_wan_interface));

    syscfg_get(NULL, "router_adv_enable", ra_en, sizeof(ra_en));
    if (strcmp(ra_en, "1") != 0) {
        return 0;
    }

    syscfg_get(NULL, "ra_interval", ra_interval, sizeof(ra_interval));

    //Create a config when wanfailover is enabled
    if(wanFailoverEnabled == true)
    {
        memset(last_broadcasted_prefix,0,sizeof(last_broadcasted_prefix));
        if (gIpv6AddrAssignment == ULA_IPV6)
        {
            // Get the current prefix for ULA
            commonSyseventGet("ipv6_prefix_ula", prefix, sizeof(prefix));
        }
        else
        {
            // Get the current prefix for Global
            commonSyseventGet("lan_prefix", prefix, sizeof(prefix));
        }

        if (gModeSwitched == ULA_IPV6)
        {
            //Get the last broadcasted prefix for ULA
            commonSyseventGet("lan_prefix", last_broadcasted_prefix, sizeof(last_broadcasted_prefix));
        }
        else if (gModeSwitched == GLOBAL_IPV6)
        {
            //Get the last broadcasted prefix for Global
            commonSyseventGet("ipv6_prefix_ula", last_broadcasted_prefix, sizeof(last_broadcasted_prefix));
        }
    }
    else
    {
        // Read from lan_prefix event if prefix is not available
        if(strlen(prefix) == 0)
        {
            commonSyseventGet("lan_prefix", prefix, sizeof(prefix));
        }
    }
    commonSyseventGet("current_lan_ipv6address", lan_addr, sizeof(lan_addr));

    // During captive portal no need to pass DNS
    // Check the reponse code received from Web Service
    if((responsefd = fopen(networkResponse, "r")) != NULL) 
    {
        if(fgets(responseCode, sizeof(responseCode), responsefd) != NULL)
        {
            iresCode = atoi(responseCode);
        }
        fclose(responsefd); /*RDKB-7136, CID-33268, free resource after use*/
        responsefd = NULL;
    }
    syscfg_get( NULL, "redirection_flag", buf, sizeof(buf));
    if( buf != NULL )
    {
        if ((strncmp(buf,"true",4) == 0) && iresCode == 204)
        {
            inWifiCp = 1;
        }
    }
    syscfg_get(NULL, "enableRFCaptivePortal", rfCpEnable, sizeof(rfCpEnable));
    if(rfCpEnable != NULL)
    {
        if (strncmp(rfCpEnable,"true",4) == 0)
        {
            syscfg_get(NULL, "rf_captive_portal", rfCpMode,sizeof(rfCpMode));
            if(rfCpMode != NULL)
            {
                if (strncmp(rfCpMode,"true",4) == 0)
                {
                    inRfCaptivePortal = 1;
                }
            }
        }  
    }
    //Check if captive portal mode is enabled and set the flag
    if((inWifiCp == 1) || (inRfCaptivePortal == 1))
    {
        inCaptivePortal = 1;
    }

    //Get preffered lifetime and valid lifetime
    commonSyseventGet("ipv6_prefix_prdtime", preferred_lft, sizeof(preferred_lft));
    commonSyseventGet("ipv6_prefix_vldtime", valid_lft, sizeof(valid_lft));

    syscfg_get(NULL, "lan_ifname", lan_if, sizeof(lan_if));

    if (atoi(preferred_lft) <= 0)
        snprintf(preferred_lft, sizeof(preferred_lft), "300");
    if (atoi(valid_lft) <= 0)
        snprintf(valid_lft, sizeof(valid_lft), "300");

    if ( atoi(preferred_lft) > atoi(valid_lft) )
        snprintf(preferred_lft, sizeof(preferred_lft), "%s",valid_lft);

    commonSyseventGet( "wan-status", wan_st, sizeof(wan_st));
    syscfg_get(NULL, "last_erouter_mode", rtmod, sizeof(rtmod));
    
    //Do not write a config line for the prefix if it's blank
    if (strlen(prefix))
	{
        char val_DNSServersEnabled[ 32 ];
        interface[i].server_type = DHCPV6_SERVER_TYPE_STATELESS;
        strncpy(interface[i].interface_name, lan_if, sizeof(interface[i].interface_name));
        interface[i].interface_name[ sizeof(interface[i].interface_name) - 1 ] = '\0';
        interface[i].ra_params.enable_ra = true;
        interface[i].ra_params.num_prefix = 0;        

        //If WAN interface is changed and advertise the prefix with lifetime in case of wanfailover
        if (strcmp(default_wan_interface, wan_interface) != 0)
        {
            if(!initializeRaPrefix(&interface[i].ra_params.raPrefix[0],prefix,preferred_lft,valid_lft))
            {
                interface[i].ra_params.num_prefix++;
            }
        }
        else
        {
            //If WAN has stopped, advertise the prefix with lifetime 0 so LAN clients don't use it any more
            if (strcmp(wan_st, "stopped") == 0)
            {
                if (interface[i].ra_params.num_prefix < 2) {
                    if(!initializeRaPrefix(&interface[i].ra_params.raPrefix[interface[i].ra_params.num_prefix], prefix, "0", "0"))
                    {
                        interface[i].ra_params.num_prefix++; // Increment the number of prefixes
                    }
                }  
            }
            else
            {
                if(!initializeRaPrefix(&interface[i].ra_params.raPrefix[0],prefix,preferred_lft,valid_lft))
                {
                    interface[i].ra_params.num_prefix++;
                }
                else
                {
                    LanManagerInfo(("Error Initializing RA Prefix\n"));
                }
                //LTE-1322
                int deviceMode = Get_Device_Mode();
                bool extender_enabled = IsExtenderEnabled();
                if (( DEVICE_MODE_ROUTER == deviceMode ) && (extender_enabled == true))
                {
                    char prefix_primary[64];
                    memset(prefix_primary,0,sizeof(prefix_primary));
                    commonSyseventGet("ipv6_prefix_primary", prefix_primary, sizeof(prefix_primary));
                    if( strlen(prefix_primary) > 0)
                    {
                        if (interface[i].ra_params.num_prefix < 2) {
                            if(!initializeRaPrefix(&interface[i].ra_params.raPrefix[interface[i].ra_params.num_prefix], prefix_primary, "0", "0"))
                            {
                                interface[i].ra_params.num_prefix++; // Increment the number of prefixes
                            }
                        } 
                    }
                }                        
            }
        }
        if(wanFailoverEnabled == true)
        {
            if(strlen(last_broadcasted_prefix) != 0)
            {
                if (interface[i].ra_params.num_prefix < 2) 
                {
                    if(!initializeRaPrefix(&interface[i].ra_params.raPrefix[interface[i].ra_params.num_prefix], last_broadcasted_prefix, "0", "0"))
                    {
                        interface[i].ra_params.num_prefix++; // Increment the number of prefixes
                    }
                }     
            }
        }

        // Set RA interval
        if (strlen(ra_interval) > 0)
        {
            interface[i].ra_params.ra_interval = atoi(ra_interval);
        } 
        else
        {
            interface[i].ra_params.ra_interval = 30; //Set ra-interval to default 30 secs as per Erouter Specs.
        }

        // Set RA lifetime based on the WAN status
        if (strcmp(default_wan_interface, wan_interface) != 0)
        {
            interface[i].ra_params.ra_router_lifetime = 180;
        }
        else
        {
            /* If WAN is stopped or not in IPv6 or dual stack mode, send RA with router lifetime of zero */
            if ( (strcmp(wan_st, "stopped") == 0) || (atoi(rtmod) != 2 && atoi(rtmod) != 3) )
            {
                interface[i].ra_params.ra_router_lifetime = 0;
            }
            else
            {
                interface[i].ra_params.ra_router_lifetime = 180;
            }
        }
        syscfg_get(NULL, "router_managed_flag", m_flag, sizeof(m_flag));
        if (strcmp(m_flag, "1") == 0)
            interface[i].ra_params.ra_managed_flag = atoi(m_flag);

        syscfg_get(NULL, "router_other_flag", o_flag, sizeof(o_flag));
        interface[i].ra_params.ra_other_flag = atoi(o_flag);

        syscfg_get(NULL, "router_mtu", ra_mtu, sizeof(ra_mtu));
        if ( (strlen(ra_mtu) > 0) && (strncmp(ra_mtu, "0", sizeof(ra_mtu)) != 0) )
            printf("   ipv6 nd mtu %s\n", ra_mtu);

        syscfg_get(NULL, "dhcpv6s_enable", dh6s_en, sizeof(dh6s_en));
        if (strcmp(dh6s_en, "1") == 0) //TODO: Need to check if this is the right condition
            printf("   ipv6 nd other-config-flag\n");

            
        /* Static DNS for DHCPv6 
        *   dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers 
        *   dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled
        */
        memset( val_DNSServersEnabled, 0, sizeof( val_DNSServersEnabled ) );
        syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServersEnabled", val_DNSServersEnabled, sizeof(val_DNSServersEnabled));
          
        if( ( val_DNSServersEnabled[ 0 ] != '\0' ) && \
            ( 0 == strcmp( val_DNSServersEnabled, "1" ) )
        )
        {
            StaticDNSServersEnabled = 1;
        }

        // Modifying rdnss value to fix the zebra config.
        if( ( inCaptivePortal != 1 )  && \
            ( StaticDNSServersEnabled != 1 )
          )
        {
            if (strlen(lan_addr))
            {
                if(!populateRdnss(&interface[i].ra_params,lan_addr, rdnsslft))
                {
                    LanManagerInfo(("RDNSS initialized\n"));   
                }
            }
        }
        /* static IPv6 DNS */
        syscfg_get(NULL, "dhcpv6spool00::optionnumber", val, sizeof(val));
        nopt = atoi(val);
        for (j = 0; j < nopt; j++) 
        {
            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::bEnabled", j); /*RDKB-12965 & CID:-34147*/
            syscfg_get(NULL, rec, val, sizeof(val));
            if (atoi(val) != 1)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::Tag", j);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (atoi(val) != 23)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::PassthroughClient", j);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (strlen(val) > 0)
                continue;

            snprintf(rec, sizeof(rec), "dhcpv6spool0option%d::Value", j);
            syscfg_get(NULL, rec, val, sizeof(val));
            if (strlen(val) == 0)
                continue;

            for (start = val; (tok = strtok_r(start, ", \r\t\n", &sp)); start = NULL) {
                snprintf(name_servs + strlen(name_servs), 
                        sizeof(name_servs) - strlen(name_servs), "%s ", tok);
            }
        }

#if defined (SPEED_BOOST_SUPPORTED)
        if( ( inCaptivePortal != 1 ) &&  (strcmp(wan_st, "started") == 0))
        {
            if(!populateSpeedBoostParams(&interface[i].ra_params))
            {
                LanManagerInfo(("Speedboost params initialized\n"));
            }
            else
            {
                LanManagerError(("Speedboost params not initialized\n"));
            }
    	}
#endif

        if(inCaptivePortal != 1)
        {
            /* Static DNS Enabled case */
            if( 1 == StaticDNSServersEnabled )
            {
                memset( name_servs, 0, sizeof( name_servs ) );
   		        syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers", name_servs, sizeof(name_servs));

                fprintf(stderr,"%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__, 
                                                                                __LINE__,
                                                                                StaticDNSServersEnabled,
                                                                                name_servs );
                if ((!strncmp(l_cSecWebUI_Enabled, "true", 4)))
                {
                    char static_dns[256] = {0};
                    commonSyseventGet("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                    if (strlen(name_servs) == 0) {
                        commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs),
                                          sizeof(name_servs) - strlen(name_servs));
                    }
                }                 
			}
			else
			{
                /* DNS from WAN (if no static DNS) */
                if (strlen(name_servs) == 0) {
                    if (wanFailoverEnabled && gIpv6AddrAssignment == ULA_IPV6) 
                    {
                        if (commonSyseventGet("backup_wan_ipv6_nameserver", name_servs + strlen(name_servs), 
                                sizeof(name_servs) - strlen(name_servs)) == 0) 
                        {
                            // If backup WAN IPv6 nameserver is not available, fallback to the regular IPv6 nameserver
                            commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs), 
                                sizeof(name_servs) - strlen(name_servs));
                        }
                    } 
                    else 
                    {
                        commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs), 
                            sizeof(name_servs) - strlen(name_servs));
                    }
                }
            }
            if(!populateRdnss(&interface[i].ra_params,name_servs, rdnsslft))
            {
                LanManagerInfo(("RDNSS initialized\n"));   
            }
            else
            {
                LanManagerInfo(("Error Initializing RDNSS\n"));
            }

            if (atoi(valid_lft) <= 3*atoi(ra_interval))
            {
                // According to RFC8106 section 5.2 dnssl lifttime must be atleast 3 time MaxRtrAdvInterval.
                dnssllft = 3*atoi(ra_interval);
                snprintf(dnssl_lft, sizeof(dnssl_lft), "%d", dnssllft);
            }
            else
            {
                snprintf(dnssl_lft, sizeof(dnssl_lft), "%s", valid_lft);
            }
            commonSyseventGet("ipv6_dnssl", dnssl, sizeof(dnssl));
            for(start = dnssl; (tok = strtok_r(start, " ", &sp)); start = NULL)
            {
                printf("   ipv6 nd dnssl %s %s\n", tok, dnssl_lft);
            }
        }
    }
    //Increment the number of interfaces for brlan0 
    config.num_interfaces++;

    memset(out,0,sizeof(out));
    memset(pref_rx,0,sizeof(pref_rx));
    commonSyseventGet("lan_prefix_v6", pref_rx, sizeof(pref_rx));
    syscfg_get(NULL, "IPv6subPrefix", out, sizeof(out));
    pref_len = atoi(pref_rx);
    if(pref_len < 64)
    {
        if(!strncmp(out,"true",strlen(out)))
        {
            memset(out,0,sizeof(out));
            memset(cmd,0,sizeof(cmd));
            syscfg_get(NULL, "IPv6_Interface", out, sizeof(out));
            pt = out;
            while((token = strtok_r(pt, ",", &pt)))
            {
                config.num_interfaces++;
                interface[ifCount].ra_params.num_prefix = 0; //Initialize the prefix count for other interfaces
                interface[ifCount].server_type = DHCPV6_SERVER_TYPE_STATELESS;
                interface[ifCount].ra_params.enable_ra = true;
                memset(interface_name,0,sizeof(interface_name));
                memset(name_servs, 0, sizeof(name_servs));

                strncpy(interface_name,token,sizeof(interface_name)-1);
                strncpy(interface[ifCount].interface_name, interface_name, sizeof(interface[ifCount].interface_name)-1);
                interface[ifCount].interface_name[sizeof(interface[ifCount].interface_name) - 1 ] = '\0';
             
                // Set the command based on the conditions
                if(wanFailoverEnabled == true)
                {
                    if (gIpv6AddrAssignment == ULA_IPV6)
                    {
                        rc = sprintf_s(cmd, sizeof(cmd), "%s%s",interface_name,"_ipaddr_v6_ula");
                    }
                    else
                    {
                        rc = sprintf_s(cmd, sizeof(cmd), "%s%s",interface_name,"_ipaddr_v6");
                    }

                    // Reset the last broadcast command
                    memset(lastBroadcastCmd,0,sizeof(lastBroadcastCmd));

                    // Set the last broadcast command based on the mode switch
                    if (gModeSwitched == ULA_IPV6 )
                    {
                        rc = sprintf_s(lastBroadcastCmd, sizeof(lastBroadcastCmd), "%s%s",interface_name,"_ipaddr_v6");
                    }
                    else if ( gModeSwitched == GLOBAL_IPV6 )
                    {   
                        rc = sprintf_s(lastBroadcastCmd, sizeof(lastBroadcastCmd), "%s%s",interface_name,"_ipaddr_v6_ula");
                    }
                }
                else
                {
                    // If WAN failover is not enabled, use the default command
                    rc = sprintf_s(cmd, sizeof(cmd), "%s%s",interface_name,"_ipaddr_v6");
                }

                if(rc < EOK)
                {
                    ERR_CHK(rc);
                }
                memset(prefix,0,sizeof(prefix));

                commonSyseventGet(cmd, prefix, sizeof(prefix));

                if (strlen(prefix) != 0)
                {
                    if (strcmp(default_wan_interface, wan_interface) != 0)
                    {
                        if(!initializeRaPrefix(&interface[ifCount].ra_params.raPrefix[0],prefix,preferred_lft,valid_lft))
                        {
                            interface[ifCount].ra_params.num_prefix++;
                        }
                    }
                    else
                    {
                        //If WAN has stopped, advertise the prefix with lifetime 0 so LAN clients don't use it any more
                        if (strcmp(wan_st, "stopped") == 0)
                        {
                            if (interface[i].ra_params.num_prefix < 2) {
                                if(!initializeRaPrefix(&interface[ifCount].ra_params.raPrefix[interface[ifCount].ra_params.num_prefix], prefix, "0", "0"))
                                {
                                    interface[ifCount].ra_params.num_prefix++; // Increment the number of prefixes
                                }
                            }  
                        }
                        else
                        {
                            if(!initializeRaPrefix(&interface[ifCount].ra_params.raPrefix[0],prefix,preferred_lft,valid_lft))
                            {
                                interface[ifCount].ra_params.num_prefix++;
                            }
                           
                        }
                    }
                }               

                memset(last_broadcasted_prefix,0,sizeof(last_broadcasted_prefix));
                commonSyseventGet(lastBroadcastCmd, last_broadcasted_prefix, sizeof(last_broadcasted_prefix));

                if (strlen(last_broadcasted_prefix) != 0)
                {
                    if (interface[ifCount].ra_params.num_prefix < 2) 
                    {
                        if(!initializeRaPrefix(&interface[ifCount].ra_params.raPrefix[interface[ifCount].ra_params.num_prefix], last_broadcasted_prefix, "0", "0"))
                        {
                            interface[ifCount].ra_params.num_prefix++; // Increment the number of prefixes
                        }
                    }
                }
 
                int deviceMode = Get_Device_Mode();
                bool extender_enabled = IsExtenderEnabled();
                if (( DEVICE_MODE_ROUTER == deviceMode ) && (extender_enabled == true))
                {
                    char lan_prefix_primary[64];
                    memset(cmd,0,sizeof(cmd));
                    memset(lan_prefix_primary,0,sizeof(lan_prefix_primary));
                    rc = sprintf_s(cmd, sizeof(cmd), "%s%s",interface_name,"_ipaddr_v6_primary");
                    commonSyseventGet(cmd, lan_prefix_primary, sizeof(lan_prefix_primary));
                    if( strlen(lan_prefix_primary) > 0)
                    {
                        if (interface[ifCount].ra_params.num_prefix < 2) 
                        {
                            if(!initializeRaPrefix(&interface[ifCount].ra_params.raPrefix[interface[ifCount].ra_params.num_prefix], lan_prefix_primary, "0", "0"))
                            {
                                interface[ifCount].ra_params.num_prefix++; // Increment the number of prefixes
                            }
                        }
                    }
                }
                interface[ifCount].ra_params.ra_interval = 3;

                if (strcmp(default_wan_interface, wan_interface) != 0)
                {
                    interface[ifCount].ra_params.ra_router_lifetime = 180;
                }
                else
                {
                    /* If WAN is stopped or not in IPv6 or dual stack mode, send RA with router lifetime of zero */
                    if ( (strcmp(wan_st, "stopped") == 0) || (atoi(rtmod) != 2 && atoi(rtmod) != 3) )
                    {
                        interface[ifCount].ra_params.ra_router_lifetime = 0;
                    }
                    else
                    {
                        interface[ifCount].ra_params.ra_router_lifetime = 180;
                    }
                }
                
                syscfg_get(NULL, "router_managed_flag", m_flag, sizeof(m_flag));
                interface[ifCount].ra_params.ra_managed_flag = atoi(m_flag);

                syscfg_get(NULL, "router_other_flag", o_flag, sizeof(o_flag));
                interface[ifCount].ra_params.ra_other_flag = atoi(o_flag);

                syscfg_get(NULL, "dhcpv6s_enable", dh6s_en, sizeof(dh6s_en));
                if (strcmp(dh6s_en, "1") == 0)
                    printf("   ipv6 nd other-config-flag\n");

                if(inCaptivePortal != 1)
                {
                    /* Static DNS Enabled case */
                    if( 1 == StaticDNSServersEnabled )
                    {
                        memset( name_servs, 0, sizeof( name_servs ) );
                        syscfg_get(NULL, "dhcpv6spool00::X_RDKCENTRAL_COM_DNSServers", name_servs, sizeof(name_servs));
                        fprintf(stderr,"%s %d - DNSServersEnabled:%d DNSServers:%s\n", __FUNCTION__,
                                                                                        __LINE__,
                                                                                        StaticDNSServersEnabled,
                                                                                                                                                            name_servs );
                        if (!strncmp(l_cSecWebUI_Enabled, "true", 4))
                        {
                            char static_dns[256] = {0};
                            commonSyseventGet("lan_ipaddr_v6", static_dns, sizeof(static_dns));
                            /* DNS from WAN (if no static DNS) */
                            if (strlen(name_servs) == 0) {
                                commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs),
                                                sizeof(name_servs) - strlen(name_servs));
                            }
                        }
                    }
                    else
                    {
                        /* DNS from WAN (if no static DNS) */
                        if (strlen(name_servs) == 0) {
                            
                            if(wanFailoverEnabled == true)
                            {
                                if ( gIpv6AddrAssignment == ULA_IPV6 )
                                {
                                    commonSyseventGet("backup_wan_ipv6_nameserver", name_servs + strlen(name_servs),
                                                    sizeof(name_servs) - strlen(name_servs));

                                    if (strlen(name_servs) == 0 )
                                    {
                                        commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs),
                                                sizeof(name_servs) - strlen(name_servs));
                                    }
                                }
                                else
                                {

                                    commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs),
                                            sizeof(name_servs) - strlen(name_servs));
                                }
                            }
                            else
                            {

                                commonSyseventGet("ipv6_nameserver", name_servs + strlen(name_servs),
                                                sizeof(name_servs) - strlen(name_servs));
                            }
                        }
                    }
                    populateRdnss(&interface[ifCount].ra_params,name_servs, rdnsslft);
                }
                ifCount++;
            }
            memset(out,0,sizeof(out));
        }
    }


    config.interfaces = (IPv6Dhcpv6InterfaceConfig*)malloc(sizeof(IPv6Dhcpv6InterfaceConfig) * config.num_interfaces);
    if (config.interfaces == NULL) {
            return -1;       
    }

    for(size_t index = 0; index < config.num_interfaces; index++)
    {
        config.interfaces[index] = interface[index];
    }

    if(dhcpv6_server_create(&config,DHCPV6_SERVER_TYPE_STATELESS) == 0)
    {
        if (config.interfaces != NULL)
        {
            for (size_t index = 0; index < config.num_interfaces; index++)
            {
                // Free dynamically allocated memory within the structure
                free(config.interfaces[index].ra_params.rdnss);
                config.interfaces[index].ra_params.num_prefix = 0;
                
                if (config.interfaces[index].ra_params.sBoost != NULL)
                {
                    free(config.interfaces[index].ra_params.sBoost);
                    config.interfaces[index].ra_params.sBoost = NULL;
                }
            }
            // Free the array itself
            free(config.interfaces);

            // Reset num_interfaces to 0 to indicate that the array is empty
            config.num_interfaces = 0;
        }
    }
    return 0;
}


