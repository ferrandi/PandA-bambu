#include <math.h>

main()
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
   exit(0);
#else
  volatile double a;
  double c;
  a = 32.0;
  c = pow(a, 1.0/3.0);
  if (c + 0.1 > 3.174802
      && c - 0.1 < 3.174802)
    exit (0);
  else
    abort ();
#endif
}
