#ifndef MAX_BROADCAST_LEN
#define MAX_BROADCAST_LEN		(46)
#endif

void smart_table_init(void);

void add_to_table(char *ip,char *sensor_value,char *config_data);

char * get_config(char *ip, char *sensor_value);

int contains_ip_sensor(char *ip,char *sensor_value);

void update_entry(char *entry);

void add_entry_to_smart_table(char * entry);

void delete_smart_table(void);

char* get_ip_from_table(char* entry);

char* get_sensor_value_from_table(char* entry);

char* get_config_data_from_table(char* entry);

void print_smart_table(void);
