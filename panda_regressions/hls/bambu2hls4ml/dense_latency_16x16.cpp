#include "ac_fixed.h"

typedef ac_fixed<16, 6> data_t;
typedef ac_fixed<37, 17> accum_t;

void dense_latency_16x16(data_t input[16], data_t output[16])
{
   static const data_t weights[16 * 16] = {};
   static const accum_t biases[16] = {};
   accum_t mult[16 * 16];
   accum_t acc[16];

#pragma HLS ARRAY_PARTITION variable=mult complete
#pragma HLS ARRAY_PARTITION variable=acc complete

   for(unsigned i = 0; i < 16; ++i)
   {
      for(unsigned j = 0; j < 16; ++j)
      {
         mult[i * 16 + j] = input[i] * weights[i * 16 + j];
      }
   }

   for(unsigned j = 0; j < 16; ++j)
   {
      acc[j] = biases[j];
      for(unsigned i = 0; i < 16; ++i)
      {
         acc[j] += mult[i * 16 + j];
      }
      output[j] = acc[j];
   }
}
