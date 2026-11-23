#include <hls_stream.h>

extern void dataflow_const_weights_widths(hls::stream<int>& input, hls::stream<int>& output);

int main()
{
   // Reference computed on the host: same layers, applied to the same input.
   static const short w1[4 * 7] = {1, -2, 3,  -4, 5,  -6, 7,  -1, 2, -3, 4,  -5, 6,  -7,
                                   2, 1,  -1, 3,  -2, 4,  -3, 1,  1, 2,  -2, 3,  -3, 4};
   static const short b1[7] = {1, -1, 2, -2, 3, -3, 4};
   static const short w2[7 * 2] = {3, -1, 2, 4, -2, 1, 5, -3, 1, 2, -4, 3, 2, -1};
   static const short b2[2] = {-5, 7};
   const int x[4] = {3, -1, 4, 2};
   int h[7], y[2];
   for(int j = 0; j < 7; ++j)
   {
      h[j] = b1[j];
      for(int i = 0; i < 4; ++i)
      {
         h[j] += x[i] * w1[i * 7 + j];
      }
   }
   for(int j = 0; j < 2; ++j)
   {
      y[j] = b2[j];
      for(int i = 0; i < 7; ++i)
      {
         y[j] += h[i] * w2[i * 2 + j];
      }
   }

   hls::stream<int> input;
   hls::stream<int> output;
   for(int i = 0; i < 4; ++i)
   {
      input.write(x[i]);
   }
   dataflow_const_weights_widths(input, output);
   int errors = 0;
   for(int j = 0; j < 2; ++j)
   {
      errors += output.read() != y[j];
   }
   return errors;
}
