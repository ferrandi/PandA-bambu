#include "read.h"

int main()
{
   short int A[20];

   for(int i = 0; i < 20; i++)
   {
      A[i] = (short int)i;
   }

   return read(A);
}