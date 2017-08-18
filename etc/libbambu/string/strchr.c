/**
 * strchr,index primitives adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

char *__builtin_strchr(const char *s, int c)
{
  do
  {
    if (*s == c) return s;
  } while (*s++);
  return 0;
}

char *__builtin_index(const char *s, int c)
{
  do
  {
    if (*s == c) return s;
  } while (*s++);
  return 0;
}

