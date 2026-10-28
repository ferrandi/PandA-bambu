#define N 4

#include "template_inline_pragma.hpp"

void top_fun(int in[N], int out[N])
{
   for(int i = 0; i < N; ++i)
   {
      out[i] = adjust<int>::add(add_offset(in[i], 1), i);
   }
}
