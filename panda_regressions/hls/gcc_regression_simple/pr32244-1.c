struct foo
{
  unsigned long long b:40;
} x;

extern void abort (void);

void test1(unsigned long long res)
{
  /* The shift is carried out in 40 bit precision.  */
  if (x.b<<32 != res)
    abort ();
}

int main()
{
#ifndef __clang__
  x.b = 0x0100;
  test1(0);
#endif
  return 0;
}
