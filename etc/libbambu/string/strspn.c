/**
 * strspn primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */
#include <stddef.h>

size_t __builtin_strspn(const char *s, const char *accept)
{
  const char *saved = s;
  const char *p = accept;

  while (*p)
  {
    if (*p++ == *s)
    {
      ++s;
      p = accept;
    }
  }
  return s - saved;
}

