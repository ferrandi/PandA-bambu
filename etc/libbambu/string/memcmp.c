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
   const unsigned char *s2 = str2, *s2_end = ((const unsigned char*)str2) + count;
   unsigned char diff = 0;
   while(s2 != s2_end)
   {
      if((diff = *s1++ - *s2++))
         return diff;
   }
   return diff;
}

int memcmp_eq(const void* str1, const void* str2, size_t count)
{
   const unsigned char* s1 = str1;
   const unsigned char *s2 = str2, *s2_end = ((const unsigned char*)str2) + count;

   while(s2 != s2_end)
   {
      if(*s1++ != *s2++)
         return 1;
   }
   return 0;
}
