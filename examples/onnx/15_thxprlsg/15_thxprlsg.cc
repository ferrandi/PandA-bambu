#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_tanh_exp_nn_relu_sigmoid( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg0_code = (( int32_t*)arg_type_ids)[0];
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  int32_t arg1_code = (( int32_t*)arg_type_ids)[1];
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  int64_t* arg0_shape = (int64_t*)(((TVMArray*)arg0)[0].shape);
  int64_t* arg0_strides = (int64_t*)(((TVMArray*)arg0)[0].strides);
  int32_t dev_type = (((TVMArray*)arg0)[0].ctx.device_type);
  int32_t dev_id = (((TVMArray*)arg0)[0].ctx.device_id);
  float* T_sigmoid = (float*)(((TVMArray*)arg1)[0].data);
  int64_t* arg1_shape = (int64_t*)(((TVMArray*)arg1)[0].shape);
  int64_t* arg1_strides = (int64_t*)(((TVMArray*)arg1)[0].strides);
  if (!(arg0_strides == NULL)) {
  }
  if (!(arg1_strides == NULL)) {
  }
  for (int32_t ax1 = 0; ax1 < 8; ++ax1) {
    float _1 = placeholder[ax1];
    float _2 = (_1) < (9.000000e+00f) ? (_1) : (9.000000e+00f);
    float _3 = expf(((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * -2.760768e-16f) + 2.000188e-13f)) + -8.604672e-11f)) + 5.122297e-08f)) + 1.485722e-05f)) + 6.372619e-04f)) + 4.893525e-03f)) / (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * (((((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f)) * ((_2) > (-9.000000e+00f) ? (_2) : (-9.000000e+00f))) * 1.198258e-06f) + 1.185347e-04f)) + 2.268435e-03f)) + 4.893525e-03f)));
    T_sigmoid[ax1] = (1.000000e+00f / (1.000000e+00f + expf((0.000000e+00f - ((_3) > (0.000000e+00f) ? (_3) : (0.000000e+00f))))));
  }
  return 0;
}

