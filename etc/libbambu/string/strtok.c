/**
 * strtok_r and strtok primitives adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */

#include <stddef.h>

char* strtok_r(char* __restrict s1, const char* __restrict s2, char** __restrict next_start)
{
   char* s=0;
   char* p=0;
   extern size_t strspn(const char *s, const char *accept);
   extern char *strpbrk(const char *s, const char *accept);
   if(((s = s1) != 0) || ((s = *next_start) != 0))
   {
      if(*(s += strspn(s, s2)))
      {
         if((p = strpbrk(s, s2)) != 0)
         {
            *p++ = 0;
         }
      }
      else
      {
         p = s = 0;
      }
      *next_start = p;
   }
   return s;
}

char* strtok(char* __restrict str, const char* __restrict delim)
{
   static char* next_start;
   extern char *strtok_r(char *str, const char *delim, char **saveptr);
   return strtok_r(str, delim, &next_start);
}
