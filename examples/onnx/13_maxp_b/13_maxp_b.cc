#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_max_pool2d( void* args,  void* arg_type_ids, int32_t num_args) {
  
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* tensor = (float*)(((TVMArray*)arg1)[0].data);
  
  for (int32_t ax2 = 0; ax2 < 10; ++ax2) {
    for (int32_t ax3 = 0; ax3 < 10; ++ax3) {
      tensor[((ax2 * 10) + ax3)] = -3.402823e+38f;
      for (int32_t rv = 0; rv < 3; ++rv) {
        for (int32_t rv1 = 0; rv1 < 3; ++rv1) {
          float _1 = tensor[((ax2 * 10) + ax3)];
          float _2 = placeholder[((((ax2 * 96) + (rv * 32)) + (ax3 * 3)) + rv1)];
          tensor[((ax2 * 10) + ax3)] = ((_1) > (_2) ? (_1) : (_2));
        }
      }
    }
  }
  return 0;
}

