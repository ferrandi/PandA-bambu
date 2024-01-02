#pragma HLS interface port = a mode = handshake
#pragma HLS interface port = b mode = handshake
#pragma HLS interface port = c mode = handshake
#pragma HLS interface port = d mode = handshake
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
