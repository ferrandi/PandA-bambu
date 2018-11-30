/**
 * strsep primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */

char* strsep(char** stringp, const char* delim)
{
   char* s = *stringp;
   char* saved = s;
   if(s)
   {
      for(; *s; s++)
      {
         for(; *delim; delim++)
         {
            if(*delim == *s)
            {
               *s++ = 0;
               *stringp = s;
               return saved;
            }
         }
      }
   }
   *stringp = 0;
   return saved;
}
