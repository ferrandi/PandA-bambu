#include "c_backend_api.h"

#ifdef BAMBU_PROFILING
extern void __builtin_bambu_time_start();
extern void __builtin_bambu_time_stop();
#endif

TVMValue param[4];
TVMArray a0[1];
TVMArray a1[1];
TVMArray a2[1];
TVMArray a3[1];

__attribute__((noinline))
void kernel(int32_t y_outer_x_outer_fused, float *compute, float* placeholder, float* placeholder1)
{
    float compute1[1];
    compute1[0] = 0.000000e+00f;
    compute1[0] = (compute1[0] + (placeholder[0] * placeholder1[y_outer_x_outer_fused]));
    compute[y_outer_x_outer_fused] = 0.000000e+00f;
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[0]);
}

__attribute__((noinline))
void parallel1(float *compute, float* placeholder, float* placeholder1)
{
  int32_t y_outer_x_outer_fused;
  #pragma omp parallel for
  for (y_outer_x_outer_fused = 0; y_outer_x_outer_fused < 8; ++y_outer_x_outer_fused) 
  {
    kernel(y_outer_x_outer_fused, compute, placeholder, placeholder1);
  }
}


__attribute__((noinline))
int32_t fused_nn_dense_add( void* args,  void* arg_type_ids, int32_t num_args) 
{

  void* arg0 = (((TVMValue*)args)[0].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* placeholder1 = (float*)(((TVMArray*)arg1)[0].data);
  
  void* arg2 = (((TVMValue*)args)[2].v_handle);
  float* placeholder2 = (float*)(((TVMArray*)arg2)[0].data);
  
  void* arg3 = (((TVMValue*)args)[3].v_handle);
  float* T_add = (float*)(((TVMArray*)arg3)[0].data);
  
  float compute[8];
  parallel1(compute, placeholder, placeholder1);
  int32_t ax1;
  for (ax1 = 0; ax1 < 8; ++ax1) {
    T_add[ax1] = (compute[ax1] + placeholder2[ax1]);
  }
  return 0;
}

int32_t fused_nn_dense_add_wrapper(float* placeholder, float* placeholder1, float* placeholder2, float* T_add)
{
  int32_t res;
  a0[0].data = placeholder;
  a1[0].data = placeholder1;
  a2[0].data = placeholder2;
  a3[0].data = T_add;
  param[0].v_handle = a0;
  param[1].v_handle = a1;
  param[2].v_handle = a2;
  param[3].v_handle = a3;
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif

  res = fused_nn_dense_add(param, 0, 0);

#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif

  return res;
}
