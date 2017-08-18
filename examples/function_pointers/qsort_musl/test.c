typedef unsigned int size_t;

//#include "qsort.c"
void qsort(void *base, size_t nel, size_t width, int (*cmp)(const void *, const void *));

int less (const void * a, const void * b);

void test(int * const pbase, size_t total_elems)
{
  qsort(pbase, total_elems, sizeof(int), less);
}
