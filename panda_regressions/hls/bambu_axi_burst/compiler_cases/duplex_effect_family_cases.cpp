#if (defined(AXI_DUPLEX_EFFECT_CASE_01) + defined(AXI_DUPLEX_EFFECT_CASE_02) + defined(AXI_DUPLEX_EFFECT_CASE_03) + defined(AXI_DUPLEX_EFFECT_CASE_04) + defined(AXI_DUPLEX_EFFECT_CASE_05) + defined(AXI_DUPLEX_EFFECT_CASE_06) + defined(AXI_DUPLEX_EFFECT_CASE_07) + defined(AXI_DUPLEX_EFFECT_CASE_08) + defined(AXI_DUPLEX_EFFECT_CASE_09) + defined(AXI_DUPLEX_EFFECT_CASE_10) + defined(AXI_DUPLEX_EFFECT_CASE_11) + defined(AXI_DUPLEX_EFFECT_CASE_12) + defined(AXI_DUPLEX_EFFECT_CASE_13) + defined(AXI_DUPLEX_EFFECT_CASE_14) + defined(AXI_DUPLEX_EFFECT_CASE_15) + defined(AXI_DUPLEX_EFFECT_CASE_16) + defined(AXI_DUPLEX_EFFECT_CASE_17) + defined(AXI_DUPLEX_EFFECT_CASE_18)) != 1
#error "Select exactly one compiler fixture case macro"
#endif

#if defined(AXI_DUPLEX_EFFECT_CASE_01)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_bases_asymmetric_64(restricted_int_ptr src, restricted_int_ptr dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=8 num_read_outstanding=2 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_02)
extern "C" void distinct_bases_destination_first(int * __restrict dst, int * __restrict src, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_03)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_bases_mismatched_bounds(restricted_int_ptr src, restricted_int_ptr dst,
                                                 unsigned trip_count, unsigned read_bound,
                                                 unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    const unsigned varying_read_bound = read_bound + i;
    if (i < varying_read_bound) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_04)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_bases_one_restrict(restricted_int_ptr src, int *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_05)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_bases_source_first(restricted_int_ptr src, restricted_int_ptr dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_06)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_bases_two_loops(restricted_int_ptr src, restricted_int_ptr dst,
                                        unsigned read_count, unsigned write_count) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < read_count; ++i) value = src[i];
  for (unsigned i = 0; i < write_count; ++i) dst[i] = value;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_07)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_equal(restricted_int_ptr src, restricted_int_ptr dst,
                                      unsigned trip_count, unsigned access_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < access_bound) value = src[i];
    if (i < access_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_08)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_extra_cfg(restricted_int_ptr src, restricted_int_ptr dst,
                                           unsigned trip_count, unsigned read_bound,
                                           unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < read_bound && (i & 1U) == 0U) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_09)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_guard_bypass(restricted_int_ptr src, restricted_int_ptr dst,
                                              unsigned trip_count, unsigned read_bound,
                                              unsigned write_bound, unsigned flag) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (flag) goto load;
    if (i < read_bound) {
load:
      value = src[i];
    }
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_10)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_read_longer(restricted_int_ptr src, restricted_int_ptr dst,
                                             unsigned trip_count, unsigned read_bound,
                                             unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < read_bound) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_11)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_signed_bound(restricted_int_ptr src, restricted_int_ptr dst,
                                              unsigned trip_count, int read_bound,
                                              unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (static_cast<int>(i) < read_bound) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_12)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_u64_bound(restricted_int_ptr src, restricted_int_ptr dst,
                                            unsigned trip_count, unsigned long long read_bound,
                                            unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < read_bound) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_13)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_width_bound(restricted_int_ptr src, restricted_int_ptr dst,
                                             unsigned trip_count, unsigned short read_bound,
                                             unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < read_bound) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_14)
typedef int * __restrict restricted_int_ptr;

extern "C" void distinct_prefix_write_longer(restricted_int_ptr src, restricted_int_ptr dst,
                                              unsigned trip_count, unsigned read_bound,
                                              unsigned write_bound) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  int value = 0;
  for (unsigned i = 0; i < trip_count; ++i) {
    if (i < read_bound) value = src[i];
    if (i < write_bound) dst[i] = value;
  }
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_15)
extern "C" void same_base_asymmetric_64(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=8 num_read_outstanding=2 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void unrelated_architectureless_root(int *data, int n) {
  data[0] = n;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_16)
extern "C" void same_base_pointwise(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=16
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void unrelated_architectureless_root(int *data, int n) {
  data[0] = n;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_17)
extern "C" void same_base_pointwise_reversed(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
  for (int i = 0; i < n; ++i) {
    const int value = data[i];
    data[i] = value * 3 + 1;
  }
}

extern "C" void unrelated_architectureless_root(int *data, int n) {
  data[0] = n;
}
#elif defined(AXI_DUPLEX_EFFECT_CASE_18)
typedef int * __restrict restricted_int_ptr;

extern "C" void restrict_both(int * __restrict input, int * __restrict output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}

extern "C" void restrict_typedef(restricted_int_ptr input, int *output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}

extern "C" void restrict_absent(int *input, int *output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}

extern "C" void restrict_one(int * __restrict input, int *output, int n)
{
#pragma HLS interface mode=m_axi port=input offset=direct bundle=shared
#pragma HLS interface mode=m_axi port=output offset=direct bundle=shared
   output[0] = input[0] + n;
}
#endif
