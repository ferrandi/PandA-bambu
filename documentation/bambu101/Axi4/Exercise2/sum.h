#pragma HLS interface port = v mode = m_axi offset = direct
#pragma HLS interface port = n mode = m_axi offset = direct
int sum(int* v, unsigned* n);