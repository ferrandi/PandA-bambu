// Copyright (C) 2026 Fabrizio Ferrandi
// AXI write-outstanding pragma aliases and validation fixtures.

#if (defined(AXI_WRITE_OUTSTANDING_ALIAS) + defined(AXI_WRITE_OUTSTANDING_EQUAL) + \
     defined(AXI_WRITE_OUTSTANDING_EQUAL_PORTS) + defined(AXI_WRITE_OUTSTANDING_CONFLICT) + \
     defined(AXI_WRITE_OUTSTANDING_PORT_CONFLICT) + defined(AXI_WRITE_OUTSTANDING_ZERO) + \
     defined(AXI_WRITE_OUTSTANDING_17) + defined(AXI_WRITE_OUTSTANDING_OVERFLOW) + \
     defined(AXI_WRITE_OUTSTANDING_MISSING) + defined(AXI_WRITE_OUTSTANDING_NONDECIMAL)) != 1
#error "Select exactly one write-outstanding alias fixture case macro"
#endif

#if defined(AXI_WRITE_OUTSTANDING_ALIAS)
extern "C" void write_outstanding_vitis_alias(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 num_write_outstanding=3
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(AXI_WRITE_OUTSTANDING_EQUAL)
extern "C" void write_outstanding_equal_aliases(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 num_write_outstanding=03 write_burst_outstanding=3
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(AXI_WRITE_OUTSTANDING_EQUAL_PORTS)
extern "C" void write_outstanding_equal_ports(int *a, int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_write_burst_length=16 num_write_outstanding=03
#pragma HLS interface mode=m_axi port=b offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=3
  for (int i = 0; i < n; ++i) { a[i] = i; b[i] = i; }
}
#elif defined(AXI_WRITE_OUTSTANDING_CONFLICT)
extern "C" void write_outstanding_conflict(int *dst, int n) {
#pragma HLS interface mode=m_axi port=dst offset=direct bundle=data max_write_burst_length=16 num_write_outstanding=3 write_burst_outstanding=4
  for (int i = 0; i < n; ++i) dst[i] = i;
}
#elif defined(AXI_WRITE_OUTSTANDING_PORT_CONFLICT)
extern "C" void write_outstanding_port_conflict(int *a, int *b, int n) {
#pragma HLS interface mode=m_axi port=a offset=direct bundle=data max_write_burst_length=16 num_write_outstanding=3
#pragma HLS interface mode=m_axi port=b offset=direct bundle=data max_write_burst_length=16 write_burst_outstanding=4
  for (int i = 0; i < n; ++i) { a[i] = i; b[i] = i; }
}
#elif defined(AXI_WRITE_OUTSTANDING_ZERO)
extern "C" int write_outstanding_zero(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src bundle=data num_write_outstanding=0
  return n > 0 ? src[0] : 0;
}
#elif defined(AXI_WRITE_OUTSTANDING_17)
extern "C" int write_outstanding_17(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src bundle=data num_write_outstanding=17
  return n > 0 ? src[0] : 0;
}
#elif defined(AXI_WRITE_OUTSTANDING_OVERFLOW)
extern "C" int write_outstanding_overflow(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src bundle=data num_write_outstanding=999999999999999999999999999999999999999999999999
  return n > 0 ? src[0] : 0;
}
#elif defined(AXI_WRITE_OUTSTANDING_MISSING)
extern "C" int write_outstanding_missing(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src bundle=data num_write_outstanding
  return n > 0 ? src[0] : 0;
}
#elif defined(AXI_WRITE_OUTSTANDING_NONDECIMAL)
extern "C" int write_outstanding_nondecmial(const int *src, int n) {
#pragma HLS interface mode=m_axi port=src bundle=data num_write_outstanding=3x
  return n > 0 ? src[0] : 0;
}
#endif
