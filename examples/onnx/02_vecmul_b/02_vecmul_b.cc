#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_multiply( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* placeholder1 = (float*)(((TVMArray*)arg1)[0].data);
  
  void* arg2 = (((TVMValue*)args)[2].v_handle);
  float* T_multiply = (float*)(((TVMArray*)arg2)[0].data);
  
  for (int32_t ax1 = 0; ax1 < 64; ++ax1) {
    T_multiply[ax1] = (placeholder[ax1] * placeholder1[ax1]);
  }
  return 0;
}

