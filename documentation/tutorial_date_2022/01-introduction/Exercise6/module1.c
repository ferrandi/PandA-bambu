#include "module_lib.h"
void module1(uint32_t input1, uint16_t input2, module1_output_t *outputs)
{
  outputs->output1 = input1 * input2;
  outputs->output2 = input1 + input2;
  outputs->output3 = (~input2) + 1;
  outputs->output4 = input2 | (((uint32_t)input2)<<16);
}
