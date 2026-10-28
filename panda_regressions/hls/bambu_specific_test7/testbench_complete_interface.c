#define N 4

void top_fun(int in[N], int out[N]);

int main()
{
   int in[N];
   int out[N] = {0, 0, 0, 0};

   for(int i = 0; i < N; ++i)
   {
      in[i] = i * 3;
   }

   top_fun(in, out);

   for(int i = 0; i < N; ++i)
   {
      if(out[i] != in[i] + i + 1)
      {
         return 1;
      }
   }
   return 0;
}
