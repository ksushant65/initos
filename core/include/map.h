#ifndef MAX_BROADCAST_LEN
#define MAX_BROADCAST_LEN		(46)
#endif

void map_init(void);

void add_to_map(char * value);

void delete_map(void);

void print_map(void);

char** get_all_nodes(void);

char** get_ip_addresses(char* service_name);

char* get_ip(char* entry);

char* get_service_name(char* entry);
