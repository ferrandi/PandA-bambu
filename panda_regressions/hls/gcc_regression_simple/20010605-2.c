void foo (), bar (), baz ();
int main ()
{
  __complex__ double x;
  __complex__ float y;
  __real__ x = 1.0;
  __imag__ x = 2.0;
  foo (x);
  __real__ y = 3.0f;
  __imag__ y = 4.0f;
  bar (y);
  exit (0);
}

void foo (__complex__ double x)
{
  if (__real__ x != 1.0 || __imag__ x != 2.0)
    abort ();
}

void bar (__complex__ float x)
{
  if (__real__ x != 3.0f || __imag__ x != 4.0f)
    abort ();
}


