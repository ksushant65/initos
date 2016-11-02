/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_pktdump
 * @{
 *
 * @file
 * @brief       Generic module to dump packages received via netapi to STDOUT
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include "byteorder.h"
#include "thread.h"
#include "msg.h"
#include "broadcast_response.h"
#include "net/gnrc.h"
#include "net/ipv6/addr.h"
#include "net/ipv6/hdr.h"
#include "net/udp.h"
#include "net/sixlowpan.h"
#include "od.h"
#include "udp_utils.h"
#include "net/gnrc/pkt.h"
#include "../../core/include/vector.h"

/**
 * @brief   PID of the pktdump thread
 */
kernel_pid_t broadcast_response_pid = KERNEL_PID_UNDEF;

/**
 * @brief   Stack for the pktdump thread
 */
static char _stack[GNRC_PKTDUMP_STACKSIZE];

Vector map;



static void *_eventloop(void *arg)
{
    (void)arg;
    vector_init(&map, 2*MAX_BROADCAST_LEN);
    msg_t msg, reply;
    msg_t msg_queue[GNRC_PKTDUMP_MSG_QUEUE_SIZE];

    /* setup the message queue */
    msg_init_queue(msg_queue, GNRC_PKTDUMP_MSG_QUEUE_SIZE);

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;

    while (1) {
        msg_receive(&msg);

        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
		        ; //empty statement to resolve "error: a label can only be part of a statement and declaration is not a statement"
        		char** resp = parse_response(msg.content.ptr);
            char* addr = resp[0];
            char* data = resp[1];

            if (data[0] == '0') {
              char *result = (char*)malloc(strlen(addr)+strlen(data));
              strcpy(result, addr);
              strcat(result, " ");
          		strcat(result, data+2);
          		vector_append(&map, result);
            }
            if (data[0] == '1') {
              char* respond_string = "0 ";
              strcat(respond_string,SERVICE);
              send(addr, "8808", respond_string, 1, 0);
              char *result = (char*)malloc(strlen(addr)+strlen(data));
              strcpy(result, addr);
              strcat(result, " ");
          		strcat(result, data+2);
          		vector_append(&map, result);
            }
            /*if (data[0] == '0') {

            }
            if (data[0] == '0') {

            }*/
        		print_map();
            case GNRC_NETAPI_MSG_TYPE_SND:
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;
            default:
                break;
        }
    }

    /* never reached */
    return NULL;
}


char** parse_response(gnrc_pktsnip_t *pkt)
{
    char** string;
    string = (char **)malloc(2*sizeof(char *));
    int i;
    for (i=0; i<2; i++)
        string[i] = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

    gnrc_pktsnip_t *data_snip = gnrc_pktsnip_search_type(pkt,GNRC_NETTYPE_UNDEF);
    gnrc_pktsnip_t *addr_snip = gnrc_pktsnip_search_type(pkt,GNRC_NETTYPE_IPV6);

    char addr_str[IPV6_ADDR_MAX_STR_LEN];
    ipv6_hdr_t *hdr = addr_snip->data;
    ipv6_addr_to_str(addr_str, &hdr->src,sizeof(addr_str));
    strcpy(string[0],addr_str);

    size_t index = 0;
    //printf("%d %d", data_snip->size, IPV6_ADDR_MAX_STR_LEN);
    while(index < data_snip->size)
    {
        string[1][index] = *((char *)data_snip->data + index);
        index++;
    }
    string[1][index] = '\0';
    return string;

}

//utility to print map
void print_map(void){
    int size = map.size, i, j;
    printf("Vector Map of connected devices:\n");
    for(i=0;i<size; i++){
        for(j=0; map.data[i][j]!='\0'; j++) printf("%c", map.data[i][j]);
        printf("\n");
    }
}

kernel_pid_t broadcast_response_init(void)
{
    if (broadcast_response_pid == KERNEL_PID_UNDEF) {
        broadcast_response_pid = thread_create(_stack, sizeof(_stack), GNRC_PKTDUMP_PRIO,
                             THREAD_CREATE_STACKTEST,
                             _eventloop, NULL, "broadcast_response");
    }
    return broadcast_response_pid;
}
