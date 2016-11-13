#include <stdio.h>
#include <inttypes.h>

#include "udp_utils.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "broadcast_response.h"
#include "net/gnrc/nettype.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6/netif.h"
#include "timex.h"
#include "xtimer.h"
#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6/netif.h"

static gnrc_netreg_entry_t server = { NULL, GNRC_NETREG_DEMUX_CTX_ALL, KERNEL_PID_UNDEF };

void send(char *addr_str, char *port_str, char *olddata, unsigned int num,
                 unsigned int delay)
{
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    size_t numof = gnrc_netif_get(ifs);
    char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];

    if (numof > 0) {
        gnrc_ipv6_netif_t *entry = gnrc_ipv6_netif_get(ifs[0]);
        for (int i = 0; i < GNRC_IPV6_NETIF_ADDR_NUMOF; i++) {
            if ((ipv6_addr_is_link_local(&entry->addrs[i].addr)) && !(entry->addrs[i].flags & GNRC_IPV6_NETIF_ADDR_FLAGS_NON_UNICAST)) {
                //char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];
                ipv6_addr_to_str(ipv6_addr, &entry->addrs[i].addr, IPV6_ADDR_MAX_STR_LEN);
            }
        }
    }

    char *data = (char*)malloc(sizeof(char)*100);
    memset(data, '\0', sizeof(char)*100);
    strcat(data, olddata);
    strcat(data, " ");
    strcat(data,ipv6_addr);
    printf("packet: %s\n", data);

    uint16_t port;
    ipv6_addr_t addr;

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("Error: unable to parse destination address");
        return;
    }
    /* parse port */
    port = (uint16_t)atoi(port_str);
    if (port == 0) {
        puts("Error: unable to parse destination port");
        return;
    }

    for (unsigned int i = 0; i < num; i++) {
        gnrc_pktsnip_t *payload, *udp, *ip;
        /* allocate payload */
        payload = gnrc_pktbuf_add(NULL, data, strlen(data), GNRC_NETTYPE_UNDEF);
        if (payload == NULL) {
            puts("Error: unable to copy data to packet buffer");
            return;
        }
        /* allocate UDP header, set source port := destination port */
        udp = gnrc_udp_hdr_build(payload, port, port);
        if (udp == NULL) {
            puts("Error: unable to allocate UDP header");
            gnrc_pktbuf_release(payload);
            return;
        }
        /* allocate IPv6 header */
        ip = gnrc_ipv6_hdr_build(udp, NULL, &addr);
        if (ip == NULL) {
            puts("Error: unable to allocate IPv6 header");
            gnrc_pktbuf_release(udp);
            return;
        }
        /* send packet */
        if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
            puts("Error: unable to locate UDP thread");
            gnrc_pktbuf_release(ip);
            return;
        }
        /* access to `payload` was implicitly given up with the send operation above
         * => use temporary variable for output */
        xtimer_usleep(delay);
    }
}

void start_server(char *port_str)
{
    uint16_t port;

    /* check if server is already running */
    if (server.pid != KERNEL_PID_UNDEF) {
        printf("Error: server already running on port %" PRIu32 "\n",
               server.demux_ctx);
        return;
    }
    /* parse port */
    port = (uint16_t)atoi(port_str);
    if (port == 0) {
        puts("Error: invalid port specified");
        return;
    }
    /* start server (which means registering pktdump for the chosen port) */
    server.pid = broadcast_response_init();
    server.demux_ctx = (uint32_t)port;
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &server);
    printf("Success: started UDP server on port %" PRIu16 "\n", port);
}

void stop_server(void)
{
    /* check if server is running at all */
    if (server.pid == KERNEL_PID_UNDEF) {
        printf("Error: server was not running\n");
        return;
    }
    /* stop server */
    gnrc_netreg_unregister(GNRC_NETTYPE_UDP, &server);
    server.pid = KERNEL_PID_UNDEF;
    puts("Success: stopped UDP server");
}

char* get_multicast_address(void)
{
  kernel_pid_t ifs[GNRC_NETIF_NUMOF];

  gnrc_netif_get(ifs);
  gnrc_ipv6_netif_t *entry = gnrc_ipv6_netif_get(ifs[0]);
  char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];
  return ipv6_addr_to_str(ipv6_addr, &entry->addrs[0].addr, IPV6_ADDR_MAX_STR_LEN);
}

void broadcast(char* message)
{
  kernel_pid_t ifs[GNRC_NETIF_NUMOF];

  gnrc_netif_get(ifs);
  gnrc_ipv6_netif_t *entry = gnrc_ipv6_netif_get(ifs[0]);
  char ipv6_addr[IPV6_ADDR_MAX_STR_LEN];
  ipv6_addr_to_str(ipv6_addr, &entry->addrs[0].addr, IPV6_ADDR_MAX_STR_LEN);

  printf("%s\n", ipv6_addr);

  send(ipv6_addr, "8808", message, 1, 0);
  char *new_message = (char*)malloc(sizeof(char)*100);
  memset(new_message, '\0', sizeof(char)*100);
  strcat(new_message, "4 10.10 300.0 4.3 mylocalip");
  //send(ipv6_addr, "8808", new_message, 1, 0);
}
