#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_pkt.h"
#include "pkt_struct.h"

#include <stdlib.h>
#include <string.h>

char **split(char *str, size_t len, char delim, char ***result, unsigned long *count, unsigned long max) {
  size_t i;
  char **_result;

  // there is at least one string returned
  *count=1;

  _result= *result;

  // when the result array is specified, fill it during the first pass
  if (_result) {
    _result[0]=str;
  }

  // scan the string for delimiter, up to specified length
  for (i=0; i<len; ++i) {

    // to compare against a list of delimiters,
    // define delim as a string and replace 
    // the next line:
    //     if (str[i]==delim) {
    //
    // with the two following lines:
    //     char *c=delim; while(*c && *c!=str[i]) c++;
    //     if (*c) {
    //       
    if (str[i]==delim) {

      // replace delimiter with zero
      str[i]=0;

      // when result array is specified, fill it during the first pass
      if (_result) {
        _result[*count]=str+i+1;
      }

      // increment count for each separator found
      ++(*count);

      // if max is specified, dont go further
      if (max && *count==max)  {
        break;
      }

    }
  }

  // when result array is specified, we are done here
  if (_result) {
    return _result;
  }

  // else allocate memory for result
  // and fill the result array                                                                                    

  *result=malloc((*count)*sizeof(char*));
  if (!*result) {
    return NULL;
  }
  _result=*result;

  // add first string to result
  _result[0]=str;

  // if theres more strings
  for (i=1; i<*count; ++i) {

    // find next string
    while(*str) ++str;
    ++str;

    // add next string to result
    _result[i]=str;

  }

  return _result;
}


int parser(pkt_structure p_s, char *pkt_data){
	char **result = 0;
	unsigned long count;
	unsigned long i=0;
	int flag =1;
	
	split(strdup(pkt_data),strlen(pkt_data),' ', &result, &count, 0);
	if((int)count != p_s.size)	return 0;
	
	for(i=0; i<count; i++){
		float num = atof(result[i]);
		if(!(num >= p_s.min_max[i][0] && num <= p_s.min_max[i][1]))
		{
			flag = 0;
			break;
		}
	}

	return flag;
}
