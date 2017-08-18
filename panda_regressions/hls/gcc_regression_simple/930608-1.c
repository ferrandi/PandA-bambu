double f (double a) {}
double (* const a[]) (double) = {&f};

main ()
{
#if (__GNUC__ != 4 || __GNUC_MINOR__ != 7)
  double (*p) ();
  p = &f;
  if (p != a[0])
    abort ();
#endif
  exit (0);
}
