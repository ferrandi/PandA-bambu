#include "c_backend_api.h"

TVMValue param[3];
TVMArray a0[1];
TVMArray a1[1];
TVMArray a2[1];

int32_t fused_nn_max_pool2d( void* args,  void* arg_type_ids, int32_t num_args);
#ifdef BAMBU_PROFILING
extern void __builtin_bambu_time_start();
extern void __builtin_bambu_time_stop();
#endif

int32_t fused_nn_max_pool2d_wrapper(float* placeholder, float* tensor)
{
  int32_t res;
  a0[0].data = placeholder;
  a1[0].data = tensor;
  param[0].v_handle = a0;
  param[1].v_handle = a1;
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif

  res = fused_nn_max_pool2d(param, 0, 0);

#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif

  return res;
}
