int __attribute__((noinline)) pipelined_function(int a, int b, int c)
{
#pragma HLS pipeline

   return (a - b) * c;
}

int outer_loop(int a, int b, int c)
{
   int x = b + c;

   if(a < -10)
      a = 2;

   while(a < 8)
   {
      if(x % 2 == 0)
         x += pipelined_function(x, b, c);
      else
         x += pipelined_function(b, x, c);
      a++;
   }

   return x;
}
