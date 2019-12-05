int function(int a, int b, int c, int d)
{
   if(a < 5)
      return a * b + c * d;
   else if(a > 10)
      return a + b * c + d;
   else
      return a + b + c * d;
}

int function_call(int a, int b, int c, int d, int e)
{
   int aa = function(a, b, c, d);
   int ab = function(b, c, d, e);
   int ac = function(c, d, e, a);
   int ad = function(d, e, a, b);
   
   return function(aa, ab, ac, ad) * e;
}
