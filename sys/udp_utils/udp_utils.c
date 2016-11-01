#include <stdio.h>
#include <inttypes.h>

#include "udp_utils.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/nettype.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6/netif.h"
#include "timex.h"
#include "xtimer.h"

static gnrc_netreg_entry_t server = { NULL, GNRC_NETREG_DEMUX_CTX_ALL, KERNEL_PID_UNDEF };

void send(char *addr_str, char *port_str, char *data, unsigned int num,
                 unsigned int delay)
{
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

void start_server(char *port_str, kernel_pid_t (*callback)(void))
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
    server.pid = (*callback)();
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

  send(ipv6_addr, "8808", message, 1, 0);
}
