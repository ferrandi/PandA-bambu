/**
 * strncasecmp primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */
#include <stddef.h>

static int __local_tolower(int c)
{
  if (c-'A' < 26) return c | 32;
  return c;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
  if (!n--) return 0;
  for (; *s1 && *s2 && n && (*s1 == *s2 || __local_tolower(*s1) == __local_tolower(*s2)); s1++, s2++, n--);
  return __local_tolower(*s1) - __local_tolower(*s2);
}
