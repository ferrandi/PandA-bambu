#define NI 66

void top_fun(int in[NI][NI], int out[NI][NI]);

int glob[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
              23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
              45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66};

void create_gold(int in[NI][NI], int gold[NI][NI])
{
   int loc[NI];

   for(int i = 0; i < NI; i++)
   {
      loc[i] = i + 1;
   }

   for(int i = 0; i < NI; i++)
   {
      for(int j = 0; j < NI; j++)
      {
         gold[i][j] = in[i][j] * loc[j] + glob[j];
      }
   }
}

int test(int out[NI][NI], int gold[NI][NI])
{
   for(int i = 0; i < NI; i++)
   {
      for(int j = 0; j < NI; j++)
      {
         if(out[i][j] != gold[i][j])
         {
            return 1;
         }
      }
   }
   return 0;
}

int main()
{
   int in[NI][NI];
   int out[NI][NI] = {{0}};
   int gold[NI][NI] = {{0}};

   for(int i = 0; i < NI; i++)
   {
      for(int j = 0; j < NI; j++)
      {
         in[i][j] = i * j;
      }
   }

   create_gold(in, gold);
   top_fun(in, out);
   return test(out, gold);
}
