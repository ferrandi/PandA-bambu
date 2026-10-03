#define NI 8

void top_fun(int in[NI], int out[NI])
{
#pragma HLS array_partition variable = in cyclic factor = 2 dim = 1
#pragma HLS array_partition variable = out cyclic factor = 2 dim = 1

   for(int i = 0; i < NI; i++)
   {
      out[i] = in[i] * 10 + i + (i + 1) * (i + 1);
   }
}
