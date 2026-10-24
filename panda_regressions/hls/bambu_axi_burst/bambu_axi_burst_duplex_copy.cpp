#ifndef DUPLEX_PROFILE
#error "DUPLEX_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

extern "C" void kernel(int * __restrict src, int * __restrict dst, int n) {
#if DUPLEX_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
#elif DUPLEX_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=2 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=8 write_burst_outstanding=1 write_fifo_depth=16
#elif DUPLEX_PROFILE == 3
// W9: the symmetric profile with two write bursts in flight (read side unchanged).
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=2 write_fifo_depth=32
#elif DUPLEX_PROFILE == 4
// W9: the symmetric profile with three write bursts in flight (read side unchanged).
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=3 write_fifo_depth=32
#else
#error "DUPLEX_PROFILE must be 1 (symmetric), 2 (asymmetric), 3 (symmetric, two writes) or 4 (symmetric, three writes)"
#endif
  for (int i = 0; i < n; ++i)
    dst[i] = src[i];
}
