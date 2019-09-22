#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
TVM_DLL int32_t fused_layout_transform_2( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  float* T_layout_trans = (float*)(((TVMArray*)arg1)[0].data);
  for (int32_t ax0_ax1_fused_ax2_fused = 0; ax0_ax1_fused_ax2_fused < 64; ++ax0_ax1_fused_ax2_fused) {
    for (int32_t ax3 = 0; ax3 < 64; ++ax3) {
      T_layout_trans[((ax0_ax1_fused_ax2_fused * 64) + ax3)] = placeholder[((ax0_ax1_fused_ax2_fused * 64) + ax3)];
    }
  }
  return 0;
}

TVM_DLL int32_t fused_layout_transform_1( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  float* T_layout_trans = (float*)(((TVMArray*)arg1)[0].data);
  for (int32_t ax0_ax1_fused = 0; ax0_ax1_fused < 2; ++ax0_ax1_fused) {
    for (int32_t ax2 = 0; ax2 < 64; ++ax2) {
      for (int32_t ax3 = 0; ax3 < 64; ++ax3) {
        T_layout_trans[(((ax0_ax1_fused * 4096) + (ax2 * 64)) + ax3)] = placeholder[(((ax2 * 128) + (ax3 * 2)) + ax0_ax1_fused)];
      }
    }
  }
  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t fused_nn_contrib_conv2d_NCHWc( void* args,  void* arg_type_ids, int32_t num_args) {
  void* arg0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg0_code = (( int32_t*)arg_type_ids)[0];
  void* arg1 = (((TVMValue*)args)[1].v_handle);
  int32_t arg1_code = (( int32_t*)arg_type_ids)[1];
  void* arg2 = (((TVMValue*)args)[2].v_handle);
  int32_t arg2_code = (( int32_t*)arg_type_ids)[2];
  float* placeholder = (float*)(((TVMArray*)arg0)[0].data);
  int64_t* arg0_shape = (int64_t*)(((TVMArray*)arg0)[0].shape);
  int64_t* arg0_strides = (int64_t*)(((TVMArray*)arg0)[0].strides);
  int32_t dev_type = (((TVMArray*)arg0)[0].ctx.device_type);
  int32_t dev_id = (((TVMArray*)arg0)[0].ctx.device_id);
  float* placeholder1 = (float*)(((TVMArray*)arg1)[0].data);
  int64_t* arg1_shape = (int64_t*)(((TVMArray*)arg1)[0].shape);
  int64_t* arg1_strides = (int64_t*)(((TVMArray*)arg1)[0].strides);
  float* conv2d_NCHWc = (float*)(((TVMArray*)arg2)[0].data);
  int64_t* arg2_shape = (int64_t*)(((TVMArray*)arg2)[0].shape);
  int64_t* arg2_strides = (int64_t*)(((TVMArray*)arg2)[0].strides);
  if (!(arg0_strides == NULL)) {
  }
  if (!(arg1_strides == NULL)) {
  }
  if (!(arg2_strides == NULL)) {
  }
  void* data_pad = TVMBackendAllocWorkspace(1, dev_id, (uint64_t)17424, 2, 32);
  if (data_pad == NULL) {
    return -1;
  }
  for (int32_t i1_i2_fused = 0; i1_i2_fused < 66; ++i1_i2_fused) {
    for (int32_t i3 = 0; i3 < 66; ++i3) {
      (( float*)data_pad)[((i1_i2_fused * 66) + i3)] = (((((1 <= i1_i2_fused) && (i1_i2_fused < 65)) && (1 <= i3)) && (i3 < 65)) ? placeholder[(((i1_i2_fused * 64) + i3) - 65)] : 0.000000e+00f);
    }
  }
  for (int32_t n_oc_chunk_fused_oh_fused = 0; n_oc_chunk_fused_oh_fused < 64; ++n_oc_chunk_fused_oh_fused) {
     float2 conv2d_NCHWc_global[16];
    for (int32_t ow_outer = 0; ow_outer < 4; ++ow_outer) {
      conv2d_NCHWc_global[0] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[1] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[2] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[3] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[4] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[5] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[6] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[7] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[8] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[9] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[10] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[11] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[12] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[13] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[14] = ((float2)(0.000000e+00f, 0.000000e+00f));
      conv2d_NCHWc_global[15] = ((float2)(0.000000e+00f, 0.000000e+00f));
      for (int32_t kh = 0; kh < 3; ++kh) {
        for (int32_t kw = 0; kw < 3; ++kw) {
          conv2d_NCHWc_global[0] = (conv2d_NCHWc_global[0] + (((float2)((( float*)data_pad)[((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw)], (( float*)data_pad)[((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[1] = (conv2d_NCHWc_global[1] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 1)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 1)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[2] = (conv2d_NCHWc_global[2] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 2)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 2)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[3] = (conv2d_NCHWc_global[3] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 3)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 3)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[4] = (conv2d_NCHWc_global[4] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 4)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 4)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[5] = (conv2d_NCHWc_global[5] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 5)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 5)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[6] = (conv2d_NCHWc_global[6] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 6)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 6)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[7] = (conv2d_NCHWc_global[7] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 7)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 7)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[8] = (conv2d_NCHWc_global[8] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 8)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 8)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[9] = (conv2d_NCHWc_global[9] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 9)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 9)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[10] = (conv2d_NCHWc_global[10] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 10)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 10)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[11] = (conv2d_NCHWc_global[11] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 11)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 11)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[12] = (conv2d_NCHWc_global[12] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 12)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 12)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[13] = (conv2d_NCHWc_global[13] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 13)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 13)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[14] = (conv2d_NCHWc_global[14] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 14)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 14)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
          conv2d_NCHWc_global[15] = (conv2d_NCHWc_global[15] + (((float2)((( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 15)], (( float*)data_pad)[(((((kh * 66) + (n_oc_chunk_fused_oh_fused * 66)) + (ow_outer * 16)) + kw) + 15)])) * (( float2*)(placeholder1 + ((kh * 6) + (kw * 2))))[0]));
        }
      }
      for (int32_t ow_inner = 0; ow_inner < 16; ++ow_inner) {
        (( float2*)(conv2d_NCHWc + (((n_oc_chunk_fused_oh_fused * 128) + (ow_outer * 32)) + (ow_inner * 2))))[0] = (( float2*)(( float*)conv2d_NCHWc_global + (ow_inner * 2)))[0];
      }
    }
  }
  if (TVMBackendFreeWorkspace(1, dev_id, data_pad) != 0) {
    return -1;
  }
  return 0;
}

