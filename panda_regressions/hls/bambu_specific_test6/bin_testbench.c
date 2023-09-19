#include "complex_t.h"

unsigned kernel(complex_t* a_ptr)
{
   return a_ptr->value[1];
}

#ifndef __BAMBU__
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main()
{
   complex_t my_struct = {
       10, 10, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {1, 4, 5, 6, 8, 9}, {0, 2, 3, 3, 0, 4}};

   const char* filename = "a_ptr.dat";
   FILE* f = fopen(filename, "wb");
   if(f != NULL)
   {
      fwrite(&my_struct, sizeof(my_struct), 1, f);
      if(fflush(f) == EOF)
      {
         perror("Failed to write the file");
      }
      fclose(f);
   }
   else
   {
      perror("Failed to open file\n");
   }

   return 0;
}
#endif
