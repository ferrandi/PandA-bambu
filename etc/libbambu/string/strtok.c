/**
 * strtok_r and strtok primitives adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */

char* strtok_r(char* __restrict s1, const char* __restrict s2, char** __restrict next_start)
{
   char* s;
   char* p;
   if(((s = s1) != 0) || ((s = *next_start) != 0))
   {
      if(*(s += strspn(s, s2)))
      {
         if((p = strpbrk(s, s2)) != 0)
            *p++ = 0;
      }
      else
         p = s = 0;
      *next_start = p;
   }
   return s;
}

char* strtok(char* __restrict str, const char* __restrict delim)
{
   static char* next_start;
   return strtok_r(str, delim, &next_start);
}
