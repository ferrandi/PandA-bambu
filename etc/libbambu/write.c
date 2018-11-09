/**
 * write primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * June, 4 2015.
 *
 */
#include <stddef.h>
#include <unistd.h>
void __bambu_writec(int fd, signed char);

ssize_t write(int fd, const void* __restrict buf, size_t count)
{
   int index;
   for(index = 0; index < count; ++index)
   {
      __bambu_writec(fd, ((signed char*)buf)[index]);
   }
   return index;
}
