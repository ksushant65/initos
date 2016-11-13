/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_pktdump Dump Network Packets
 * @ingroup     net_gnrc
 * @brief       Dump network packets to STDOUT for debugging
 *
 * @{
 *
 * @file
 * @brief       Interface for a generic network packet dumping module
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef GNRC_PKTDUMP_H_
#define GNRC_PKTDUMP_H_

#include "kernel_types.h"
#include "net/gnrc/pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Message queue size for the pktdump thread
 */
#ifndef GNRC_PKTDUMP_MSG_QUEUE_SIZE
#define GNRC_PKTDUMP_MSG_QUEUE_SIZE     (8U)
#endif

/**
 * @brief   Priority of the pktdump thread
 */
#ifndef GNRC_PKTDUMP_PRIO
#define GNRC_PKTDUMP_PRIO               (THREAD_PRIORITY_MAIN - 1)
#endif

#ifndef MAX_BROADCAST_LEN
#define MAX_BROADCAST_LEN		(100)
#endif

/**
 * @brief   Stack size used for the pktdump thread
 */
#ifndef GNRC_PKTDUMP_STACKSIZE
#define GNRC_PKTDUMP_STACKSIZE          (THREAD_STACKSIZE_MAIN)
#endif

/**
 * @brief   The PID of the pktdump thread
 */
extern kernel_pid_t broadcast_response_pid;

/**
 * @brief   Start the packet dump thread and listening for incoming packets
 *
 * @return  PID of the pktdump thread
 * @return  negative value on error
 */
kernel_pid_t broadcast_response_init(void);

char** parse_response(gnrc_pktsnip_t *pkt);
void print_map(void);
#ifdef __cplusplus
}
#endif

#endif /* GNRC_PKTDUMP_H_ */
/** @} */
