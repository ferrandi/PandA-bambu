__attribute__((noinline))
void atomicIncrement(unsigned * var, unsigned value)
{
  *var += value;
}


