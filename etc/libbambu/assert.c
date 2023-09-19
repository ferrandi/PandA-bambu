/**
 *  __assert and __assert_fail primitives adapted to the PandA infrastructure by Fabrizio Ferrandi from Politecnico di
 * Milano. June, 4 2015.
 *
 */
extern void abort(void) __attribute__((noreturn));
extern int printf(const char *format, ...);
void __assert(const char* __assertion, const char* __file, int __line)
{
   printf("%s: %d: Assertion `%s' failed.\n", __file, __line, __assertion);
   abort();
}

void __assert_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function)
{
   printf("%s: %d: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
   abort();
}
