#define N 4

void top_fun(int in[N], int out[N])
{
#pragma HLS array_partition variable = in complete
#pragma HLS array_partition variable = out complete
#pragma HLS interface port = in mode = ptrdefault
#pragma HLS interface port = out mode = ptrdefault
   out[0] = in[0] + 1;
   out[1] = in[1] + 2;
   out[2] = in[2] + 3;
   out[3] = in[3] + 4;
}
