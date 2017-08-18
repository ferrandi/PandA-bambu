/**
 * stpcpy primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
char *__builtin_stpcpy(char * __restrict s1, char * __restrict s2)
{
  while ( (*s1++ = *s2++) != 0 );
  return s1 - 1;
}

