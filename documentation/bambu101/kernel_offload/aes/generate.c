#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "aes.h"

int main(int argc, char **argv) {
  struct bench_args_t data;
  uint8_t initial_contents[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  int i, fd;

  // Fill data structure
  for(i=0; i<32; i++)
    data.k[i] = i;
  memcpy(data.buf, initial_contents, 16);

  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );
  data_to_input(fd, &data);
  
  return 0;
}
