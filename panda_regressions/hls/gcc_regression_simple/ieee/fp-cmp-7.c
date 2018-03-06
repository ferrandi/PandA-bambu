extern void link_error ();

#ifndef __llvm__
void foo(double x)
{
  if (x > __builtin_inf())
    link_error ();
}
#endif
int main ()
{
#ifndef __llvm__
  foo (1.0);
#endif
  return 0;
}

