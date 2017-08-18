/**
 * strcspn primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */
#include <stddef.h>

size_t __builtin_strcspn(const char *s, const char *reject)
{
  const char *saved=s;
  for ( ; *s ; s++)
  {
    for ( ; *reject ; reject++ )
    {
      if (*reject == *s) return s - saved;
    }
  }
  return s - saved;
}


