/**
 * memchr primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
#include <stddef.h>

void* memchr(const void* s, int c, size_t n)
{
   const unsigned char* r = (const unsigned char*)s;
   while(n)
   {
      if(*r == ((unsigned char)c))
         return (void*)r;
      ++r;
      --n;
   }
   return 0;
}
