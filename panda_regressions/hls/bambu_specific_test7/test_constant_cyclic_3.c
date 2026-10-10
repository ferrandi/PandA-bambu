#define NI 66
#define UNROLL_FACTOR 3

int glob[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
              23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
              45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};

void top_fun(int in[NI][NI], int out[NI][NI])
{
#pragma HLS array_partition variable = glob cyclic factor = UNROLL_FACTOR
#pragma HLS array_partition variable = in cyclic factor = UNROLL_FACTOR dim = 2
#pragma HLS array_partition variable = out cyclic factor = UNROLL_FACTOR dim = 2

   int loc[NI];
#pragma HLS array_partition variable = loc cyclic factor = UNROLL_FACTOR

#pragma clang loop unroll_count(UNROLL_FACTOR)
   for(int i = 0; i < NI; i++)
   {
      loc[i] = i + 1;
   }

   for(int i = 0; i < NI; i++)
   {
#pragma clang loop unroll_count(UNROLL_FACTOR)
      for(int j = 0; j < NI; j++)
      {
         out[i][j] = in[i][j] * loc[j] + glob[j];
      }
   }
}
