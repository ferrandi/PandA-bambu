/**
 * strdup primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */
#include <stddef.h>

char *__builtin_strdup(const char *s)
{
  const char *c = s, *d;
  size_t l;
  for (; *c ; c++);
  l = c-s+1;
  d = __builtin_malloc(l);
  if (!d) return 0;
  return __builtin_memcpy(d, s, l);
}

