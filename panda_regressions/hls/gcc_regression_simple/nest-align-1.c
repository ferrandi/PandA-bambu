/* Test for alignment handling when a variable is accessed by nested
   function.  */
/* Origin: Joey Ye <joey.ye@intel.com> */

#include <stddef.h>

typedef int aligned __attribute__((aligned));
extern void abort (void);

void
check (int *i)
{
  *i = 20;
  if ((((ptrdiff_t) i) & (__alignof__(aligned) - 1)) != 0)
    abort ();
}

#ifndef __clang__
void
foo (void)
{
  aligned jj;
  void bar ()
    {
      jj = -20;
    }
  jj = 0;
  bar ();
  if (jj != -20)
    abort ();
  check (&jj);
  if (jj != 20)
    abort ();
}
#endif

int
main()
{
#ifndef __clang__
  foo ();
#endif
  return 0;
}
