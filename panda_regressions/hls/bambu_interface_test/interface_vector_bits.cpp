// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Exercise bit-preserving integer/vector conversions in inferred interfaces.
#include <cstdlib>
#include <cstring>

typedef unsigned int vec_u32 __attribute__((vector_size(8)));
typedef float vec_f32 __attribute__((vector_size(8)));
typedef unsigned short vec_u16 __attribute__((vector_size(4), may_alias));

#ifdef FLOAT_VECTOR
typedef vec_f32 stream_value;
#else
typedef vec_u32 stream_value;
#endif

#ifdef CHANNEL
#include "ac_channel.h"
#ifdef AGGREGATE
struct payload
{
   vec_u32 bits;
   vec_f32 values;
};
#else
typedef stream_value payload;
#endif
#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
extern "C" void __attribute__((noinline)) vector_bits(ac_channel<payload>& input, ac_channel<payload>& output)
{
   for(unsigned i = 0; i < 4; ++i)
   {
#ifdef AGGREGATE
      const payload value = input.read();
      payload result;
      result.bits = value.bits ^ vec_u32 { 0x80000000U, 0x12345678U };
      result.values = value.values;
      output.write(result);
#else
      output.write(input.read());
#endif
   }
}
#else
#ifdef MIXED_WIDTH
#pragma HLS interface port = input mode = bus
#pragma HLS interface port = output mode = bus
#else
#pragma HLS interface port = input mode = fifo depth = 4
#pragma HLS interface port = output mode = fifo depth = 4
#endif
extern "C" void __attribute__((noinline)) vector_bits(const stream_value* input, stream_value* output)
{
#ifdef MIXED_WIDTH
   const auto narrow_input = reinterpret_cast<const vec_u16*>(input);
   const auto narrow_output = reinterpret_cast<vec_u16*>(output);
   for(unsigned i = 0; i < 6; ++i)
   {
      narrow_output[i] = narrow_input[i];
   }
   // Access both widths through the same port, whose integer carrier is 64 bits.
   output[3] = input[3];
#else
   for(unsigned i = 0; i < 4; ++i)
   {
      output[i] = input[i];
   }
#endif
}
#endif

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   const vec_u32 patterns[4] = {
       {0x80000001U, 0xffffffffU}, {0x01234567U, 0x89abcdefU}, {0U, 0x80000000U}, {0x7fc12345U, 0xdeadbeefU}};
#ifdef CHANNEL
   ac_channel<payload> input, output;
   for(unsigned i = 0; i < 4; ++i)
   {
#ifdef AGGREGATE
      payload value;
      value.bits = patterns[i];
      // Include signed zero and NaN payloads: numeric casts lose these bits.
      std::memcpy(&value.values, &patterns[3 - i], sizeof(vec_f32));
#else
      payload value;
      std::memcpy(&value, &patterns[i], sizeof(value));
#endif
      input.write(value);
   }
   vector_bits(input, output);
   for(unsigned i = 0; i < 4; ++i)
   {
      const payload value = output.read();
#ifdef AGGREGATE
      const vec_u32 expected = patterns[i] ^ vec_u32 { 0x80000000U, 0x12345678U };
      if(std::memcmp(&value.bits, &expected, sizeof(vec_u32)) ||
         std::memcmp(&value.values, &patterns[3 - i], sizeof(vec_f32)))
#else
      if(std::memcmp(&value, &patterns[i], sizeof(vec_u32)))
#endif
      {
         std::abort();
      }
   }
   if(input.available(1) || output.available(1))
   {
      std::abort();
   }
#else
   stream_value input[4], output[4] = {};
   std::memcpy(input, patterns, sizeof(input));
#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(patterns));
   m_param_alloc(1, sizeof(output));
#endif
   vector_bits(input, output);
   if(std::memcmp(patterns, output, sizeof(patterns)))
   {
      std::abort();
   }
#endif
   return 0;
}
