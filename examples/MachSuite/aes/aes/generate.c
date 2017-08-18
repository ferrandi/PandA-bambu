#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "aes.h"
// Fake benchmark function to satisfy the extern
void aes256_encrypt_ecb(aes256_context *ctx, uint8_t k[32], uint8_t buf[16]){}

void generate_binary()
{
  struct bench_args_t data;
  uint8_t initial_contents[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  char *ptr;
  int status, i, fd, written=0;

  // Fill data structure
  memset(&data.ctx, 0, sizeof(aes256_context));
  srandom(1);
  for(i=0; i<32; i++)
    data.k[i] = i;
  memcpy(data.buf, initial_contents, 16);

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
