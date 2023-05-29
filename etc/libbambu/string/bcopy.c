/**
 * bcopy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
#include <stddef.h>

void bcopy(const void* src, void* dest, size_t n)
{
   extern void *memmove(void *dest, const void *src, size_t n);
   memmove(dest, src, n);
}
