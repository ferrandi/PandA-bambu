extern void empty();

int irr4_test(int a, int b, int c)
{
  unsigned int index=0;
  if(a>b)
    goto A;
  else
    goto B;

  A:
    ++index;
    empty();
    goto C;
  B:
    a += 2;
    empty();
    goto D;
  C:
    empty();
    goto D;
  D:
    empty();
    goto E;
  E:
    empty();
    if(index < a)
      goto F;
    else
      goto C;
  F:
    return index;
}

