extern void abort ();

int f(int x)
{
  return (x >> (sizeof (x) * __CHAR_BIT__ - 1)) ? -1 : 1;
}

volatile int one = 1;
int main (void)
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
   return 0;
#else
  /* Test that the function above returns different values for
     different signs.  */
  if (f(one) == f(-one))
    abort ();
  return 0;
#endif
}

