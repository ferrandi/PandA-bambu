extern volatile int ReadyFlag_NotProperlyInitialized;

volatile int ReadyFlag_NotProperlyInitialized=1;

int main(void)
{
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
#else
  if (ReadyFlag_NotProperlyInitialized != 1)
    __builtin_abort ();
#endif
  return 0;
}
