#include "aggregate.h"
#include "stddef.h"

void
_quicksort (void *const pbase, size_t total_elems, size_t size,
	    int (*cmp)(const void *, const void *));
int less (const void * a, const void * b);

void test(float * const pbase, size_t total_elems)
{
  _quicksort(pbase, (sizeof(float) * total_elems) / sizeof(struct aggregate), sizeof(struct aggregate), less);
}
