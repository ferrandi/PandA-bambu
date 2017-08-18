double d;

double foo (void);
__inline__ double foo (void)
{
  return d;
}

int bar (void);
__inline__ int bar (void)
{
  foo();
  return 0;
}

int main (void)
{
  if (bar ())
    abort ();
  exit (0);
}
