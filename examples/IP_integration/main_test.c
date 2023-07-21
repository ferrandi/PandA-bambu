#include "module_lib.h"

int main()
{
  uint32_t param1=10;
  uint32_t param2=10<<16;
  my_ip(0, param1, param2);
  my_ip(1, param1, param2);
  my_ip(2, param1, param2);
  my_ip(3, param1, param2);
  return 0;
}
