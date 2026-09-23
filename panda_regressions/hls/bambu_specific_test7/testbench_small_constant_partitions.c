#define NI 8

int glob[] = {1, 2, 3, 4, 5, 6, 7, 8};
void top_fun(int in[NI], int out[NI]);

void create_gold(int in[NI], int gold[NI])
{
   int loc[NI];
   for(int i = 0; i < NI; i++)
   {
      loc[i] = i + 1;
   }

   for(int j = 0; j < NI; j++)
   {
      gold[j] = in[j] * 10 + j + loc[j] * glob[j];
   }
}

int test(int out[NI], int gold[NI])
{
   for(int j = 0; j < NI; j++)
   {
      if(out[j] != gold[j])
      {
         return 1;
      }
   }

   return 0;
}

int main()
{
   int in[NI];
   int out[NI] = {0};
   int gold[NI] = {0};

   for(int j = 0; j < NI; j++)
   {
      in[j] = 10 * j - 7;
   }

   create_gold(in, gold);
   top_fun(in, out);
   return test(out, gold);
}
