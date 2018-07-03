struct foo
{
  unsigned long long b:40;
} x;

extern void abort (void);

void test1(unsigned long long res)
{
  /* Build a rotate expression on a 40 bit argument.  */
  if ((x.b<<8) + (x.b>>32) != res)
    abort ();
}

int main()
{
#ifndef __clang__
  x.b = 0x0100000001;
  test1(0x0000000101);
  x.b = 0x0100000000;
  test1(0x0000000001);
#endif
  return 0;
}
