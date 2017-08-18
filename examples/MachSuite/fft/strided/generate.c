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
void fft(double real[size], double img[size], double real_twid[size], double img_twid[size]){ }

void generate_binary()
{
  struct bench_args_t data;
  char *ptr;
  int status, i, n, fd, written=0;
  double typed;

  // Fill data structure
  srandom(1);
  for(i=0; i<size; i++){
    data.real[i] = random();
    data.img[i] = random();
  }

  //Pre-calc twiddles
  for(n=0; n<(size>>1); n++){
      typed = (double)(twoPI*n/size);
      data.real_twid[n] = cos(typed);
      data.img_twid[n] = (-1.0)*sin(typed);
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
