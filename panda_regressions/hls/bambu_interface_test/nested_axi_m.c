#define N 20
#define NUM_TYPE unsigned char

NUM_TYPE __attribute__((noinline)) my_func(NUM_TYPE a)
{
   return a + 1;
}

void __attribute__((noinline)) kernel(NUM_TYPE arg0[N], NUM_TYPE arg1[N])
{
   unsigned i = 0;
   for(; i < N; ++i)
   {
      arg1[i] = my_func(arg0[i]);
   }
}

void __attribute__((noinline)) kernel_0(NUM_TYPE arg1[N], NUM_TYPE arg2[N], NUM_TYPE arg3[N])
{
   unsigned i = 0;
   for(; i < N; ++i)
   {
      arg1[i] = arg2[i] + my_func(arg3[i]);
   }
}

#pragma HLS interface port = arg0 mode = m_axi offset = direct
#pragma HLS interface port = arg1 mode = m_axi offset = direct
void main_module(NUM_TYPE* arg0, NUM_TYPE* arg1)
{
   kernel(arg0, arg1);
   kernel_0(arg0, arg1, arg1);
}