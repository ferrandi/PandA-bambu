/**
 * strcat primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

char *strcat(char *__restrict dest, const char *__restrict src)
{
  char *s = dest;
  while (*s++);
  --s;
  while ((*s++ = *src++) != 0);
  return dest;
}

