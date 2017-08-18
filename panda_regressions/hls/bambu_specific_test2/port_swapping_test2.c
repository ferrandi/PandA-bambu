int sum(int a, int b, int c) __attribute__((noinline));
int sum(int a, int b, int c)
{
   return a + b + c;
}

int function(int a, int b, int c, int d, int e, int f)
{
   int t0, t1, t2, p1, p2, p3;
   t0 = a-b;
   t1 = c-d;
   t2 = e-f;
   p1 = t0*t1;
   p2 = t1*t2;
   p3 = t2*t0;
   return sum(p1,p2,p3);
}
