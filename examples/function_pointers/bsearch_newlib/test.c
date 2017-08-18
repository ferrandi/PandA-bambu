typedef unsigned int size_t;

#include "aggregate.h"

int less (const void * a, const void * b);

void *bsearch(const void *key, const void *base, size_t /* nmemb */ high,
              size_t size, int (*compar)(const void *, const void *));

int test(float * key, float *base, size_t nmemb)
{
  void * res = bsearch(key, base, (sizeof(float) * nmemb) / sizeof(struct aggregate), sizeof(struct aggregate), less);
  return res ? (res - (void *)base)/sizeof(struct aggregate) : -1;
}
