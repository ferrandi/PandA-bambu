#pragma HLS interface port = a mode = acknowledge
#pragma HLS interface port = b mode = acknowledge
#pragma HLS interface port = c mode = acknowledge
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
