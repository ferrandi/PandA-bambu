#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_softmax( void* args,  void* arg_type_ids, int32_t num_args) {
  
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* tensor = (float*)(((TVMArray*)arg1)[0].data);
  
  float tensor1[1];
  float tensor2[8];
  float tensor3[1];
  tensor1[0] = -3.402823e+38f;
  
  for (int32_t k1 = 0; k1 < 8; ++k1) {
    float _1 = tensor1[0];
    float _2 = placeholder[k1];
    tensor1[0] = ((_1) > (_2) ? (_1) : (_2));
  }
  
  for (int32_t ax1 = 0; ax1 < 8; ++ax1) {
    tensor2[ax1] = expf((placeholder[ax1] - tensor1[0]));
  }
  
  tensor3[0] = 0.000000e+00f;
  for (int32_t k2 = 0; k2 < 8; ++k2) {
    tensor3[0] = (tensor3[0] + tensor2[k2]);
  }
  
  for (int32_t ax11 = 0; ax11 < 8; ++ax11) {
    tensor[ax11] = (tensor2[ax11] / tensor3[0]);
  }
  return 0;
}

