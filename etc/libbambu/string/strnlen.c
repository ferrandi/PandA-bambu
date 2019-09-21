/**
 * strnlen primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */
#include <stddef.h>

size_t strnlen(const char* s, size_t maxlen)
{
   const char* saved = s;
   while(maxlen-- > 0 && *s)
      s++;
   return s - saved;
}
