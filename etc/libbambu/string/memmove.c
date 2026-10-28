/**
 * memmove primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */

#include <stddef.h>

void* memmove(void* dest, const void* src, size_t len)
{
   char* d = dest;
   const char* s = src;
   if(d < s)
   {
      char* d_end = d + len;
      while(d != d_end)
         *d++ = *s++;
   }
   else
   {
      char* lasts = s + (len - 1);
      char *lastd = d + (len - 1), *d_rend = d - 1;
      while(lastd != d_rend)
         *lastd-- = *lasts--;
   }
   return dest;
}
