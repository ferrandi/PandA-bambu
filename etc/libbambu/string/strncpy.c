/**
 * strncpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */
#include <stddef.h>

char* strncpy(char* __restrict dest, const char* __restrict src, size_t n)
{
   char* d = dest;
   while(n)
   {
      if((*d = *src) != 0)
         src++;
      ++d;
      --n;
   }
   return dest;
}
