/**
 * memset primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */

#include <stddef.h>

void* memset(void* dest, int val, size_t len)
{
   unsigned char *ptr = dest, *ptr_end = ((unsigned char*)dest) + len;
   while(ptr != ptr_end)
      *ptr++ = val;
   return dest;
}
