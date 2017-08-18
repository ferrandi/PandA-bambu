/**
 * strlen primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */
#include <stddef.h>

size_t __builtin_strlen(const char *s)
{
  const char *c = s;
  for (; *c ; c++);
  return c-s;
}
