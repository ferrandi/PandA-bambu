#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "stencil3d.h"
// Fake benchmark function to satisfy the extern
void stencil3d( TYPE C0, TYPE C1, TYPE orig[row_size * col_size * height_size],
                TYPE sol[row_size * col_size * height_size] ) { }

void generate_binary()
{
  struct bench_args_t data;
  char *ptr;
  int status, i, fd, written=0;

  // Fill data structure
  srandom(1);
  data.C0 = random();
  data.C1 = random();
  for(i=0; i<row_size*col_size*height_size; i++)
    data.orig[i] = random();
  memset(data.sol, 0, row_size*col_size*height_size*sizeof(int));

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
