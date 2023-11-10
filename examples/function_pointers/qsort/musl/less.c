int less (const void * a, const void * b)
{
  int * aPtr = (int *)a;
  int * bPtr = (int *)b;
  if (*aPtr == *bPtr) return 0;
  return *aPtr < *bPtr ? 1 : -1;
}

