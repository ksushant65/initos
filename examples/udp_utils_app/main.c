#include<stdio.h>

#include "udp_utils.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int main(void){
	msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
	start_server("8808");

	msg_receive(_main_msg_queue);
	return 0;
}
