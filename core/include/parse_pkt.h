#ifndef PARSE_PKT_H
#define PARSE_PKT_H
#endif
#include <stdio.h>
#include "pkt_struct.h"

extern char **split(char *str, size_t len, char delim, char ***result, unsigned long *count, unsigned long max);
extern int parser(char *pkt_data);
