int __attribute__((noinline)) called_sumprod(int a, int b, int c, int d)
{
#pragma HLS pipeline II = 1 style = frp

   return a + b + c * d;
}

void __attribute__((noinline)) called_store(int a, int b, int c, int* out)
{
#pragma HLS pipeline II = 1 style = frp

   *out = a + b + c;
}

int main_caller(int a, int b, int c, int d, int e)
{
   int ab = 0;
   int bc = 0;
   int ac = 0;
   int ad = 0;
#pragma HLS pipeline II = 5
   if(a > b) ab = called_sumprod(a, b, c, d);
   if(b > c) bc = called_sumprod(b, c, d, e);
   if(a > c) ac = called_sumprod(c, d, e, a);
   if(a > d) ad = called_sumprod(d, e, a, b);

   return called_sumprod(ab, bc, ac, ad);
}

int main_load_store(int a, int b, int c, int d, int e)
{
   static int data[4];
//#pragma HLS array_partition variable = data complete
   int acc = e;
   int idx0 = a & 1;
   int idx1 = 2 + (b & 1);

   data[0] = a;
   data[1] = b;
   data[2] = c;
   data[3] = d;

#pragma HLS pipeline II = 7
   if(a > b)
   {
      acc += data[idx0];
   }
   if(b > c)
   {
      data[idx0] = acc + data[idx1];
   }
   if(a > c)
   {
      acc += data[1 + (d & 1)];
   }
   if(a > d)
   {
      data[3 - (c & 1)] = acc + data[idx0];
   }

   return acc + data[0] + data[1] + data[2] + data[3];
}

int main_void_caller(int a, int b, int c, int d, int e)
{
   static int ab;
   static int bc;
   static int ac;
   static int ad;

   ab = 0;
   bc = 0;
   ac = 0;
   ad = 0;

#pragma HLS pipeline II = 5
   if(a > b)
   {
      called_store(a, b, c, &ab);
   }
   if(b > c)
   {
      called_store(b, c, d, &bc);
   }
   if(a > c)
   {
      called_store(c, d, e, &ac);
   }
   if(a > d)
   {
      called_store(d, e, a, &ad);
   }

   return called_sumprod(ab, bc, ac, ad);
}
