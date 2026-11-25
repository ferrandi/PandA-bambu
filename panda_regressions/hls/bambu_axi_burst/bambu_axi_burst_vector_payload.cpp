// Shared vector-payload fixture. VECTOR_BITS selects the memory element width;
// VECTOR_IN_PLACE selects the same-bundle duplex case.
#ifndef VECTOR_BITS
#error "VECTOR_BITS must be 32, 64, or 128"
#endif

#if VECTOR_BITS != 32 && VECTOR_BITS != 64 && VECTOR_BITS != 128
#error "unsupported vector payload width"
#endif

#ifndef VECTOR_IN_PLACE
#define VECTOR_IN_PLACE 0
#endif

typedef unsigned char vector_payload __attribute__((vector_size(VECTOR_BITS / 8)));

#if VECTOR_IN_PLACE
extern "C" void __attribute__((noinline)) kernel(const vector_payload *src,
                                                  vector_payload *dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#else
extern "C" void __attribute__((noinline)) kernel(const vector_payload *__restrict src,
                                                  vector_payload *__restrict dst, int n) {
#pragma HLS interface mode=m_axi port=src offset=direct bundle=input max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=output max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#endif
  for (int i = 0; i < n; ++i)
    dst[i] = src[i];
}
