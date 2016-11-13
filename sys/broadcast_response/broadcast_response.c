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
#include "../../core/include/map.h"
#include "../../core/include/sensor_data.h"
#include "../../core/include/parse_pkt.h"
#include "../../core/include/smart_table.h"

char config[20], ip[20];

char* itoa(int i, char b[]){
    char const digit[] = "0123456789";
    char* p = b;
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    int shifter = i;
    do{ //Move to where representation ends
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}

char*  ftoa(float num){
   int integer_part = num;
   int float_part = (num-integer_part)*100;

   char* int_string = (char*)malloc(13);
   itoa(integer_part,int_string);
   char float_string[2];
   itoa(float_part,float_string);

   strcat(int_string,".");
   strcat(int_string,float_string);

   return int_string;
}

kernel_pid_t broadcast_response_pid = KERNEL_PID_UNDEF;

/**
 * @brief   Stack for the pktdump thread
 */
static char _stack[GNRC_PKTDUMP_STACKSIZE];

static void *_eventloop(void *arg)
{
    (void)arg;
    map_init();
    smart_table_init();
    //vector_init(&map, 2*MAX_BROADCAST_LEN);

    msg_t msg, reply;
    msg_t msg_queue[GNRC_PKTDUMP_MSG_QUEUE_SIZE];

    /* setup the message queue */
    msg_init_queue(msg_queue, GNRC_PKTDUMP_MSG_QUEUE_SIZE);

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;

    while (1) {
        msg_receive(&msg);
        printf("A message is receieved\n");
        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
	        ; //empty statement to resolve "error: a label can only be part of a statement and declaration is not a statement"
      		char** resp = parse_response(msg.content.ptr);

            //TODO: Match the format of resp[1] to "<string><delimeter><delimeter separated strings><delimeter><dstIP><delimeter><srcIP>"
            //e.g. "0 23 22 WTF fe80::1234:1234::1 fe80::2000:9190::1"

            char* addr2 = resp[0]; //local linux tap interface address
            //get source(remote) ipv6 addr(tap interface), data(config number) and info
            char **payload = NULL;
            unsigned long payloadSize;
            size_t respLen = strlen(resp[1]);
            split(resp[1], respLen, ' ', &payload, &payloadSize, 0);
            char* addr = payload[payloadSize-1]; // remote ip address(could be a smartphone)
            char* data = payload[0];
            unsigned long int temp=1;
            char *info = (char *)malloc(sizeof(char)*100);
            memset(info, '\0', sizeof(char)*100);
            while(temp<payloadSize-2){
                strcat(info, payload[temp]);
                strcat(info, " ");
                temp++;
            }

            printf("%s %s -> %s\n", data, info, addr2);

            if (data[0] == '0') {
                //reply of ping
              char *result = (char*)malloc(sizeof(char)*100);
              memset(result, '\0', sizeof(char)*100);
              strcpy(result, addr);
              strcat(result, " ");
          	  strcat(result, info);
          	  add_to_map(result);
              print_map();
            }
            if (data[0] == '1') {
                //ping
              char* respond_string = (char*)malloc(sizeof(char)*100);
              memset(respond_string, '\0', sizeof(char)*100);
              strcat(respond_string, "0 ");
              strcat(respond_string,"WTF ");
              strcat(respond_string,addr);
              send(addr2, "8808", respond_string, 1, 0);
              char *result = (char*)malloc(sizeof(char)*100);
              memset(result, '\0', sizeof(char)*100);
              strcpy(result, addr);
              strcat(result, " ");
          	  strcat(result, info);
          	  add_to_map(result);
              print_map();
            }
            if (data[0] == '2') {
                //send sensory data format="2"
              float value = get_sensor_value();
              char* to_send = (char*) malloc(sizeof(char)*100);
              memset(to_send, '\0', sizeof(char)*100);
              strcat(to_send, "3 ");
              strcat(to_send, ftoa(value));
              strcat(to_send, " ");
              strcat(to_send, addr);
              send(addr2, "8808", to_send, 1, 0);

            }
            if (data[0] == '3') {
                //receive sensory data from another node format=not worth mentioning
              add_to_table(ip,info,config);
              print_smart_table();
            }
            if (data[0] == '4'){
                //set config data
                //format="4 2 32 ..."

                send(addr2, "8808", "2 ::", 1, 0);
                strcpy(ip,addr);
                strcpy(config,info);

                unsigned long int i=1;
                while(i<payloadSize-2){
                    set_sensor_value(atof(payload[i]));
                    i++;
                }
            }
            if (data[0] == '5') {
                //send sensory data format="5"
              float value = get_sensor_value();
              char* to_send = (char*) malloc(sizeof(char)*100);
              memset(to_send, '\0', sizeof(char)*100);
              strcat(to_send, "6 ");
              strcat(to_send, ftoa(value));
              strcat(to_send, " ");
              strcat(to_send, addr);
              send(addr2, "8808", to_send, 1, 0);

            }
            if (data[0] == '6') {
              printf("Here is the corresponding config %s\n", get_config(ip,info));
            }
            if (data[0] == '7'){
                //Receives packet from user's device and starts the process
                send(addr, "8808", "5 ::", 1, 0);
                strcpy(ip,addr);
            }
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
        //memset(string[i], '\0', MAX_BROADCAST_LEN*sizeof(char));

    gnrc_pktsnip_t *data_snip = gnrc_pktsnip_search_type(pkt,GNRC_NETTYPE_UNDEF);
    gnrc_pktsnip_t *addr_snip = gnrc_pktsnip_search_type(pkt,GNRC_NETTYPE_IPV6);

    char addr_str[IPV6_ADDR_MAX_STR_LEN];
    ipv6_hdr_t *hdr = addr_snip->data;
    ipv6_addr_to_str(addr_str, &hdr->src,sizeof(addr_str));
    strcpy(string[0],addr_str);

    size_t index = 0;
    while(index < data_snip->size)
    {
        string[1][index] = *((char *)data_snip->data + index);
        index++;
    }
    string[1][index] = '\0';
    return string;

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
