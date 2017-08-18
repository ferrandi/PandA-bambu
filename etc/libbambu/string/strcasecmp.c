/**
 * strcasecmp primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

static int __local_tolower(int c)
{
  if (c-'A' < 26) return c | 32;
  return c;
}

int __builtin_strcasecmp(const char *s1, const char *s2)
{
  for (; *s1 && *s2 && (*s1 == *s2 || __local_tolower(*s1) == __local_tolower(*s2)); s1++, s2++);
  return __local_tolower(*s1) - __local_tolower(*s2);
}
