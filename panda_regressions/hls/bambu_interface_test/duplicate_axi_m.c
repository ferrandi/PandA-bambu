#define N 20
#define NUM_TYPE unsigned char

NUM_TYPE __attribute__((noinline)) my_func(NUM_TYPE a)
{
   return a + 1;
}

void __attribute__((noinline)) kernel_0(NUM_TYPE arg1[N], NUM_TYPE arg2[N], NUM_TYPE arg3[N])
{
   unsigned i = 0;
   for(; i < N; ++i)
   {
      arg1[i] = arg2[i] + my_func(arg3[i]);
   }
}

#pragma HLS_interface arg0 m_axi direct
#pragma HLS_interface arg1 m_axi direct
void main_module(NUM_TYPE* arg0, NUM_TYPE* arg1)
{
   kernel_0(arg0, arg1, arg1);
}