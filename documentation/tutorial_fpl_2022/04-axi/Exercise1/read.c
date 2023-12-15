#pragma HLS interface port = data mode = m_axi offset = direct

short int read(short int* data)
{
   return *(data);
}