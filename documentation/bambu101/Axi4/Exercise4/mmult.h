#define rank 32
#define tile_rank 2

/* AXI pragmas */
#pragma HLS interface port = a mode = m_axi offset = direct bundle = gmem0
#pragma HLS interface port = b mode = m_axi offset = direct bundle = gmem1
#pragma HLS interface port = c_out mode = m_axi offset = direct bundle = gmem2

/* Cache pragmas */
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#pragma HLS cache bundle = gmem1 line_count = 32 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = tree write_policy = wt
#pragma HLS cache bundle = gmem2 line_count = 16 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    4 rep_policy = tree write_policy = wb

void mmult(int* a, int* b, int* c_out);