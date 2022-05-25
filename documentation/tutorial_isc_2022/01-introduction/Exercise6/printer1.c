#include "module_lib.h"
#include <stdio.h>
void printer1(uint64_t value1, uint64_t value2, uint16_t value3, uint32_t value4)
{
  printf("printer1: %llx %llx %x %x\n", value1, value2, value3, value4);
}
