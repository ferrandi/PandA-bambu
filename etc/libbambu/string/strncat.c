/**
 * strncat primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */
#include <stddef.h>

char* strncat(char* __restrict dest, const char* __restrict src, size_t n)
{
   char* s = dest;
   while(*s++)
      ;
   --s;
   while(n && ((*s = *src++) != 0))
   {
      --n;
      ++s;
   }
   *s = 0;
   return dest;
}
