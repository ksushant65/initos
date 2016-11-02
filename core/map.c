#include "include/vector.h"
#include "include/map.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static Vector map;

char delimiter;

void map_init(void)
{
	delimiter = ' ';
	vector_init(&map, 2*MAX_BROADCAST_LEN);
}

void add_to_map(char * entry) 
{
	vector_append(&map,entry);
}


void delete_map(void)
{
	vector_free(&map);
}

char** get_all_nodes(void)
{
	char ** ip_addresses = (char **)malloc(map.size*sizeof(char *));
    int i;
    for (i=0; i<map.size; i++)
        ip_addresses[i] = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	for(i=0;i<map.size;i++)
	{
		strcpy(ip_addresses[i] , get_ip(vector_get(&map,i)) );
	}

	/*
	printf("getAllNodes\n");
	for(int i=0;i<map.size;i++){
		printf("%d : %s\n", i,ip_addresses[i]);
	}*/
	return ip_addresses;
}

char** get_ip_addresses(char* service_name)
{
	char ** ip_addresses = (char **)malloc(map.size*sizeof(char *));
    int i;
    for (i=0; i<map.size; i++)
        ip_addresses[i] = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

    //printf("get_ip_addresses\n");
    int j=0;
	for(i=0;i<map.size;i++)
	{
		//printf("service_name_of_entry :%sEND\n service_name:%sEND\n strcmp_result : %d\n", get_service_name(vector_get(&map,i)),service_name,
			//strcmp(get_service_name(vector_get(&map,i)), service_name));
		if(strcmp(get_service_name(vector_get(&map,i)), service_name) == 0){
			strcpy(ip_addresses[j++] ,  get_ip(vector_get(&map,i)));
		}
	}

	/*for(int i=0;i<map.size;i++){
		printf("%d : %s\n", i,ip_addresses[i]);
	}*/

	return ip_addresses;
}

char* get_ip(char* entry)
{
	char* ip;
	int i;
	ip = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	for(i=0;entry[i] != ' ';i++)
	{
		ip[i] = entry[i];
	}

	ip[i] = '\0';

	return ip;
}

char* get_service_name(char* entry)
{
	char* service;
	int i=0;
	service = (char *)malloc(MAX_BROADCAST_LEN*sizeof(char));

	while(entry[i] != ' ')
		i++;
	i++;
	int j=0;
	for(j=0; entry[j] != '\0' ;i++,j++)
	{
		service[j] = entry[i];
	}

	service[j] = '\0';
	//printf("entry : %s  service : %s\n",entry,service);
	return service;

}

//utility to print map
void print_map(void)
{
    int size = map.size, i, j;
    printf("Vector Map of connected devices:\n");
    for(i=0;i<size; i++){
        for(j=0; map.data[i][j]!='\0'; j++) printf("%c", map.data[i][j]);
        printf("\n");
    }
}