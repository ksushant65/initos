#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

int numOfCols;

void vector_init(Vector *vector, int cols) {
  // initialize size and capacity
  vector->size = 0;
  vector->capacity = VECTOR_INITIAL_CAPACITY;
  numOfCols = cols;

  // allocate memory for vector->data
  vector->data = (char **)malloc(sizeof(char* ) * vector->capacity);
  int i=0;
  for(i=0;i< vector->capacity; i++)
	vector->data[i] = (char*)malloc(sizeof(char) * numOfCols);
}

void vector_append(Vector *vector, char * value) {
  // make sure there's room to expand into
  vector_double_capacity_if_full(vector);

  // append the value and increment vector->size
  vector->data[vector->size++] = value;
}

char* vector_get(Vector *vector, int index) {
  if (index >= vector->size || index < 0) {
    printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
    exit(1);
  }
  return vector->data[index];
}

void vector_set(Vector *vector, int index, char* value) {
  // zero fill the vector up to the desired index
  while (index >= vector->size) {
    vector_append(vector, '\0');
  }

  // set the value at the desired index
  vector->data[index] = value;
}

void vector_double_capacity_if_full(Vector *vector) {
  if (vector->size >= vector->capacity) {
    // double vector->capacity and resize the allocated memory accordingly
    int i=vector->capacity;
    vector->capacity *= 2;
    vector->data = (char** )realloc(vector->data, sizeof(char*) * vector->capacity);
    for(;i< vector->capacity; i++)
	vector->data[i] = (char *) malloc(sizeof(char)*numOfCols);
  }
}

void vector_free(Vector *vector) {
  //int i;
  //printf("%s", vector->data[0]);
  //free(vector->data[0]);
  //for(i=0; i< 1; i++) free(vector->data[i]);
  free(vector->data);
}
