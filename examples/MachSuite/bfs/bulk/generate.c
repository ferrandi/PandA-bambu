#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

// R-MAT parameters (*100)
#if 1
// Scale-free, small-world graph
#define A 57
#define B 19
#define C 19
#define D 5
#else
// Erdos-Renyi, uniform random graph
// For reference. Do not use.
#define A 25
#define B 25
#define C 25
#define D 25
#endif

#include "bulk.h"
// Fake benchmark function to satisfy the extern

void bfs(node_t nodes[N_NODES], edge_t edges[N_EDGES], node_index_t starting_node, level_t level[N_NODES], edge_index_t level_counts[N_LEVELS]) { }

void generate_binary()
{
  struct bench_args_t data;
  char *ptr;
  int status, i, fd, written=0;
  node_index_t adjmat[N_NODES][N_NODES]; // This is small enough to be fine.
  node_index_t r,c,s,temp;
  edge_index_t e;
  int scale;
  long int rint;

  // Generate dense R-MAT matrix
  memset(adjmat, 0, N_NODES*N_NODES*sizeof(node_index_t));
  srandom(1);
  
  e = 0;
  while( e<N_EDGES/2 ) { // generate N_EDGES/2 undirected edges (N_EDGES directed)
    r = 0;
    c = 0;
    // Pick a random edge according to R-MAT parameters
    for( scale=SCALE; scale>0; scale-- ) { // each level of the quadtree
      rint = random()%100;
      if( rint>=(A+B) ) // C or D (bottom half)
        r += 1<<(scale-1);
      if( (rint>=A && rint<A+B) || (rint>=A+B+C) ) // B or D (right half)
        c += 1<<(scale-1);
    }
    if( adjmat[r][c]==0 && r!=c ) { // ignore self-edges, they're irrelevant
      // We make undirected edges
      adjmat[r][c]=1;
      adjmat[c][r]=1;
      ++e;
    }
  }

  // Shuffle matrix (to eliminate degree locality)
  for( s=0; s<N_NODES; s++ ) {
    rint = random()%N_NODES;
    // Swap row s with row rint
    for( r=0; r<N_NODES; r++ ) {
      for( c=0; c<N_NODES; c++ ) {
        temp = adjmat[r][c];
        adjmat[r][c] = adjmat[rint][c];
        adjmat[rint][c] = temp;
      }
    }
    // Swap col s with col rint (to keep symmetry)
    for( c=0; c<N_NODES; c++ ) {
      for( r=0; r<N_NODES; r++ ) {
        temp = adjmat[r][c];
        adjmat[r][c] = adjmat[r][rint];
        adjmat[r][rint] = temp;
      }
    }
  }

  // Scan rows for edge list lengths, and fill edges while we're at it
  e = 0;
  for( r=0; r<N_NODES; r++ ) { // count first
    data.nodes[r].edge_begin = 0;
    data.nodes[r].edge_end = 0;
    for( c=0; c<N_NODES; c++ ) {
      if( adjmat[r][c] ) {
        ++data.nodes[r].edge_end;
        data.edges[e].dst = c;
        //data.edges[e].weight = random()%(MAX_WEIGHT-MIN_WEIGHT)+MIN_WEIGHT;
        ++e;
      }
    }
  }

  for( r=1; r<N_NODES; r++ ) { // now scan
    data.nodes[r].edge_begin = data.nodes[r-1].edge_end;
    data.nodes[r].edge_end += data.nodes[r-1].edge_end;
  }

  // Pick starting node
  do {
    rint = random()%N_NODES;
  } while( (data.nodes[rint].edge_end-data.nodes[rint].edge_begin)<2 );
  data.starting_node = rint;

  // Fill data structure
  memset(data.level, MAX_LEVEL, N_NODES*sizeof(level_t));
  memset(data.level_counts, 0, N_LEVELS*sizeof(edge_index_t));

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
