/**
 * strcpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

char *strcpy(char *__restrict dest, const char *__restrict src)
{
  const unsigned char *s = src;
  unsigned char *d = dest;
  while (*d++ = *s++);
  return dest;
}
