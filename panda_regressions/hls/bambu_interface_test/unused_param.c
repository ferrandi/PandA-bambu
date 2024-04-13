unsigned long long test(_Bool *a1, _Bool a2[1], _Bool *a3, _Bool *a4, double b, double c)
{
#pragma HLS interface port = a1 mode = axis
#pragma HLS interface port = a3 mode = fifo
#pragma HLS interface port = a4 mode = m_axi offset = direct bundle = gmem0

   return b+c;
}



