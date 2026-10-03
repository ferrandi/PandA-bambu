#include "ap_int.h"
#include "hls_stream.h"

static void st_passthrough(hls::stream<ap_uint<16>>& in, hls::stream<ap_uint<16>>& mid)
{
#pragma nounroll
   for(int i = 0; i < 8; ++i)
      mid.write(in.read());
}

static void st_add(hls::stream<ap_uint<16>>& mid, hls::stream<ap_uint<16>>& out)
{
#pragma nounroll
   for(int i = 0; i < 8; ++i)
      out.write(mid.read() + 43);
}

static void st_output(hls::stream<ap_uint<16>>& out, hls::stream<ap_uint<16>>& d)
{
#pragma nounroll
   for(int i = 0; i < 8; ++i)
      d.write(out.read());
}

#pragma HLS interface port = a mode = axis
#pragma HLS interface port = d mode = axis
void hls_stream_dataflow_top(hls::stream<ap_uint<16>>& a, hls::stream<ap_uint<16>>& d)
{
#pragma HLS dataflow
   hls::stream<ap_uint<16>> mid;
   hls::stream<ap_uint<16>> out;

   st_passthrough(a, mid);
   st_add(mid, out);
   st_output(out, d);
}
