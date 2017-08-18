volatile double a, *p;

int main ()
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
#else
  double c, d;
  volatile double b;

  d = 1.0;
  p = &b;
  do
  {
    c = d;
    d = c * 0.5;
    b = 1 + d;
  } while (b != 1.0);

  a = 1.0 + c;
  if (a == 1.0)
    abort();
#endif
  exit (0);
}
