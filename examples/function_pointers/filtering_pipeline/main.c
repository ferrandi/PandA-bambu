#include "mypgm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test(unsigned char* in, unsigned char* out, unsigned int x_size, unsigned int y_size, int type);

int main(int argc, char* argv[])
{
   unsigned char image1[MAX_IMAGESIZE * MAX_IMAGESIZE], image2[MAX_IMAGESIZE * MAX_IMAGESIZE];
   char out_filename[MAX_FILENAME];
   char* dot;
   int in_basename_len;
   unsigned int x_size1, y_size1, i;

   if(argc < 2)
   {
      printf("usage: %s <input.pgm>\n", argv[0]);
      exit(-1);
   }
   dot = strrchr(argv[1], '.');
   if(!dot || strcmp(dot, ".pgm"))
   {
      printf("Input file is not a pgm file.\n");
      exit(-1);
   }
   in_basename_len = strlen(argv[1]) - 4;

   load_image_data(argv[1], image1, &x_size1, &y_size1);
   for(i = 0; i < 3; ++i)
   {
      test(image1, image2, x_size1, y_size1, i);
      sprintf(out_filename, "%.*s.pipe%d.pgm\0", in_basename_len, argv[1], i);
      save_image_data(out_filename, image2, x_size1, y_size1);
   }

   return 0;
}
