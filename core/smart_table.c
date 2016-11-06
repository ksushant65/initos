#include "include/vector.h"
#include "include/smart_table.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static Vector smart_table;

char ip_sensor_delim = '-';
char sensor_config_delim = '=';

void smart_table_init(void)
{
	vector_init(&smart_table, 2*MAX_BROADCAST_LEN);
}

void add_to_table(char *ip,char *sensor_value,char *config_data)
{

	char* entry = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	int i;
	for(i=0;ip[i] != '\0';i++)
		entry[i] = ip[i];

	entry[i++] = ip_sensor_delim;
	int j=0;
	for(j=0;sensor_value[j] != '\0';j++,i++)
		entry[i] = sensor_value[j];

	entry[i++] = sensor_config_delim;
	for(j=0;config_data[j] != '\0';j++,i++)
		entry[i] = config_data[j];

	entry[i++] = '\0';

	if(contains_ip_sensor(ip,sensor_value)){
		update_entry(entry);
	}else{
		add_entry_to_smart_table(entry);
	}
}

char * get_config(char *ip, char *sensor_value)
{
	char *nullConfig = '\0';
	if(contains_ip_sensor(ip,sensor_value)){
		int size = smart_table.size, i;
    	for(i=0;i<size; i++)
    	{
	        char *entry = smart_table.data[i];

	        if(strcmp(get_ip_from_table(entry),ip) == 0 && strcmp(get_sensor_value_from_table(entry),sensor_value) == 0){
	        	return get_config_data_from_table(entry);
	        }
    	}
	}

	return nullConfig;
}

int contains_ip_sensor(char *ip,char *sensor_value)
{
	int size = smart_table.size, i;
    for(i=0;i<size; i++)
    {
        char *entry = smart_table.data[i];

        if(strcmp(get_ip_from_table(entry),ip) == 0 && strcmp(get_sensor_value_from_table(entry),sensor_value) == 0){
        	return 1;
        }
    }

    return 0;
}

void update_entry(char *entry)
{
	int size = smart_table.size, i;
    for(i=0;i<size; i++){
        char *data = smart_table.data[i];

        if(strcmp(get_ip_from_table(entry),get_ip_from_table(data)) == 0 && strcmp(get_sensor_value_from_table(entry),get_sensor_value_from_table(data)) == 0){
        	strcpy(smart_table.data[i],entry);
        }
    }
}

void add_entry_to_smart_table(char * entry) 
{
	vector_append(&smart_table,entry);
}

void delete_smart_table(void)
{
	vector_free(&smart_table);
}

char* get_ip_from_table(char* entry)
{
	char* ip;
	int i;
	ip = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	for(i=0;entry[i] != ip_sensor_delim;i++)
	{
		ip[i] = entry[i];
	}

	ip[i] = '\0';

	return ip;
}

char* get_sensor_value_from_table(char* entry)
{
	int i=0;
	while(entry[i] != ip_sensor_delim)
		i++;

	char* sensor_value = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	int j=0;
	for(i = i+1;entry[i] != sensor_config_delim;i++)
		sensor_value[j++] = entry[i];

	sensor_value[j] = '\0';

	return sensor_value;
}

char* get_config_data_from_table(char* entry)
{
	int i=0;
	while(entry[i] != sensor_config_delim)
		i++;

	char* config_data = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	int j=0;
	for(i = i+1;entry[i] != '\0';i++)
		config_data[j++] = entry[i];

	config_data[j] = '\0';

	return config_data;
}


//utility to print smart_table
void print_smart_table(void)
{
    int size = smart_table.size, i, j;
    printf("Vector smart_table:\n");
    for(i=0;i<size; i++)
    {
        for(j=0; smart_table.data[i][j]!='\0'; j++) printf("%c", smart_table.data[i][j]);
        printf("\n");
    }
}