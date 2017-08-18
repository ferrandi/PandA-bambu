/**
 * bzero primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
#include <stddef.h>

void __builtin_bzero (void *s, size_t n)
{
  memset(s, 0, n);
}
