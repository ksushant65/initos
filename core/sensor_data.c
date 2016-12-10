#include "sensor_data.h"

float sensor_value=1;

float get_sensor_value(void) {
  return sensor_value;
}

int set_sensor_value(float data) {
	//Checking if the data is in valid range or not.
	sensor_value = data;
	return 1;
}
