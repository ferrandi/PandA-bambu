#include "dataflow2.hpp"

#include <cassert>

int main()
{
   ac_channel<int> in1 = {1, 2};
   ac_channel<int> in2 = {5, 7};
   ac_channel<int> out;

   assert(in1.size() == in2.size() && "Input channels must have same size.");
   const auto in_size = in1.size();
   for(auto i = 0; i < in_size; ++i)
   {
      dataflow_top(in1, in2, out);
   }

   int v;
   while(out.nb_read(v))
   {
      std::cout << "res: " << v << "\n";
   }
   return 0;
}
