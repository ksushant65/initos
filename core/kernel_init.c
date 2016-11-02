/*
 * Copyright (C) 2013 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     core_internal
 * @{
 *
 * @file
 * @brief       Platform-independent kernel initilization
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "kernel_init.h"
#include "sched.h"
#include "thread.h"
#include "lpm.h"
#include "irq.h"
#include "log.h"
#include "../sys/include/udp_utils.h"
#include "parse_pkt.h"
#include "pkt_struct.h"

#ifdef MODULE_SCHEDSTATISTICS
#include "sched.h"
#endif

#define ENABLE_DEBUG (0)
#include "debug.h"

#ifdef MODULE_AUTO_INIT
#include <auto_init.h>
#endif

volatile int lpm_prevent_sleep = 0;

extern int main(void);
static void *main_trampoline(void *arg)
{
    (void) arg;

#ifdef MODULE_AUTO_INIT
    auto_init();
#endif

#ifdef MODULE_SCHEDSTATISTICS
    schedstat *stat = &sched_pidlist[thread_getpid()];
    stat->laststart = 0;
#endif

    LOG_INFO("main(): This is RIOT! (Version: " RIOT_VERSION ")\n");

    char* broadcast_string = (char*)malloc(sizeof(char)*100);
    start_server("8808");
    broadcast_string[0] = '1';
    broadcast_string[1] = ' ';
    strcat(broadcast_string,SERVICE);
    broadcast(broadcast_string);

    pkt_structure p_s;
    p_s.size = 3;
    p_s.min_max[0][0] = 2;
    p_s.min_max[0][1] = 7;
    p_s.min_max[1][0] = 20;
    p_s.min_max[1][1] = 70;
    p_s.min_max[2][0] = 200;
    p_s.min_max[2][1] = 700;

    char *pkt_data = "2.566 30.55 200.1";
    printf("Packet Validity: %d",parser(p_s,pkt_data));
    main();

    return NULL;
}

static void *idle_thread(void *arg)
{
    (void) arg;

    while (1) {
        if (lpm_prevent_sleep) {
            lpm_set(LPM_IDLE);
        }
        else {
            lpm_set(LPM_IDLE);
            /* lpm_set(LPM_SLEEP); */
            /* lpm_set(LPM_POWERDOWN); */
        }
    }

    return NULL;
}

const char *main_name = "main";
const char *idle_name = "idle";

static char main_stack[THREAD_STACKSIZE_MAIN];
static char idle_stack[THREAD_STACKSIZE_IDLE];

void kernel_init(void)
{
    (void) irq_disable();

    thread_create(idle_stack, sizeof(idle_stack),
            THREAD_PRIORITY_IDLE,
            THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
            idle_thread, NULL, idle_name);

    thread_create(main_stack, sizeof(main_stack),
            THREAD_PRIORITY_MAIN,
            THREAD_CREATE_WOUT_YIELD | THREAD_CREATE_STACKTEST,
            main_trampoline, NULL, main_name);

    cpu_switch_context_exit();
}
