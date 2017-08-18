#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "kmp.h"
// Fake benchmark function to satisfy the extern
int kmp(char pattern[PATTERN_SIZE], char input[STRING_SIZE], int kmpNext[PATTERN_SIZE]) { }

void generate_binary()
{
  struct bench_args_t data;
  char *ptr;
  int status, i, fd, nbytes, written=0;

  // Load string file
  fd = open("TR.txt", O_RDONLY);
  assert( fd>=0 && "couldn't open text file" );
  nbytes = 0;
  printf("text\n");
  do {
    status = read(fd, data.input, STRING_SIZE-nbytes);
    assert(status>=0 && "couldn't read from text file");
    nbytes+=status;
  } while( nbytes<STRING_SIZE );
  close(fd);
  printf("done text\n");
  // Fill remaining data structure
  memcpy(data.pattern, "bull", PATTERN_SIZE);
  /*data.outs = 0;*/
  for (i = 0; i < PATTERN_SIZE; i++)
    data.kmpNext[i] = 0;

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
