// Four feature combinations, sharing one access pattern per direction.
#ifndef AXI_PROFILE
#define AXI_PROFILE 0
#endif

#if defined(AXI_READ)
extern "C" int kernel(const int src[4096], int n)
{
#if AXI_PROFILE == 0
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=1 num_read_outstanding=1
#elif AXI_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=1
#elif AXI_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=1 num_read_outstanding=3
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=src max_read_burst_length=16 num_read_outstanding=3
#endif
   int result = 0;
   for(int i = 0; i < n; ++i)
      result += src[i];
   return result;
}
#elif defined(AXI_WRITE)
extern "C" void kernel(int dst[4096], int n)
{
#if AXI_PROFILE == 0
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=dst max_write_burst_length=1 num_write_outstanding=1
#elif AXI_PROFILE == 1
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=dst max_write_burst_length=16 num_write_outstanding=1
#elif AXI_PROFILE == 2
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=dst max_write_burst_length=1 num_write_outstanding=3
#else
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=dst max_write_burst_length=16 num_write_outstanding=3
#endif
   for(int i = 0; i < n; ++i)
      dst[i] = (i * 13 + 7) ^ (i >> 2);
}
#else
extern "C" void kernel(int* __restrict src, int* __restrict dst, int n)
{
#if AXI_PROFILE == 0
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=1 num_read_outstanding=1
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=1 num_write_outstanding=1
#elif AXI_PROFILE == 1
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 num_write_outstanding=1
#elif AXI_PROFILE == 2
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=1 num_read_outstanding=3
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=1 num_write_outstanding=3
#else
#pragma HLS interface mode=m_axi port=src offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=3
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=shared max_write_burst_length=16 num_write_outstanding=3
#endif
   for(int i = 0; i < n; ++i)
      dst[i] = src[i];
}
#endif
