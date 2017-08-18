/**
 * memrchr primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
#include <stddef.h>

void *__builtin_memrchr(const void *s, int c, size_t n)
{
  const unsigned char *r;
  r = ((unsigned char *)s) + ((size_t) n);
  while (n)
  {
    if (*--r == ((unsigned char)c))
      return (void *) r;
    --n;
  }
  return 0;
}

