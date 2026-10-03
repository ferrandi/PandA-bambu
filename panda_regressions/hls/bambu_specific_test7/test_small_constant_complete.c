#define NI 8

int glob[] = {1, 2, 3, 4, 5, 6, 7, 8};

void top_fun(int in[NI], int out[NI])
{
#pragma HLS array_partition variable = glob complete
#pragma HLS array_partition variable = in complete
#pragma HLS array_partition variable = out complete

   int loc[NI];
#pragma HLS array_partition variable = loc complete dim = 1

#pragma clang loop unroll(full)
   for(int i = 0; i < NI; i++)
   {
      loc[i] = i + 1;
   }

#pragma clang loop unroll(full)
   for(int j = 0; j < NI; j++)
   {
      out[j] = in[j] * 10 + j + loc[j] * glob[j];
   }
}
