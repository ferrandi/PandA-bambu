#pragma HLS_interface a fifo
#pragma HLS_interface b fifo
#pragma HLS_interface c fifo
#pragma HLS_interface d fifo
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
