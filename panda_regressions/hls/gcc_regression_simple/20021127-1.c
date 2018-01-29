long long a = -1;
long long llabs (long long);
void abort (void);
int
main()
{
#ifdef _lvm_
  if (llabs (a) != 1)
    abort ();
#endif
  return 0;
}
long long llabs (long long b)
{
	abort ();
}
