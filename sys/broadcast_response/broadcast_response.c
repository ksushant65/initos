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
#include <math.h>
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

/**
 * @brief   PID of the pktdump thread
 */
#define PRECISION 5
 char*  ftoa(float num){
   int whole_part = num;
   int digit = 0, reminder =0;
   int log_value = log10(num), index = log_value;
   long wt =0;

   // String containg result
   char* str = (char*) malloc(20 * sizeof(char));

   //Initilise stirng to zero
   memset(str, 0 ,20);

   //Extract the whole part from float num
   for(int  i = 1 ; i < log_value + 2 ; i++)
   {
       wt  =  pow(10.0,i);
       reminder = whole_part  %  wt;
       digit = (reminder - digit) / (wt/10);

       //Store digit in string
       str[index--] = digit + 48;              // ASCII value of digit  = digit + 48
       if (index == -1)
          break;
   }

    index = log_value + 1;
    str[index] = '.';

   float fraction_part  = num - whole_part;
   float tmp1 = fraction_part,  tmp =0;

   //Extract the fraction part from  num
   for( int i= 1; i < PRECISION; i++)
   {
      wt =10;
      tmp  = tmp1 * wt;
      digit = tmp;

      //Store digit in string
      str[++index] = digit +48;           // ASCII value of digit  = digit + 48
      tmp1 = tmp - digit;
   }

   return str;
}

kernel_pid_t broadcast_response_pid = KERNEL_PID_UNDEF;

float sensor_data = 0;

float get_sensor_data(char* addr){
  send(addr, "8808", "2", 1,0);
  while(!sensor_data);
  float data = sensor_data;
  sensor_data = 0;
  return data;
}
/**
 * @brief   Stack for the pktdump thread
 */
static char _stack[GNRC_PKTDUMP_STACKSIZE];

static void *_eventloop(void *arg)
{
    (void)arg;
    map_init();
    //vector_init(&map, 2*MAX_BROADCAST_LEN);

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
            char* info = data+2;

            if (data[0] == '0') {
              char *result = (char*)malloc(strlen(addr)+strlen(data));
              strcpy(result, addr);
              strcat(result, " ");
          		strcat(result, data+2);
          		add_to_map(result);
              print_map();
            }
            if (data[0] == '1') {
              char* respond_string = (char*)malloc(sizeof(char)*100);
              respond_string[0] = '0';
              respond_string[1] = ' ';
              strcat(respond_string,SERVICE);
              send(addr, "8808", respond_string, 1, 0);
              char *result = (char*)malloc(strlen(addr)+strlen(data));
              strcpy(result, addr);
              strcat(result, " ");
          		strcat(result, data+2);
          		add_to_map(result);
              print_map();
            }
            if (data[0] == '2') {
              float value = get_sensor_value();
              char* to_send = (char*) malloc(22);
              to_send[0] = '3';
              to_send[1] = ' ';
              strcat(to_send, ftoa(value));
              send(addr, "8808", to_send, 1, 0);

            }
            if (data[0] == '3') {
              sensor_data = atof(info);
            }
            /*if (data[0] == '0') {

            }*/
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

kernel_pid_t broadcast_response_init(void)
{
    if (broadcast_response_pid == KERNEL_PID_UNDEF) {
        broadcast_response_pid = thread_create(_stack, sizeof(_stack), GNRC_PKTDUMP_PRIO,
                             THREAD_CREATE_STACKTEST,
                             _eventloop, NULL, "broadcast_response");
    }
    return broadcast_response_pid;
}
