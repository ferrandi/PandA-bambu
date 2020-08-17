#pragma HLS_interface a handshake
#pragma HLS_interface b handshake
#pragma HLS_interface c handshake
#pragma HLS_interface d handshake
void sum3numbers(short* a, short* b, short* c, short* d)
{
   *d = *a + *b + *c;
}
