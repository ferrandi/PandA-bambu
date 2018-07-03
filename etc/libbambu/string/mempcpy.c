/**
 * mempcpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
#include <stddef.h>

void *
mempcpy(void * dest, const void * src, size_t n)
{
  char *p1 = dest;
  const char *p2 = src;

  while (n)
  {
    *p1++ = *p2++;
    --n;
  }
  return p1;
}

