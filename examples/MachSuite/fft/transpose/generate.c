#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "fft.h"
// Fake benchmark function to satisfy the extern
void fft1D_512(double data_x[512], double data_y[512]){ }

void generate_binary()
{
  struct bench_args_t data;
  char *ptr;
  int status, i, fd, written=0;

  // Fill data structure
  srandom(1);
  for(i=0; i<512; i++){
    data.work_x[i] = (double)random();
    data.work_y[i] = (double)random();
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
