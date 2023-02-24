#pragma HLS_interface data m_axi direct

short int read(short int* data)
{
   return *(data);
}