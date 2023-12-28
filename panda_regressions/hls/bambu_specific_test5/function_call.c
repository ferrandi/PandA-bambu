int __attribute__((noinline)) called_sumprod(int a, int b, int c, int d)
{

   #pragma HLS pipeline
   
   return a + b + c * d;
}

int main_caller(int a, int b, int c, int d, int e)
{

   #pragma HLS pipeline
   
   int aa = called_sumprod(a, b, c, d);
   int ab = called_sumprod(b, c, d, e);
   int ac = called_sumprod(c, d, e, a);
   int ad = called_sumprod(d, e, a, b);
   
   return called_sumprod(aa, ab, ac, ad);
}
