#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "net.h"
// Fake benchmark function to satisfy the extern
void backprop(TYPE weights[NUM_LAYERS - 1][MAX_ROWS][MAX_COLS],
        TYPE inputs[NUM_TRAIN][SIZE_IN],
        TYPE targets[NUM_TRAIN][SIZE_OUT]) { }

void generate_binary()
{
    struct bench_args_t data;
    char *ptr;
    int status, i, j, k, fd, written=0;

    // Fill data structure
    srandom(1);

    for(i=0; i < NUM_LAYERS - 1; i++){
        for(j=0; j < MAX_ROWS; j++){
            for(k=k; k < MAX_COLS; k++){
                data.weights[i][j][k] = (((double)random())/((double)RAND_MAX)) - 0.5;
            }
        }
    }

    //Random input/outputs for now...
    for(i=0; i<NUM_TRAIN; i++){
        for(j=0; j<SIZE_IN - 1; j++){
            data.inputs[i][j] = (((double)random())/((double)RAND_MAX));
        }
        //biases included in weights...
        data.inputs[i][SIZE_IN] = 1.0;
        for(j=0; j < SIZE_OUT; j++){
            data.targets[i][j] = (((double)random())/((double)RAND_MAX));
        }
    } 

    // Open and write
    fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    assert( fd>0 && "Couldn't open input data file" );

    ptr = (char *) &data;
    while( written<sizeof(data) ) {
        status = write( fd, ptr, sizeof(data)-written );
        assert( status>=0 && "Couldn't write input data file" );
        written += status;
    }
}

int main(int argc, char **argv)
{
    generate_binary();
    return 0;
}
