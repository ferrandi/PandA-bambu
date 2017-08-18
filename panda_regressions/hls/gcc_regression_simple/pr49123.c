/* PR lto/49123 */

extern void abort (void);
static struct S { int f : 1; } s;
static int v = -1;

int
main ()
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
   exit(0);
#else
  s.f = v < 0;
  if ((unsigned int) s.f != -1U)
    abort ();
  return 0;
#endif
}
