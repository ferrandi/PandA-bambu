#include <iostream>

#define __BAMBU_SIM__

#include "ac_channel.h"
#include "ap_int.h"

extern void sum3numbers(ac_channel<ap_uint<64>>& a, ac_channel<ap_uint<64>>& b, ac_channel<ap_uint<64>>& c,
                        ac_channel<ap_uint<64>>& d);

#define N 8

#ifdef EXPORT_TB
#include "bambu_tb_exporter.hpp"
#endif

int main()
{
   ac_channel<ap_uint<64>> a = {0, 1, 2, 3, 4, 5, 6, 7};
   ac_channel<ap_uint<64>> b = {0, 1, 2, 3, 4, 5, 6, 7};
   ac_channel<ap_uint<64>> c = {0, 1, 2, 3, 4, 5, 6, 7};
   ac_channel<ap_uint<64>> d = {0, 0, 0, 0, 0, 0, 0, 0};

   short gold[N] = {0, 3, 6, 9, 12, 15, 18, 21};

#ifdef EXPORT_TB
   bambu::testbench_exporter btb("test.sum3numbers.xml");
   btb.export_init();
   btb.export_channel(a, "a");
   btb.export_channel(b, "b");
   btb.export_channel(c, "c");
   btb.export_channel(d, "d");
#endif

   sum3numbers(a, b, c, d);

   for(unsigned i = 0; i < N; ++i)
   {
      if(d[i] != gold[i])
      {
         std::cout << "Test not good" << std::endl;
         return -1;
      }
   }

   std::cout << "Test passed!" << std::endl;

   return 0;
}