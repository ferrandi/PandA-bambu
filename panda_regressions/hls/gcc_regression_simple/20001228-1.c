int foo1(void)
{
  union {
    char a[sizeof (unsigned)];
    unsigned b;
  } u;
  
  u.b = 0x01;
  return u.a[0];
}

int foo2(void)
{
  volatile union {
    char a[sizeof (unsigned)];
    unsigned b;
  } u;
  
  u.b = 0x01;
  return u.a[0];
}

int main(void)
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
  exit (0);
#else
  if (foo1() != foo2())
    abort ();
  exit (0);
#endif
}
