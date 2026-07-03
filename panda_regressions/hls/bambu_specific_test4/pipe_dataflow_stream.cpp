#include <cstdio>
#include <hls_stream.h>

#define BATCH_ITEM_NUMBER 16
#define TOTAL_BATCHES 8

extern "C" void __attribute__((noinline))
load(hls::stream<unsigned>& dma_in, hls::stream<unsigned char>& ctrl_out, unsigned int array_out[BATCH_ITEM_NUMBER * 2])
{
   static unsigned char out_buffer_index = 0;
   for(int b = 0; b < TOTAL_BATCHES; ++b)
   {
      for(int i = 0; i < BATCH_ITEM_NUMBER; ++i)
      {
         array_out[BATCH_ITEM_NUMBER * out_buffer_index + i] = dma_in.read();
      }
      ctrl_out.write(out_buffer_index);
      out_buffer_index = (out_buffer_index + 1) % 2;
   }
}

extern "C" void __attribute__((noinline))
compute(hls::stream<unsigned char>& ctrl_in, hls::stream<unsigned char>& ctrl_out,
        unsigned int array_in[BATCH_ITEM_NUMBER * 2], unsigned int array_out[BATCH_ITEM_NUMBER * 2])
{
   static unsigned char out_buffer_index = 0;
   for(int b = 0; b < TOTAL_BATCHES; ++b)
   {
      unsigned char in_buffer_index = ctrl_in.peek();
      for(int i = 0; i < BATCH_ITEM_NUMBER; ++i)
      {
         array_out[BATCH_ITEM_NUMBER * out_buffer_index + i] =
             array_in[BATCH_ITEM_NUMBER * in_buffer_index + i] * array_in[BATCH_ITEM_NUMBER * in_buffer_index + i];
      }
      ctrl_in.read();
      ctrl_out.write(out_buffer_index);
      out_buffer_index = (out_buffer_index + 1) % 2;
   }
}

extern "C" void __attribute__((noinline))
store(hls::stream<unsigned char>& ctrl_in, unsigned int array_in[BATCH_ITEM_NUMBER * 2], hls::stream<unsigned>& dma_out)
{
   for(int b = 0; b < TOTAL_BATCHES; ++b)
   {
      unsigned char in_buffer_index = ctrl_in.peek();
      for(int i = 0; i < BATCH_ITEM_NUMBER; ++i)
      {
         dma_out.write(array_in[BATCH_ITEM_NUMBER * in_buffer_index + i]);
      }
      ctrl_in.read();
   }
}

#pragma HLS interface port = dma_in mode = axis
#pragma HLS interface port = dma_out mode = axis
extern "C" void top(hls::stream<unsigned>& dma_in, hls::stream<unsigned>& dma_out)
{
#pragma HLS DATAFLOW

   hls::stream<unsigned char> ctrl1;
   hls::stream<unsigned char> ctrl2;
   unsigned int buffer1[BATCH_ITEM_NUMBER * 2];
   unsigned int buffer2[BATCH_ITEM_NUMBER * 2];
   load(dma_in, ctrl1, buffer1);
   compute(ctrl1, ctrl2, buffer1, buffer2);
   store(ctrl2, buffer2, dma_out);
}
