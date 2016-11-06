#define VECTOR_INITIAL_CAPACITY 300

// default is a 2d array of dimensions Nx2. column size MAX_BROADCAST_LENGTH(100)

// Define a vector type
typedef struct {
  int size;      // slots used so far
  int capacity;  // total available slots
  char** data;     // array of integers we're storing
} Vector;

void vector_init(Vector *vector, int cols);

void vector_append(Vector *vector, char* value);

char* vector_get(Vector *vector, int index);

void vector_set(Vector *vector, int index, char*);

void vector_double_capacity_if_full(Vector *vector);

void vector_free(Vector *vector);

int vector_get_index(Vector *vector,char* value);
