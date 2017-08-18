int a[2];

void f (int b, int o);
__inline__ void f (int b, int o)
{
  unsigned int i;
  int *p;
  for (p = &a[b], i = b; --i < ~0; )
    *--p = i * 3 + o;
}

g(int b)
{
  f (b, (int)a);
}

main ()
{
  a[0] = a[1] = 0;
  g (2);
  if (a[0] != (int)a || a[1] != (int)a + 3)
    abort ();
  exit (0);
}
