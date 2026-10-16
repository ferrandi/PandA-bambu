// Manual analysis-only same-base duplex candidates. Each top is compiled separately.

extern "C" void same_base_pointwise(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void same_base_pointwise_reversed(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void same_base_cached(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS cache bundle=shared line_count=16 line_size=32 ways=2 rep_policy=lru write_policy=wt
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void shifted_index(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 1; i < n; ++i) data[i] = data[i - 1] + 1;
}

extern "C" void stride_index(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[2 * i] = data[2 * i] + 1;
}

extern "C" void conditional_store(int *data, int n, int enable) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) if (enable) data[i] = data[i] + 1;
}

extern "C" void residual_store(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[i] = data[i] + 1;
  data[n] = 0;
}

extern "C" void volatile_effect(volatile int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[i] = data[i] + 1;
}

extern "C" void separate_phase(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[i] = data[i] + 1;
  for (int i = 0; i < n; ++i) data[i] = data[i] + 1;
}

extern "C" void distinct_bases(int *src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}
