/**
 * memcmp,memcmp_eq primitives adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * September, 11 2013.
 *
 */
/* Public domain.  */
#include <stddef.h>

int memcmp(const void* str1, const void* str2, size_t count)
{
   const unsigned char* s1 = str1;
   const unsigned char* s2 = str2;

   while(count--)
   {
      if(*s1 != *s2)
         return *s1 - *s2;
      s1++;
      s2++;
   }
   return 0;
}

int memcmp_eq(const void* str1, const void* str2, size_t count)
{
   const unsigned char* s1 = str1;
   const unsigned char* s2 = str2;

   while(count-- > 0)
   {
      if(*s1++ != *s2++)
         return 1;
   }
   return 0;
}
