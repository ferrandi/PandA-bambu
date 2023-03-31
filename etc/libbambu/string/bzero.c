/**
 * bzero primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
#include <stddef.h>

void bzero(void* s, size_t n)
{
   extern void* memset(void* dest, int val, size_t len);
   memset(s, 0, n);
}
