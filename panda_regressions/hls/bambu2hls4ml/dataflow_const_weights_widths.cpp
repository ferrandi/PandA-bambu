#include <hls_stream.h>

// Two instances of the same dataflow module template take constant weight arrays of different sizes through a
// parameter with the same name, as two hls4ml Dense layers do (4x7 and 7x2 16-bit weights: 448 and 224 bits).
// The arrays must be folded into the modules: a "weights" bundle shared by the two instances would need two
// different data widths.

static const short w1[4 * 7] = {1, -2, 3,  -4, 5,  -6, 7,  -1, 2, -3, 4,  -5, 6,  -7,
                                2, 1,  -1, 3,  -2, 4,  -3, 1,  1, 2,  -2, 3,  -3, 4};
static const short b1[7] = {1, -1, 2, -2, 3, -3, 4};
static const short w2[7 * 2] = {3, -1, 2, 4, -2, 1, 5, -3, 1, 2, -4, 3, 2, -1};
static const short b2[2] = {-5, 7};

template <int N_IN, int N_OUT>
void dense_kernel(const int data[N_IN], int res[N_OUT], const short weights[N_IN * N_OUT], const short biases[N_OUT])
{
#pragma HLS ARRAY_PARTITION variable = weights complete
#pragma HLS ARRAY_PARTITION variable = biases complete
#pragma clang loop unroll(full)
   for(int j = 0; j < N_OUT; ++j)
   {
      int acc = biases[j];
#pragma clang loop unroll(full)
      for(int i = 0; i < N_IN; ++i)
      {
         acc += data[i] * weights[i * N_OUT + j];
      }
      res[j] = acc;
   }
}

template <int N_IN, int N_OUT>
void dense(hls::stream<int>& data_stream, hls::stream<int>& res_stream, const short weights[N_IN * N_OUT],
           const short biases[N_OUT])
{
   int data[N_IN];
   int res[N_OUT];
#pragma HLS ARRAY_PARTITION variable = data complete
#pragma HLS ARRAY_PARTITION variable = res complete
   for(int i = 0; i < N_IN; ++i)
   {
      data[i] = data_stream.read();
   }
   dense_kernel<N_IN, N_OUT>(data, res, weights, biases);
   for(int j = 0; j < N_OUT; ++j)
   {
      res_stream.write(res[j]);
   }
}

void dataflow_const_weights_widths(hls::stream<int>& input, hls::stream<int>& output)
{
#pragma HLS interface port = input mode = axis
#pragma HLS interface port = output mode = axis
#pragma HLS DATAFLOW
   hls::stream<int> layer1_out;
   dense<4, 7>(input, layer1_out, w1, b1);
   dense<7, 2>(layer1_out, output, w2, b2);
}
