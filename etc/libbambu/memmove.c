/**
 * memmove primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */
#include <stddef.h>

void* memmove(void* dest, const void* src, size_t len)
{
   char* d = dest;
   const char* s = src;
   if(d < s)
      while(len--)
         *d++ = *s++;
   else
   {
      char* lasts = s + (len - 1);
      char* lastd = d + (len - 1);
      while(len--)
         *lastd-- = *lasts--;
   }
   return dest;
}

#ifdef __llvm__

void _llvm_memmove_p0i8_p0i8_i32(void* dest, const void* src, size_t len,
#if __clang_major__ < 7
                                 unsigned int align,
#endif
                                 _Bool isvolatile)
{
   char* d = dest;
   const char* s = src;
   if(d < s)
      while(len--)
         *d++ = *s++;
   else
   {
      char* lasts = s + (len - 1);
      char* lastd = d + (len - 1);
      while(len--)
         *lastd-- = *lasts--;
   }
}

void _llvm_memmove_p0i8_p0i8_i64(void* dest, const void* src, unsigned long long int len,
#if __clang_major__ < 7
                                 unsigned int align,
#endif
                                 _Bool isvolatile)
{
   char* d = dest;
   const char* s = src;
   if(d < s)
      while(len--)
         *d++ = *s++;
   else
   {
      char* lasts = s + (len - 1);
      char* lastd = d + (len - 1);
      while(len--)
         *lastd-- = *lasts--;
   }
}

#endif
