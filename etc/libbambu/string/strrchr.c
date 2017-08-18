/**
 * strrchr,rindex primitives adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

char *__builtin_strrchr(const char *s, int c)
{
  const char *p = 0;
  do
  {
    if (*s == c) p = s;
  } while (*s++);
  return p;
}

char *__builtin_rindex(const char *s, int c)
{
  const char *p = 0;
  do
  {
    if (*s == c) p = s;
  } while (*s++);
  return p;
}

