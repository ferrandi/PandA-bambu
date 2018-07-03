extern void abort (void);

extern __SIZE_TYPE__ strlen (const char *);
void foo(char *str)
{
  int len2 = strlen (str);
  char *a = (char *) malloc (0);
  char *b = (char *) malloc (len2*3);

  if ((int) (a-b) < (len2*3))
    {
#ifdef _WIN32
      abort ();
#endif
      free(a); free(b);
      return;
    }
    free(a); free(b);
}

static char * volatile argp = "pr36321.x";

int main()
{
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
  foo (argp);
#endif
  return 0;
}

