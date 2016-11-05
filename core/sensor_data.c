float sensor_value=0;

float get_sensor_value(void) {
  return 2.3;
}

int set_sensor_value(float data) {
	//Checking if the data is in valid range or not.
	sensor_value = data;
	return 1;
}
