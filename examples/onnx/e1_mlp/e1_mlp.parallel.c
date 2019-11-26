#include "c_backend_api.h"
#include <math.h>

#ifdef BAMBU_PROFILING
extern void __builtin_bambu_time_start();
extern void __builtin_bambu_time_stop();
#endif

TVMValue param1[4];
TVMValue param2[2];
TVMArray a0[1];
TVMArray a1[1];
TVMArray a2[1];
TVMArray a3[1];
TVMArray b0[1];
TVMArray b1[1];

__attribute__((noinline))
void kernel(int32_t y_outer_x_outer_fused, float *compute, float* placeholder, float* placeholder1)
{
    float compute1[16] = {0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f};
    int32_t k, i;
    for (k = 0; k < 49; ++k) {
      for(i=0; i < 16; ++i)
        compute1[i] = compute1[i] + (placeholder + (k * 16))[i] * (placeholder1 + ((y_outer_x_outer_fused * 784) + (k * 16)))[i];
    }
    compute[y_outer_x_outer_fused] = 0.000000e+00f;
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[0]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[1]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[2]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[3]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[4]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[5]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[6]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[7]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[8]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[9]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[10]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[11]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[12]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[13]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[14]);
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[15]);
}


__attribute__((noinline))
void parallel(float *compute, float* placeholder, float* placeholder1)
{
  int32_t y_outer_x_outer_fused;
  #pragma omp parallel for
  for (y_outer_x_outer_fused = 0; y_outer_x_outer_fused < 10; ++y_outer_x_outer_fused) 
  {
    kernel(y_outer_x_outer_fused, compute, placeholder, placeholder1);
  }
}

__attribute__((noinline))
int32_t fused_nn_dense_add( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  void* arg2 = (((TVMValue*)args)[2].v_handle);
  void* arg3 = (((TVMValue*)args)[3].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  float* placeholder1 = (float*)(((TVMArray*)arg1)[0].data);
  float* placeholder2 = (float*)(((TVMArray*)arg2)[0].data);
  float* T_add = (float*)(((TVMArray*)arg3)[0].data);
  float compute[10];

  
  parallel(compute, placeholder, placeholder1);
  

  int32_t ax1;
  for (ax1 = 0; ax1 < 10; ++ax1) {
    T_add[ax1] = (compute[ax1] + placeholder2[ax1]);
  }
  return 0;
}


__attribute__((noinline))
int32_t fused_nn_softmax( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  float* tensor = (float*)(((TVMArray*)arg1)[0].data);
  float tensor1[1];
  float tensor2[10];
  float tensor3[1];
  tensor1[0] = -3.402823e+38f;
  int32_t k1;
  for (k1 = 0; k1 < 10; ++k1) {
    float _1 = tensor1[0];
    float _2 = placeholder[k1];
    tensor1[0] = ((_1) > (_2) ? (_1) : (_2));
  }
  int32_t ax1;
  for (ax1 = 0; ax1 < 10; ++ax1) {
    tensor2[ax1] = expf((placeholder[ax1] - tensor1[0]));
  }
  tensor3[0] = 0.000000e+00f;
  int32_t k2;
  for (k2 = 0; k2 < 10; ++k2) {
    tensor3[0] = (tensor3[0] + tensor2[k2]);
  }
  int32_t ax11;
  for (ax11 = 0; ax11 < 10; ++ax11) {
    tensor[ax11] = (tensor2[ax11] / tensor3[0]);
  }
  return 0;
}

int32_t mlp(TVMValue* param1, TVMValue* param2){
  int32_t res1, res2;
  res1 = fused_nn_dense_add(param1, 0, 0);
  res2 = fused_nn_softmax(param2, 0, 0);
  return res2;
}


int32_t mlp_wrapper(float* placeholder, float* placeholder1, float* placeholder2, float* tensor)
{
  float T_add[10];
  int32_t res, res1, res2;
  a0[0].data = placeholder;
  a1[0].data = placeholder1;
  a2[0].data = placeholder2;
  a3[0].data = T_add;
  param1[0].v_handle = a0;
  param1[1].v_handle = a1;
  param1[2].v_handle = a2;
  param1[3].v_handle = a3;
  
  b0[0].data = T_add;
  b1[0].data = tensor;
  param2[0].v_handle = b0;
  param2[1].v_handle = b1;
  
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif

  res = mlp(param1, param2);

#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif

  return res;
}
