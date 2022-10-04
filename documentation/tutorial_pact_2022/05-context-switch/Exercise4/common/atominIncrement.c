__attribute__((noinline))
void atomicIncrement(unsigned * var, unsigned value)
{
#pragma omp atomic
  *var += value;
}


