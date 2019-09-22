#include "c_backend_api.h"

TVMValue param[2];
TVMArray a0[1];
TVMArray a1[1];

int32_t fused_tanh_exp_nn_relu_sigmoid( void* args,  void* arg_type_ids, int32_t num_args);
#ifdef BAMBU_PROFILING
extern void __builtin_bambu_time_start();
extern void __builtin_bambu_time_stop();
#endif

int32_t fused_activations_wrapper(float* placeholder, float* T_sigmoid)
{
  int32_t res;
  a0[0].data = placeholder;
  a1[0].data = T_sigmoid;
  param[0].v_handle = a0;
  param[1].v_handle = a1;
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif

  res = fused_tanh_exp_nn_relu_sigmoid(param, 0, 0);

#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif

  return res;
}
