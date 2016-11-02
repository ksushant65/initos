#ifndef PKT_STRUCT_H
#define PKT_STRUCT_H
#define SIZE 3

typedef struct{
	float min_max[SIZE][2];

} pkt_structure;

extern pkt_structure packet_structure;

void pkt_struct_init(void);
#endif
