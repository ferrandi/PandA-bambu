extern void irr3_empty();

int irr3_test(int a, int b, int c)
{
  unsigned int index=0;
  if(a>b)
    goto A;
  else
    goto B;

  A:
    ++index;
    irr3_empty();
    goto C;
  B:
    a += 2;
    irr3_empty();
    goto C;
  C:
  if(index < a)
    goto A;
  else if(a < b)
    goto B;
  if(c>b)
  {
    irr3_empty();
    return index + c;
  }
  else
  {
    irr3_empty();
    return index - c;
  }
}
