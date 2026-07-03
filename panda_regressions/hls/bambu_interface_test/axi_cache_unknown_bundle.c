#pragma HLS interface port = x mode = m_axi offset = direct bundle = test

#pragma HLS cache bundle = gmem0 line_count = 2 line_size = 8 bus_size = 256 ways = 1 num_write_outstanding = 1 rep_policy = \
    lru write_policy = wt
void kernel(int* x)
{
   x[0] = x[0] + 1;
}
