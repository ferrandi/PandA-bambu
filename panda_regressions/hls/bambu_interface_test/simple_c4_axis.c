#pragma HLS interface port = a mode = axis
#pragma HLS interface port = b mode = axis
#pragma HLS interface port = c mode = axis
#pragma HLS interface port = d mode = axis
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
