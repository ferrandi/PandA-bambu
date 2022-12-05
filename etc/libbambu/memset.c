/**
 * memset primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */
#include <stddef.h>

void* memset(void* dest, int val, size_t len)
{
   unsigned char* ptr = dest;
   while(len-- > 0)
      *ptr++ = val;
   return dest;
}

void* __internal_bambu_memset(void* dest, int val, size_t len)
{
   volatile unsigned char* ptr = dest;
   while(len-- > 0)
      *ptr++ = val;
   return dest;
}

#ifdef __llvm__

void _llvm_memset_p0i8_i32(void* dest, unsigned char val, size_t len,
#if __clang_major__ < 7
                           unsigned int align,
#endif
                           _Bool isvolatile)
{
   volatile unsigned char* ptr = dest;
   while(len-- > 0)
      *ptr++ = val;
}

void _llvm_memset_p0i8_i64(void* dest, unsigned char val, unsigned long long int len,
#if __clang_major__ < 7
                           unsigned int align,
#endif
                           _Bool isvolatile)
{
   volatile unsigned char* ptr = dest;
   while(len-- > 0)
      *ptr++ = val;
}

#endif
