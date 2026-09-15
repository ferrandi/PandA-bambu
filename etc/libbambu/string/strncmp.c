/**
 * strncmp primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * October, 22 2024.
 *
 */
/* Public domain.  */

#include <stddef.h>

int strncmp(const char* str1, const char* str2, size_t n)
{
   while(n && *str1 && (*str1 == *str2))
   {
      str1++;
      str2++;
      n--;
   }
   if(n == 0)
   {
      return 0;
   }
   return *(unsigned char*)str1 - *(unsigned char*)str2;
}
