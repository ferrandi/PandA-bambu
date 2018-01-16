/**
 * strdup primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */
#include <stddef.h>

char *strdup(const char *s)
{
  const char *c = s, *d;
  size_t l;
  for (; *c ; c++);
  l = c-s+1;
  d = malloc(l);
  if (!d) return 0;
  return memcpy(d, s, l);
}

