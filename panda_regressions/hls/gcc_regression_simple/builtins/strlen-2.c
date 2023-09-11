/* Copyright (C) 2003  Free Software Foundation.

   Test strlen optimizations on conditional expressions.
      
   Written by Jakub Jelinek, June 23, 2003.  */

#include <stddef.h>
extern size_t strlen (const char *);
extern char *strcpy (char *, const char *);
extern int memcmp (const void *, const void *, size_t);
extern void abort (void);

size_t g = 0, h = 0, i = 0, j = 0, k = 0, l = 0;

size_t
foo (void)
{
  if (l)
    abort ();
  return ++l;
}

void
main_test (void)
{
  if (strlen (i ? "foo" + 1 : j ? "bar" + 1 : "baz" + 1) != 2)
    abort ();
  if (strlen (g++ ? "foo" : "bar") != 3 || g != 1)
    abort ();
  if (strlen (h++ ? "xfoo" + 1 : "bar") != 3 || h != 1)
    abort ();
  if (strlen ((i++, "baz")) != 3 || i != 1)
    abort ();
  /* The following calls might not optimize strlen call away.  */
  if (strlen (j ? "foo" + k++ : "bar" + k++) != 3 || k != 1)
    abort ();
  if (strlen (foo () ? "foo" : "bar") != 3 || l != 1)
    abort ();
}
int main()
{
  main_test();
  return 0;
}
