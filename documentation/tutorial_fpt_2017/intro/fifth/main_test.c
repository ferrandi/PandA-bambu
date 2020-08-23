#include "module_lib.h"

#ifdef BAMBU_PROFILING
extern void __builtin_bambu_time_start();
extern void __builtin_bambu_time_stop();
#endif

int main()
{
  uint32_t param1=10;
  uint32_t param2=10<<16;
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif
  my_ip(0, param1, param2);
  my_ip(1, param1, param2);
  my_ip(2, param1, param2);
  my_ip(3, param1, param2);
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif
  return 0;
}
