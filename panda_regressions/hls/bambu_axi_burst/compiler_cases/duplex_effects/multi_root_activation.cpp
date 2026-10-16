#ifndef DUPLEX_MATRIX_CASE
#error Define DUPLEX_MATRIX_CASE to select one isolated activation scenario
#endif

#if DUPLEX_MATRIX_CASE == 1 || DUPLEX_MATRIX_CASE == 2
#if defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#endif
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if !defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#endif
#elif DUPLEX_MATRIX_CASE == 3 || DUPLEX_MATRIX_CASE == 4
#if defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 1; i < n; ++i) data[i] = data[i - 1] + 1;
}
#endif
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if !defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 1; i < n; ++i) data[i] = data[i - 1] + 1;
}
#endif
#elif DUPLEX_MATRIX_CASE == 5 || DUPLEX_MATRIX_CASE == 6
#if defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[i] = i;
}
#endif
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if !defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) data[i] = i;
}
#endif
#elif DUPLEX_MATRIX_CASE == 7 || DUPLEX_MATRIX_CASE == 8
#if defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
}
#endif
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if !defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
}
#endif
#elif DUPLEX_MATRIX_CASE == 9 || DUPLEX_MATRIX_CASE == 10
extern "C" __attribute__((noinline)) void matrix_dataflow_task(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *input, int n) {
#pragma HLS DATAFLOW
#pragma HLS interface mode=m_axi port=input offset=direct bundle=peer_input
  matrix_dataflow_task(input, n);
}
#endif
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if !defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *input, int n) {
#pragma HLS DATAFLOW
#pragma HLS interface mode=m_axi port=input offset=direct bundle=peer_input
  matrix_dataflow_task(input, n);
}
#endif
#elif DUPLEX_MATRIX_CASE == 11
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=DF_bambu_shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=DF_bambu_shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#elif DUPLEX_MATRIX_CASE == 12
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#elif DUPLEX_MATRIX_CASE == 13 || DUPLEX_MATRIX_CASE == 14
#if defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#if DUPLEX_MATRIX_CASE == 13
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=8 num_read_outstanding=1 read_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#endif
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#endif
extern "C" void matrix_candidate(int *data, int n) {
#if DUPLEX_MATRIX_CASE == 13
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=8 num_read_outstanding=1 read_fifo_depth=32
#endif
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#if !defined(DUPLEX_MATRIX_REVERSED)
extern "C" void matrix_peer(int *data, int n) {
#if DUPLEX_MATRIX_CASE == 13
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=8 num_read_outstanding=1 read_fifo_depth=32
#else
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#endif
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#endif
#elif DUPLEX_MATRIX_CASE == 15
extern "C" __attribute__((noinline)) void matrix_other_dataflow_task(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=other max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=other max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
extern "C" void matrix_peer(int *input, int n) {
#pragma HLS DATAFLOW
#pragma HLS interface mode=m_axi port=input offset=direct bundle=other
  matrix_other_dataflow_task(input, n);
}
#elif DUPLEX_MATRIX_CASE == 16
extern "C" __attribute__((noinline)) void matrix_helper(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
extern "C" void matrix_candidate(int *shared, int n) {
  matrix_helper(shared, n);
}
extern "C" void matrix_peer(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
#elif DUPLEX_MATRIX_CASE == 17
extern "C" void matrix_candidate(int *data, int n) {
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_read_burst_length=16 num_read_outstanding=1 read_fifo_depth=32
#pragma HLS interface mode=m_axi port=data offset=direct bundle=shared max_write_burst_length=16 write_burst_outstanding=1
  for (int i = 0; i < n; ++i) { const int value = data[i]; data[i] = value + 1; }
}
extern "C" void unrelated_architectureless(int *data, int n) { data[0] = n; }
#else
#error Unknown DUPLEX_MATRIX_CASE
#endif
