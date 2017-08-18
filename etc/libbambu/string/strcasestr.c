/**
 * strcasestr primitive adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di Milano.
 * January, 27 2016.
 *
*/
/* Public domain.  */

static int __local_tolower(int c)
{
  if (c-'A' < 26) return c | 32;
  return c;
}

char *__builtin_strcasestr(const char *haystack, const char *needle)
{
  const char *h = haystack;
  const char *n = needle;

  while (*h && *n )
  {
    if ((*h == *n) || ( __local_tolower(*h) ==__local_tolower(*n)))
    {
      ++h;
      ++n;
    }
    else
    {
      h = ++haystack;
      n = needle;
    }
  }
  return (*n) ? 0 : (char *) haystack;
}

