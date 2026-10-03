#include "icrc.h"
#include <stdio.h>

unsigned short main()
{
   unsigned short crc = 245;
   unsigned char onech = 5;

   unsigned short res = icrc1(crc, onech);

   crc = 134;
   onech = 3;

   res += icrc1(crc, onech);

   printf("res = %d\n", res);
   
   return res;
}