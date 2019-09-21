#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_max_pool2d( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg0_code = (( int32_t*)arg_type_ids)[0];
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  int32_t arg1_code = (( int32_t*)arg_type_ids)[1];
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  int64_t* arg0_shape = (int64_t*)(((TVMArray*)arg0)[0].shape);
  int64_t* arg0_strides = (int64_t*)(((TVMArray*)arg0)[0].strides);
  int32_t dev_type = (((TVMArray*)arg0)[0].ctx.device_type);
  int32_t dev_id = (((TVMArray*)arg0)[0].ctx.device_id);
  float* tensor = (float*)(((TVMArray*)arg1)[0].data);
  int64_t* arg1_shape = (int64_t*)(((TVMArray*)arg1)[0].shape);
  int64_t* arg1_strides = (int64_t*)(((TVMArray*)arg1)[0].strides);
  if (!(arg0_strides == NULL)) {
  }
  if (!(arg1_strides == NULL)) {
  }
  for (int32_t ax2 = 0; ax2 < 2; ++ax2) {
    for (int32_t ax3 = 0; ax3 < 2; ++ax3) {
      tensor[((ax2 * 2) + ax3)] = -3.402823e+38f;
      for (int32_t rv = 0; rv < 3; ++rv) {
        for (int32_t rv1 = 0; rv1 < 3; ++rv1) {
          float _1 = tensor[((ax2 * 2) + ax3)];
          float _2 = placeholder[((((ax2 * 24) + (rv * 8)) + (ax3 * 3)) + rv1)];
          tensor[((ax2 * 2) + ax3)] = ((_1) > (_2) ? (_1) : (_2));
        }
      }
    }
  }
  return 0;
}

