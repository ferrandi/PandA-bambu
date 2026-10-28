/**
 * memcpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */

#include <bambu_config.h>
#include <stddef.h>

static __FORCE_INLINE void* __local_memcpy(void* dest, const void* src, __uint64_t len)
{
   char* d = dest;
   const char *s = src, *s_end = s + len;
   while(s != s_end)
      *d++ = *s++;
   return dest;
}

void* memcpy(void* dest, const void* src, size_t len)
{
   return __local_memcpy(dest, src, len);
}

void _llvm_memcpy_p0i8_p0i8_i32(void* dest, const void* src, __uint32_t len, bool isVolatile)
{
   __local_memcpy(dest, src, len);
}

void _llvm_memcpy_p0i8_p0i8_i64(void* dest, const void* src, __uint64_t len, bool isVolatile)
{
   __local_memcpy(dest, src, len);
}
