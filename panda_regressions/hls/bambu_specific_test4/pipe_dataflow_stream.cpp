#include <hls_stream.h>
#include <cstdio>


extern "C" void __attribute__((noinline)) load(hls::stream<unsigned>& dma_in, hls::stream<unsigned char>& ctrl_out, unsigned int array_out[16*2])
{
   static unsigned char out_buffer_index = 0;
   for(int i = 0; i < 16; ++i)
   {
      array_out[16*out_buffer_index+i] = dma_in.read();
   }
   ctrl_out.write(out_buffer_index);
   out_buffer_index =  (out_buffer_index + 1)%2;
}

extern "C" void __attribute__((noinline)) compute(hls::stream<unsigned char>& ctrl_in, hls::stream<unsigned char>& ctrl_out, unsigned int array_in[16*2], unsigned int array_out[16*2])
{
   static unsigned char out_buffer_index = 0;
   unsigned char in_buffer_index = ctrl_in.read();
   for(int i = 0; i < 16; ++i)
   {
      array_out[16*out_buffer_index+i] = array_in[16*in_buffer_index+i] * array_in[16*in_buffer_index+i];
   }
   ctrl_out.write(out_buffer_index);
   out_buffer_index =  (out_buffer_index + 1)%2;
}


extern "C" void __attribute__((noinline)) store(hls::stream<unsigned char>& ctrl_in, unsigned int array_in[16*2], hls::stream<unsigned>& dma_out)
{
   unsigned char in_buffer_index = ctrl_in.read();
   for(int i = 0; i < 16; ++i)
   {
      dma_out.write(array_in[16*in_buffer_index+i]);
   }
}

#pragma HLS interface port = dma_in mode = axis
#pragma HLS interface port = dma_out mode = axis
extern "C" void top(hls::stream<unsigned>&dma_in, hls::stream<unsigned>&dma_out)
{
   #pragma HLS DATAFLOW

   hls::stream<unsigned char> ctrl1;
   hls::stream<unsigned char> ctrl2;
   unsigned int buffer1[16*2];
   unsigned int buffer2[16*2];
   load(dma_in, ctrl1, buffer1);
   compute(ctrl1, ctrl2, buffer1, buffer2);
   store(ctrl2,buffer2,dma_out);
}
