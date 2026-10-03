#define NI 40

int glob[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
              13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
              25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
              37, 38, 39, 40};

void top_fun(int ni, int in[NI][NI], int out[NI][NI])
{
#pragma HLS array_partition variable = glob cyclic factor = UNROLL_FACTOR dim = 1
#pragma HLS array_partition variable = in cyclic factor = UNROLL_FACTOR dim = 2
#pragma HLS array_partition variable = out cyclic factor = UNROLL_FACTOR dim = 2

   int loc[NI];
#pragma HLS array_partition variable = loc cyclic factor = UNROLL_FACTOR dim = 1

#pragma clang loop unroll_count(UNROLL_FACTOR)
   for(int i = 0; i < ni; i++)
   {
      loc[i] = i + 1;
   }

   for(int i = 0; i < ni; i++)
   {
#pragma clang loop unroll_count(UNROLL_FACTOR)
      for(int j = 0; j < ni; j++)
      {
         out[i][j] = in[i][j] * loc[j] + glob[j];
      }
   }
}
