#include "vector_t.h"

#include <cstdio>
#include <cstdlib>
#include <iterator>

#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#pragma HLS interface port = d mode = bus
extern "C" void __attribute__((noinline)) sum3numbers(vector_t& a, vector_t& b, vector_t& c, vector_t& d)
{
   unsigned i;
   for(i = 0; i < std::distance(a.begin, a.end); ++i)
      *(d.begin + i) = *(a.begin + i) + *(b.begin + i) + *(c.begin + i);
   d.end = d.begin + i;
}

void gold_sum3numbers(vector_t& a, vector_t& b, vector_t& c, vector_t& d)
{
   unsigned i;
   for(i = 0; i < std::distance(a.begin, a.end); ++i)
      *(d.begin + i) = *(a.begin + i) + *(b.begin + i) + *(c.begin + i);
   d.end = d.begin + i;
}

int main()
{
   vector_t a = {{1, 2, 3, 4, 5, 6, 7, 8}};
   a.begin = a.values;
   a.end = &a.values[8];
   vector_t b = {{1, 2, 3, 4, 5, 6, 7, 8}};
   b.begin = b.values;
   b.end = &b.values[8];
   vector_t c = {{1, 2, 3, 4, 5, 6, 7, 8}};
   c.begin = c.values;
   c.end = &c.values[8];
   vector_t d, d_gold;
   d.begin = d.values;
   d.end = d.values;
   d_gold.begin = d.values;
   d_gold.end = d.values;

   sum3numbers(a, b, c, d);
   gold_sum3numbers(a, b, c, d_gold);

   uint8_t *it_gold, *it;
   for(it_gold = d_gold.begin, it = d.begin; it_gold != d_gold.end && it != d.end; ++it, ++it_gold)
      if(*it != *it_gold)
         abort();
   if(it != d.end || it_gold != d_gold.end)
      abort();
   printf("Checked %d values: OK!\n", std::distance(d_gold.begin, d_gold.end));

   return 0;
}
