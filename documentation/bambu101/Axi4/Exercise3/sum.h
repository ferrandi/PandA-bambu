#pragma HLS interface port = v mode = m_axi offset = direct bundle = test
#pragma HLS interface port = n mode = m_axi offset = direct bundle = test
int sum(int* v, unsigned* n);