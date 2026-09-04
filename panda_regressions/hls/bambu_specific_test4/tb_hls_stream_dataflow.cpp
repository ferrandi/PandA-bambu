#include "ap_int.h"
#include "hls_stream.h"

#include <iostream>

extern void hls_stream_dataflow_top(hls::stream<ap_uint<16>>& a, hls::stream<ap_uint<16>>& d);

int main()
{
   hls::stream<ap_uint<16>> in = {1, 2, 3, 4, 5, 6, 7, 8};
   hls::stream<ap_uint<16>> out;

   const auto in_size = in.size();
   hls_stream_dataflow_top(in, out);

   ap_uint<16> v;
   for(unsigned int i = 0; i < in_size; ++i)
   {
      out.read(v);
      std::cout << "res: " << v << "\n";
   }
   return 0;
}
