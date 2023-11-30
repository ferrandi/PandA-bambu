typedef unsigned int size_t;

#include "aggregate.h"

void qsort_r(void  *base,
           size_t nel,
           size_t width,
           int (*comp)(void *, void *, void*),
             void *arg);

int less (void * a, void * b, void * notUsed);

void test(float * const pbase, size_t total_elems)
{
  qsort_r(pbase, (sizeof(float) * total_elems) / sizeof(struct aggregate), sizeof(struct aggregate), 0, (void *)0);
}
