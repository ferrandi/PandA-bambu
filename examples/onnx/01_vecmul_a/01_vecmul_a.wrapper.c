#include "c_backend_api.h"

TVMValue param[3];
TVMArray a0[1];
TVMArray a1[1];
TVMArray a2[1];

int32_t fused_multiply( void* args,  void* arg_type_ids, int32_t num_args);
#ifdef BAMBU_PROFILING
extern void __builtin_bambu_time_start();
extern void __builtin_bambu_time_stop();
#endif

int32_t fused_multiply_wrapper(float* placeholder, float* placeholder1, float* T_multiply)
{
  int32_t res;
  a0[0].data = placeholder;
  a1[0].data = placeholder1;
  a2[0].data = T_multiply;
  param[0].v_handle = a0;
  param[1].v_handle = a1;
  param[2].v_handle = a2;
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif

  res = fused_multiply(param, 0, 0);

#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif

  return res;
}
