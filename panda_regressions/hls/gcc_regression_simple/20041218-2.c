#ifndef __llvm__
extern void abort (void);

int test(int n)
{
  struct s { char b[n]; };
  n++;
  return sizeof(struct s);
}

int main()
{
  if (test(123) != 123)
    abort ();
  return 0;
}
#else
int main()
{
  return 0;
}
#endif
