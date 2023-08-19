#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define WRITE_OUTPUT
#define CHECK_OUTPUT

#include "support.h"

int main(int argc, char** argv)
{
   // Parse command line.
   char* in_file;
   assert(argc < 3 && "Usage: ./benchmark <input_file>");
   in_file = "input.data";
   if(argc > 1)
      in_file = argv[1];

   // Load input data
   int in_fd;
   char* data;
   data = malloc(INPUT_SIZE);
   assert(data != NULL && "Out of memory");
   in_fd = open(in_file, O_RDONLY);
   assert(in_fd > 0 && "Couldn't open input data file");
   input_to_data(in_fd, data);

   // Unpack and call
   run_benchmark(data);

#ifdef WRITE_OUTPUT
   int out_fd;
   out_fd =
       open("output.data", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
   assert(out_fd > 0 && "Couldn't open output data file");
   data_to_output(out_fd, data);
   close(out_fd);
#endif

// Validate benchmark results
#ifdef CHECK_OUTPUT
   if(!check_data(data))
   {
      fprintf(stderr, "Benchmark results are incorrect\n");
      return -1;
   }
#endif
   free(data);

   printf("Success.\n");
   return 0;
}
