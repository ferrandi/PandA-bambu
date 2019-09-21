/**
 * strndup primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */
#include <stddef.h>

char* strndup(const char* s, size_t n)
{
   const char* saved = s;
   char* d;
   size_t l;
   while(n-- > 0 && *s)
      s++;
   l = s - saved;
   d = malloc(l + 1);
   if(!d)
      return 0;
   d[l] = 0;
   memcpy(d, saved, l);
   return d;
}
