int pipelined_function(int a, int b, int c)
{

   #pragma HLS_pipeline
   
   return (a - b) * c;
   
}

int outer_loop(int a, int b, int c, int d, int e)
{

   int x = 0;

   if(a < -10)
      a = 2;

   while(a < 8)
   {
      //...
      if(x%2 == 0)
         x += pipelined_function(x, b, c);
      else
         x += pipelined_function(b, x, c);
      a++;
   }

   return x;
}
