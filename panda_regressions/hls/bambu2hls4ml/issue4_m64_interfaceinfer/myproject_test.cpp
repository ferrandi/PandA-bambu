#include <cstdint>
#include <iostream>

#include "firmware/myproject_float.h"

#ifdef __BAMBU__
#include <mdpi/mdpi_user.h>
#endif

namespace
{
   struct test_vector
   {
      std::uint16_t inputs[N_IN];
      std::uint64_t expected_outputs[N_OUT];
   };

   constexpr test_vector test_vectors[] = {
       {{0, 0}, {112785, 104159}},
       {{1024, 2048}, {141477, 152905}},
       {{64512, 512}, {109614, 96352}},
       {{1536, 65024}, {119974, 119753}},
   };
} // namespace

int main()
{
   bool failed = false;
   for(unsigned int test_index = 0; test_index < sizeof(test_vectors) / sizeof(test_vectors[0]); ++test_index)
   {
      hls::stream<in_container_t> input_stream("input_stream");
      hls::stream<out_container_t> output_stream("output_stream");
      for(unsigned int input_index = 0; input_index < N_IN; ++input_index)
      {
         input_stream.write(test_vectors[test_index].inputs[input_index]);
      }

#ifdef __BAMBU__
      m_param_alloc(0, sizeof(input_stream));
      m_param_alloc(1, sizeof(output_stream));
#endif
      myproject_float(input_stream, output_stream);

      for(unsigned int output_index = 0; output_index < N_OUT; ++output_index)
      {
         const auto actual = output_stream.read().to_uint64();
         const auto expected = test_vectors[test_index].expected_outputs[output_index];
         if(actual != expected)
         {
            std::cerr << "Test " << test_index << ", output " << output_index << ": expected " << expected << ", got "
                      << actual << std::endl;
            failed = true;
         }
      }
   }
   return failed ? 1 : 0;
}
