#pragma HLS interface port = a mode = valid
#pragma HLS interface port = b mode = valid
#pragma HLS interface port = c mode = valid
#pragma HLS interface port = d mode = bus
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
