/**
 * stpncpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
#include <stddef.h>

char* stpncpy(char* __restrict s1, const char* __restrict s2, size_t n)
{
   char* s = s1;
   const char* p = s2;
   while(n)
   {
      if((*s = *s2) != 0)
         s2++;
      ++s;
      --n;
   }
   return s1 + (s2 - p);
}
