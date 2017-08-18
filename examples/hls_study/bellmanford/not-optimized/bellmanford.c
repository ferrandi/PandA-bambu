
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* Let INFINITY be the maximum value for an int */

typedef struct {
    int source;
    int dest;
    int weight;
} Edge;

#define EXT_DATASET
#ifndef EXT_DATASET
#define N_dest 10
#define N_dist 5
#else
#define N_dest 21
#define N_dist 15
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#define INFINITY INT_MAX

typedef enum {false=0,true=1} bool;

#pragma map generate_hw 0
bool
__attribute__ ((noinline))  
bellmanford(int source[N_dest], int dest[N_dest], int weight [N_dest], int distance[N_dist], int edgecount, int nodecount, int src)
{
    int i, j;
    bool succeeded = true;

    for (i=0; i < nodecount; ++i)
      distance[i] = INFINITY;
    distance[src] = 0;

    for (i=0; i < nodecount; ++i) {
        for (j=0; j < edgecount; ++j) {
            if (distance[source[j]] != INFINITY) {
                int new_distance = distance[source[j]] + weight[j];
                if (new_distance < distance[dest[j]])
                  distance[dest[j]] = new_distance;
            }
        }
    }

    for (i=0; i < edgecount; ++i) {
        if (distance[dest[i]] > distance[source[i]] + weight[i]) {
            succeeded = false;
            i = edgecount;
        }
    }

    return succeeded;
}

int main(void)
{
    int *source, *dest, *weight, *dist, i, edgecount, nodecount, src, success;

#ifndef EXT_DATASET
    edgecount = 10;
    nodecount = 5;
    src = 0; // value should be 0< & <5

    source = (int*) malloc( 10 * sizeof( int ) );
    dest = (int*) malloc( edgecount * sizeof( int ) );
    weight = (int*) malloc( edgecount * sizeof( int ) );
    dist = (int*) malloc( nodecount * sizeof( int ) );

    source[0] = 0; dest[0] = 1; weight[0] = 5;
    source[1] = 0; dest[1] = 2; weight[1] = 8;
    source[2] = 0; dest[2] = 3; weight[2] = -4;
    source[3] = 1; dest[3] = 0; weight[3] = -2;
    source[4] = 2; dest[4] = 1; weight[4] = -3;
    source[5] = 2; dest[5] = 3; weight[5] = 9;
    source[6] = 3; dest[6] = 1; weight[6] = 7;
    source[7] = 3; dest[7] = 4; weight[7] = 2;
    source[8] = 4; dest[8] = 0; weight[8] = 6;
    source[9] = 4; dest[9] = 2; weight[9] = 7;
#else
    edgecount = 21;
    nodecount = 15;
    src = 0;

    source = (int*) malloc( 21 * sizeof( int ) );
    dest = (int*) malloc( edgecount * sizeof( int ) );
    weight = (int*) malloc( edgecount * sizeof( int ) );
    dist = (int*) malloc( nodecount * sizeof( int ) );

    source[0] = 0; dest[0] = 2; weight[0] = 2;
    source[1] = 0; dest[1] = 13; weight[1] = 30;
    source[2] = 1; dest[2] = 12; weight[2] = -2;
    source[3] = 2; dest[3] = 1; weight[3] = -1;
    source[4] = 2; dest[4] = 11; weight[4] = 9;
    source[5] = 3; dest[5] = 13; weight[5] = -10;
    source[6] = 3; dest[6] = 10; weight[6] = 7;
    source[7] = 5; dest[7] = 3; weight[7] = 3;
    source[8] = 5; dest[8] = 6; weight[8] = 2;
    source[9] = 6; dest[9] = 8; weight[9] = 3;
    source[10] = 7; dest[10] = 6; weight[10] = -4;
    source[11] = 7; dest[11] = 8; weight[11] = 6;
    source[12] = 8; dest[12] = 7; weight[12] = 3;
    source[13] = 9; dest[13] = 7; weight[13] = 2;
    source[14] = 9; dest[14] = 4; weight[14] = -5;
    source[15] = 10; dest[15] = 11; weight[15] = 1;
    source[16] = 10; dest[16] = 14; weight[16] = 2;
    source[17] = 11; dest[17] = 5; weight[17] = 8;
    source[18] = 12; dest[18] = 0; weight[18] = -3;
    source[19] = 13; dest[19] = 9; weight[19] = 3;
    source[20] = 14; dest[20] = 1; weight[20] = 6;

#endif

    #pragma map call_hw VIRTEX5 0
    success = bellmanford(source, dest, weight, dist, edgecount, nodecount, src);
    
#ifndef EXT_DATASET
    if (success != true)
	printf("FAIL!\n");
    else
	printf("PASS!\n");
#else
    if (success != false)
	printf("FAIL!\n");
    else
	printf("PASS!\n");
#endif

    return 0;
}
