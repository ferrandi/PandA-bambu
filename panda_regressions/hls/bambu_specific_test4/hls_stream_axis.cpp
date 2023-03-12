#include <ap_int.h>
#include <hls_stream.h>

#pragma HLS_interface a axis
#pragma HLS_interface b axis
#pragma HLS_interface c axis
#pragma HLS_interface d axis
void sum3numbers(hls::stream<ap_uint<16>>& a, hls::stream<ap_uint<16>>& b, hls::stream<ap_uint<16>>& c,
                 hls::stream<ap_uint<16>>& d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
      d.write(a.read() + b.read() + c.read());
}
