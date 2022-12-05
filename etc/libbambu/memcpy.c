/**
 * memcpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */
#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t len)
{
   char* d = dest;
   const char* s = src;
   while(len--)
      *d++ = *s++;
   return dest;
}

void* __internal_bambu_memcpy(void* dest, const void* src, size_t len)
{
   char* d = dest;
   const char* s = src;
   while(len--)
      *d++ = *s++;
   return dest;
}

#ifdef __llvm__

void _llvm_memcpy_p0i8_p0i8_i32(void* dest, const void* src, unsigned int len,
#if __clang_major__ < 13
                                unsigned int align,
#endif
                                _Bool isvolatile)
{
   char* d = dest;
   const char* s = src;
   while(len--)
      *d++ = *s++;
}

void _llvm_memcpy_p0i8_p0i8_i64(void* dest, const void* src, unsigned long long int len,
#if __clang_major__ < 13
                                unsigned int align,
#endif
                                _Bool isvolatile)
{
   char* d = dest;
   const char* s = src;
   while(len--)
      *d++ = *s++;
}

#endif
