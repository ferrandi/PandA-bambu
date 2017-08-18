#include "aggregate.h"


int less (void * a, void * b, void * notUsed)
{
  struct aggregate * aPtr = (struct aggregate *)a;
  struct aggregate * bPtr = (struct aggregate *)b;

  float aSum = aPtr->a0 +
               aPtr->a1 +
               aPtr->a2 +
               aPtr->a3 +
               aPtr->a4 +
               aPtr->a5 +
               aPtr->a6 +
               aPtr->a7;
  float bSum = bPtr->a0 +
               bPtr->a1 +
               bPtr->a2 +
               bPtr->a3 +
               bPtr->a4 +
               bPtr->a5 +
               bPtr->a6 +
               bPtr->a7;
  int equal = (bSum - aSum) == 0;
  if (equal) return 0;

  int lt = (aSum - bSum) < 0;
  return lt ? -1 : 1;
}

