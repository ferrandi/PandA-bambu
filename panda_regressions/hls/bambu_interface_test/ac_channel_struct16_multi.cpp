#include <ac_channel.h>
#include <cstdio>

// Struct with 16 elements of unsigned long long (64-bit)
struct Data16
{
   unsigned long long val[16];
};

// Kernel: reads a struct via ac_channel, sums in pairs (first reduction step),
// stores results back into the same array, and returns via ac_channel.
extern "C" void __attribute__((noinline)) reduce16(Data16* inData1, ac_channel<Data16>& out, int n)
{
   for(int i = 0; i < n; ++i)
   {
      Data16 inData = *(inData1 + i);
      Data16 outData;
      // Pairwise reduction: sum elements [i] + [i+1], store at [i] for i = 0, 2, ..., 14
      for(int i = 0; i < 15; i++)
      {
         unsigned long long a0 = inData.val[i];
         unsigned long long a1 = inData.val[i + 1];
         outData.val[i] = i % 2 == 0 ? a0 + a1 : 0;
      }
      outData.val[15] = 0;

      out.write(outData);
   }
}

extern "C" void __attribute__((noinline)) reduce16_1(Data16* inData1, ac_channel<Data16>& out, int n)
{
   for(int i = 0; i < n; ++i)
   {
      Data16 inData = *(inData1 + i);
      Data16 outData;

      // Pairwise reduction: sum elements [i] + [i+1], store at [i] for i = 0, 2, ..., 14
      for(int i = 0; i < 15; i++)
      {
         unsigned long long a0 = inData.val[i];
         unsigned long long a1 = inData.val[i + 1];
         outData.val[i] = i % 2 == 0 ? a0 + a1 : 0;
      }
      outData.val[15] = 0;

      out.write(outData);
   }
}

#pragma HLS interface port=out1 mode=axis
#pragma HLS interface port=out2 mode=axis
#pragma HLS interface port = inData1 mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = inData2 mode = m_axi offset = direct bundle = gmem1
extern "C" void top(Data16* inData1, Data16* inData2, ac_channel<Data16>& out1, ac_channel<Data16>& out2, int n)
{
#pragma HLS DATAFLOW
   reduce16(inData1, out1, n);
   reduce16_1(inData2, out2, n);
}
