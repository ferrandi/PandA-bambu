#include "module_lib.h"
void module2(uint32_t input1, module2_output_t *outputs)
{
  outputs->output1 = input1 * input1;
  outputs->output2 = input1 | (((uint64_t)input1)<<32);
  outputs->output3 = (uint16_t)input1;
}
