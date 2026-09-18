#define NI 16
#define ARR_PART_FACTOR 2

void top_fun(int in[NI][NI], int out[NI][NI])
{
#pragma HLS array_partition variable = in cyclic factor = ARR_PART_FACTOR dim = 2
#pragma HLS array_partition variable = out cyclic factor = ARR_PART_FACTOR dim = 2

   int loc[NI];
#pragma HLS array_partition variable = loc cyclic factor = ARR_PART_FACTOR dim = 1

   for(int i = 0; i < NI; i++)
   {
      loc[i] = i + 1;
   }

   for(int i = 0; i < NI; i++)
   {
      for(int j = 0; j < NI; j++)
      {
         out[i][j] = in[i][j] * loc[j];
      }
   }
}
