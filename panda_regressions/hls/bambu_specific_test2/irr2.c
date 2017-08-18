int irr2_test(int a, int b, int c)
{
  unsigned int index=0;
  if(a>b)
    goto A;
  else
    goto B;

  A:
    ++index;
    goto C;
  B:
    a += 2;
    goto C;
  C:
  if(index < a)
    goto A;
  else if(a < b)
    goto B;
  return index + c;
}
