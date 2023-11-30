typedef unsigned int size_t;

typedef int (*__compar_d_fn_t)(void *, void *, void *);

#include "aggregate.h"

//#include "qsort.c"
void
_quicksort (void *const pbase, size_t total_elems, size_t size,
	    int (*cmp)(const void *, const void *, void *), void *arg);
//#include "less.c"
int less (void * a, void * b, void * notUsed);

void test(float * const pbase, size_t total_elems)
{
  _quicksort(pbase, (sizeof(float) * total_elems) / sizeof(struct aggregate), sizeof(struct aggregate), less , (void *)0);
}
