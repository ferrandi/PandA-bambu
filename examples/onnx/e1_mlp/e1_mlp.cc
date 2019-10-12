#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
extern void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_dense_add( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg0_code = (( int32_t*)arg_type_ids)[0];
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  int32_t arg1_code = (( int32_t*)arg_type_ids)[1];
  void* arg2 = (((TVMValue*)args)[2].v_handle);
  int32_t arg2_code = (( int32_t*)arg_type_ids)[2];
  void* arg3 = (((TVMValue*)args)[3].v_handle);
  int32_t arg3_code = (( int32_t*)arg_type_ids)[3];
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  int64_t* arg0_shape = (int64_t*)(((TVMArray*)arg0)[0].shape);
  int64_t* arg0_strides = (int64_t*)(((TVMArray*)arg0)[0].strides);
  int32_t dev_type = (((TVMArray*)arg0)[0].ctx.device_type);
  int32_t dev_id = (((TVMArray*)arg0)[0].ctx.device_id);
  float* placeholder1 = (float*)(((TVMArray*)arg1)[0].data);
  int64_t* arg1_shape = (int64_t*)(((TVMArray*)arg1)[0].shape);
  int64_t* arg1_strides = (int64_t*)(((TVMArray*)arg1)[0].strides);
  float* placeholder2 = (float*)(((TVMArray*)arg2)[0].data);
  int64_t* arg2_shape = (int64_t*)(((TVMArray*)arg2)[0].shape);
  int64_t* arg2_strides = (int64_t*)(((TVMArray*)arg2)[0].strides);
  float* T_add = (float*)(((TVMArray*)arg3)[0].data);
  int64_t* arg3_shape = (int64_t*)(((TVMArray*)arg3)[0].shape);
  int64_t* arg3_strides = (int64_t*)(((TVMArray*)arg3)[0].strides);
  if (!(arg0_strides == NULL)) {
  }
  if (!(arg1_strides == NULL)) {
  }
  if (!(arg2_strides == NULL)) {
  }
  if (!(arg3_strides == NULL)) {
  }
   float compute[10];
  for (int32_t y_outer_x_outer_fused = 0; y_outer_x_outer_fused < 10; ++y_outer_x_outer_fused) {
     float compute1[16];
    (( float16*)(compute1 + 0))[0] = ((float16)(0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f, 0.000000e+00f));
    for (int32_t k = 0; k < 49; ++k) {
      (( float16*)(compute1 + 0))[0] = ((( float16*)(compute1 + 0))[0] + ((( float16*)(placeholder + (k * 16)))[0] * (( float16*)(placeholder1 + ((y_outer_x_outer_fused * 784) + (k * 16))))[0]));
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
  for (int32_t ax1 = 0; ax1 < 10; ++ax1) {
    T_add[ax1] = (compute[ax1] + placeholder2[ax1]);
  }
  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_softmax( void* args,  void* arg_type_ids, int32_t num_args) {
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
   float tensor1[1];
   float tensor2[10];
   float tensor3[1];
  tensor1[0] = -3.402823e+38f;
  for (int32_t k1 = 0; k1 < 10; ++k1) {
    float _1 = tensor1[0];
    float _2 = placeholder[k1];
    tensor1[0] = ((_1) > (_2) ? (_1) : (_2));
  }
  for (int32_t ax1 = 0; ax1 < 10; ++ax1) {
    tensor2[ax1] = expf((placeholder[ax1] - tensor1[0]));
  }
  tensor3[0] = 0.000000e+00f;
  for (int32_t k2 = 0; k2 < 10; ++k2) {
    tensor3[0] = (tensor3[0] + tensor2[k2]);
  }
  for (int32_t ax11 = 0; ax11 < 10; ++ax11) {
    tensor[ax11] = (tensor2[ax11] / tensor3[0]);
  }
  return 0;
}

