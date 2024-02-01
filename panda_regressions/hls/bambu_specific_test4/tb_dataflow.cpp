#include "dataflow.hpp"

int main()
{
   ac_channel<int> in = {1, 2};
   ac_channel<int> out;

   const auto in_size = in.size();
   for(auto i = 0; i < in_size; ++i)
   {
      dataflow_top(in, out);
   }

   int v;
   while(out.nb_read(v))
   {
      std::cout << "res: " << v << "\n";
   }
   return 0;
}
