#define _WIN32_WINNT 0x0501

#include <stdio.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <iphlpapi.h>

#include "network.h"
#include "common.h"

#pragma comment(lib, "Iphlpapi.lib")

int get_adapters_addresses(IP_ADAPTER_ADDRESSES *adapters, ULONG *size)
{
    return GetAdaptersAddresses(AF_UNSPEC, 0, 0, adapters, size);
}

int c_get_network_interfaces(struct network_interface *ns, int max_ns)
{
    struct sockaddr *addr;
    IP_ADAPTER_ADDRESSES *adapters, *adapter;
    IP_ADAPTER_UNICAST_ADDRESS *unicast;
    ULONG buffer_size;
    DWORD error;
    int family, i;

    /* make an initial call to get the necessary
     * size into the buffer_size variable */
    error = get_adapters_addresses(NULL, &buffer_size);

    if (error != ERROR_BUFFER_OVERFLOW) {
        /* if we didn't get ERROR_BUFFER_OVERFLOW
         * then buffer_size was not set */
        return 0;
    }

    adapters = (IP_ADAPTER_ADDRESSES*)malloc(buffer_size);
    error = get_adapters_addresses(adapters, &buffer_size);
    i = 0;

    if (error == NO_ERROR) {
        adapter = adapters;

        while (i < max_ns && adapter) {
            wszcopy(ns[i].name, adapter->FriendlyName, NAME_SIZE);
            memcpy(ns[i].mac_address, adapter->PhysicalAddress, MAC_SIZE);

            for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
                addr = unicast->Address.lpSockaddr;
                family = addr->sa_family;

                if (family == AF_INET) {
                    ipv4copy(&ns[i].ip_address, addr);
                } else if (family == AF_INET6) {
                    ipv6copy(&ns[i].ip6_address, addr);
                }
            }

            i++;
            adapter = adapter->Next;
        }
    }

    free(adapters);
    return i;
}

#include "common.inc"
