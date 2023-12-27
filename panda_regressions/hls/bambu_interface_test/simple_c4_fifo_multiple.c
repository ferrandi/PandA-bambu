#pragma HLS interface port = a mode = fifo
#pragma HLS interface port = b mode = fifo
#pragma HLS interface port = c mode = fifo
#pragma HLS interface port = d mode = fifo
void sum3numbers(short* a, short* b, short* c, short* d)
{
   int i;
#pragma nounroll
   for(i = 0; i < 8; ++i)
      d[i] = a[i] + b[i] + c[i];
}
