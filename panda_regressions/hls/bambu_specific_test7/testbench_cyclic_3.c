#define NI 36

void top_fun(int ni, int in[NI][NI], int out[NI][NI]);

int glob[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
              19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};

void create_gold(int ni,int in[NI][NI], int gold[NI][NI])
{
   int loc[NI];

   for(int i = 0; i < ni; i++)
   {
      loc[i] = i + 1;
   }

   for(int i = 0; i < ni; i++)
   {
      for(int j = 0; j < ni; j++)
      {
         gold[i][j] = in[i][j] * loc[j] + glob[j];
      }
   }
}

int test(int ni, int out[NI][NI], int gold[NI][NI])
{
   for(int i = 0; i < ni; i++)
   {
      for(int j = 0; j < ni; j++)
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
   int gold[NI][NI];

   for(int i = 0; i < NI; i++)
   {
      for(int j = 0; j < NI; j++)
      {
         in[i][j] = i * j;
      }
   }

   create_gold(NI, in, gold);
   top_fun(NI, in, out);
   return test(NI, out, gold);
}