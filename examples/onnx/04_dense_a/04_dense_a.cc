#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_dense_add( void* args,  void* arg_type_ids, int32_t num_args) {

  void* arg0 = (((TVMValue*)args)[0].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* placeholder1 = (float*)(((TVMArray*)arg1)[0].data);
  
  void* arg2 = (((TVMValue*)args)[2].v_handle);
  float* placeholder2 = (float*)(((TVMArray*)arg2)[0].data);
  
  void* arg3 = (((TVMValue*)args)[3].v_handle);
  float* T_add = (float*)(((TVMArray*)arg3)[0].data);
  
  float compute[8];
  for (int32_t y_outer_x_outer_fused = 0; y_outer_x_outer_fused < 8; ++y_outer_x_outer_fused) {
    float compute1[1];
    compute1[0] = 0.000000e+00f;
    compute1[0] = (compute1[0] + (placeholder[0] * placeholder1[y_outer_x_outer_fused]));
    compute[y_outer_x_outer_fused] = 0.000000e+00f;
    compute[y_outer_x_outer_fused] = (compute[y_outer_x_outer_fused] + compute1[0]);
  }
  for (int32_t ax1 = 0; ax1 < 8; ++ax1) {
    T_add[ax1] = (compute[ax1] + placeholder2[ax1]);
  }
  return 0;
}

