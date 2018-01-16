/**
 * strstr primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
#include <stddef.h>

char *strstr(const char *haystack, const char *needle)
{
  const char *s = haystack;
  const char *p = needle;
  do
  {
    if (!*p) return (char *) haystack;
    if (*p == *s)
    {
      ++p;
      ++s;
    }
    else
    {
      p = needle;
      if (!*s) return 0;
      s = ++haystack;
    }
  } while (1);
}

