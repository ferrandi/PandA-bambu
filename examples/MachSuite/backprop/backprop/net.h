#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define TYPE double

//Number of times you train the network with the same data set
#define EPOCS 100

//Number of training data
#define NUM_TRAIN 100

#define MAX_ROWS 10
#define MAX_COLS 10

#define NUM_LAYERS 3

//Number of acspects for each training data
#define SIZE_IN 4

//Classification catagories (output nodes)
#define SIZE_OUT 10

//Learning Rate
#define N 0.5

//Momentum
#define M 0.1

static const int layer_size[] = {SIZE_IN, MAX_COLS, SIZE_OUT};

void backprop(TYPE weights[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS], 
        TYPE inputs[NUM_TRAIN][SIZE_IN], 
        TYPE targets[NUM_TRAIN][SIZE_OUT]);

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
        TYPE weights[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS];
        TYPE inputs[NUM_TRAIN][SIZE_IN];
        TYPE targets[NUM_TRAIN][SIZE_OUT];
};
int INPUT_SIZE = sizeof(struct bench_args_t);


void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  backprop( args->weights, args->inputs, args->targets );
}

////////////////////////////////////////////////////////////////////////////////
