/* PR rtl-optimization/31691 */
/* Origin: Chi-Hua Chen <stephaniechc-gccbug@yahoo.com> */

extern void abort (void);

static int get_kind(int) __attribute__ ((noinline));

static int get_kind(int v)
{
  volatile int k = v;
  return k;
}

static int some_call(void) __attribute__ ((noinline));

static int some_call(void)
{
  return 0;
}

static void example (int arg)
{
  int tmp, kind = get_kind (arg);

  if (kind == 9 || kind == 10 || kind == 5)
    {
      if (some_call() == 0)
        {
          if (kind == 9 || kind == 10)
            tmp = arg;
          else
            abort();
        }
    }
} 

int main(void)
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
#else
  example(10);
#endif
  return 0;
}
