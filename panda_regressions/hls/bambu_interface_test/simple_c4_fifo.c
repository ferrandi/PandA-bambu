#pragma HLS interface port = a mode = fifo
#pragma HLS interface port = b mode = fifo
#pragma HLS interface port = c mode = fifo
#pragma HLS interface port = d mode = fifo
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
