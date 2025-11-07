SM_DHCPMGR library
===================

This folder builds `libsm_dhcpmgr.a` and `libsm_dhcpmgr.so` from the sources in this directory.

How to build
------------

From the repository root (or inside each directory):

    make -C Target_Layer   # builds libdhcp_server_v4
    make -C SM_DHCPMGR     # builds libsm_dhcpmgr (uses headers from Target_Layer)

Notes for linking from other folders
-----------------------------------

Example compile and link flags when using the libraries from another folder:

    -I/path/to/DHCPManager_Server/SM_DHCPMGR -I/path/to/DHCPManager_Server/Target_Layer \
    -L/path/to/DHCPManager_Server/SM_DHCPMGR -lsm_dhcpmgr \
    -L/path/to/DHCPManager_Server/Target_Layer -ldhcp_server_v4

You can install the headers and libraries to a system prefix with:

    make -C Target_Layer install PREFIX=/some/prefix
    make -C SM_DHCPMGR install PREFIX=/some/prefix
