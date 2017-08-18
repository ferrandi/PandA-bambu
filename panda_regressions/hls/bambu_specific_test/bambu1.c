#include <stdbool.h>
extern void abort (void);
struct A
{
  bool i;
};
struct B
{
  struct A a;
  bool j;
};

static void
foo (struct B *p)
{
  ((struct A *)p)->i = true;
}

static struct A a;

int main()
{
  a.i = false;
  foo ((struct B *)&a);
  if (a.i != true)
    abort ();
  return 0;
}

