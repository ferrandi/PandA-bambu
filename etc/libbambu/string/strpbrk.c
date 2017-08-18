/**
 * strpbrk primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

char *__builtin_strpbrk(const char *s, const char *accept)
{
  for ( ; *s ; s++ )
  {
    for ( ; *accept ; accept++ )
    {
      if (*accept == *s) return s;
    }
  }
  return 0;
}
