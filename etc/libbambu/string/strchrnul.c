/**
 * strchrnul primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
 */
/* Public domain.  */

char* strchrnul(const char* s, int c)
{
   --s;
   while(*++s && (*s != c))
      ;
   return s;
}
