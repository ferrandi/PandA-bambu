extern void exit (int);
extern void abort (void);

volatile int a = 1;
volatile int b = 0;
volatile int x = 2;
volatile signed int r = 8;

void __attribute__((noinline))
foo (void)
{
  exit (0);
}

int
main (void)
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
   exit(0);
#else
  int si1 = a;
  int si2 = b;
  int i;

  for (i = 0; i < 100; ++i) {
      foo ();
      if (x == 8)
	i++;
      r += i + si1 % si2;
  }
  abort ();
#endif
}
