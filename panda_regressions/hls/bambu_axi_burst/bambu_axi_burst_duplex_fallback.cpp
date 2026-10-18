#ifndef DUPLEX_FALLBACK_PROFILE
#error DUPLEX_FALLBACK_PROFILE must select a duplex fallback profile
#endif

#define DUPLEX_FALLBACK_N 38

#if DUPLEX_FALLBACK_PROFILE < 1 || DUPLEX_FALLBACK_PROFILE > 4
#error unsupported DUPLEX_FALLBACK_PROFILE
#endif

extern "C" void bambu_axi_burst_duplex_fallback(int *data, int n) {
#if DUPLEX_FALLBACK_PROFILE == 1
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
  int snapshot[DUPLEX_FALLBACK_N];
  for (int i = 0; i < n; ++i) {
    snapshot[i] = data[i];
  }
  for (int i = 0; i < n; ++i) {
    data[i] = snapshot[n - 1 - i];
  }
#elif DUPLEX_FALLBACK_PROFILE == 2
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
  for (int i = 0; i < n; ++i) {
    data[i + 1] = data[i] + 1;
  }
#elif DUPLEX_FALLBACK_PROFILE == 3
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
  int state = data[0] & 1;
  int checksum = 0;
  for (int i = 1; i < n; ++i) {
    if (state != 0) {
      const int loaded = data[i];
      checksum += loaded;
      state = (loaded ^ i) & 1;
    } else {
      data[i] = 0x2468 + i;
      state ^= 1;
    }
  }
  data[n] = state;
  data[n + 1] = checksum;
#elif DUPLEX_FALLBACK_PROFILE == 4
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=64
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1 write_fifo_depth=32
  for (int i = 0; i < n; ++i) {
    const int loaded = data[i];
    data[i] = loaded + 7;
    data[i + 64] = loaded ^ 0x13579;
  }
#endif
}
