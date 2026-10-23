#ifndef MDPI_EXTENT_CASE
#error MDPI_EXTENT_CASE must select an extent test case
#endif

#if MDPI_EXTENT_CASE == 1 || MDPI_EXTENT_CASE == 5 || MDPI_EXTENT_CASE == 7
extern "C" void kernel(int *data) {
#elif MDPI_EXTENT_CASE == 4
extern "C" void kernel(unsigned char *data) {
#elif MDPI_EXTENT_CASE == 2 || MDPI_EXTENT_CASE == 6
extern "C" void kernel(int data[1]) {
#elif MDPI_EXTENT_CASE == 3
extern "C" void kernel(unsigned char data[1]) {
#else
#error unsupported MDPI_EXTENT_CASE
#endif
#pragma HLS interface mode=m_axi port=data offset=direct bundle=memory
  data[0] += 1;
}
