unsigned char x = 50;
volatile short y = -5;

int main ()
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
   exit(0);
#else
  x /= y;
  if (x != (unsigned char) -10)
    abort ();
  exit (0);
#endif
}
