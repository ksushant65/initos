#ifndef UDP_UTILS_H
#define UDP_UTILS_H

#include "kernel_types.h"

extern void send(char *addr_str, char *port_str, char *data, unsigned int num, unsigned int delay);

extern void start_server(char *port_str, kernel_pid_t (*callback)(void (*cb)(char**)), void (*cb)(char**));

extern void stop_server(void);

extern char* get_multicast_address(void);

extern void broadcast(char* message);
#endif
