#ifndef BANKS_NUMBER
#define BANKS_NUMBER 2
#endif

#define N 20
#if BANKED_BUS_AXI == 1
#pragma HLS bus bank_number = BANKS_NUMBER chunk_size = 32 mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#elif BANKED_BUS == 1
#pragma HLS bus bank_number = 2 chunk_size = 32
#pragma HLS interface port = a mode = bus
#elif BUS_AXI == 1
#pragma HLS bus mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#else
#pragma HLS interface port = a mode = bus
#endif
#if FIFO == 1
#pragma HLS interface port = b mode = fifo
#elif AXI_CACHE == 1
#pragma HLS interface port = b mode = m_axi offset = direct bundle = gmem0
#pragma HLS cache bundle = gmem0 line_count = 16 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif AXI == 1
#pragma HLS interface port = b mode = m_axi offset = direct bundle = gmem0
#else
#pragma HLS interface port = b mode = bus
#endif
unsigned int vector_add(int* a, int* b)
{
   unsigned int sum = 0;
   unsigned i;
   for(i = 0; i < N; i++)
   {
      sum += a[i] + b[i];
   }
   return sum;
}

#if BANKED_BUS_AXI == 1
#pragma HLS bus bank_number = 2 chunk_size = 32 mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#elif BANKED_BUS == 1
#pragma HLS bus bank_number = 2 chunk_size = 32
#pragma HLS interface port = a mode = bus
#elif BUS_AXI == 1
#pragma HLS bus mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#else
#pragma HLS interface port = a mode = bus
#endif
#if INFERRED == 1
#else
#pragma HLS interface port = b mode = array elem_count = N
#endif
unsigned int vector_add_array(int* a, int b[])
{
   unsigned int sum = 0;
   unsigned i;
   for(i = 0; i < N; i++)
   {
      sum += a[i] + b[i];
   }
   return sum;
}

#if BANKED_BUS_AXI == 1
#pragma HLS bus bank_number = 2 chunk_size = 32 mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#pragma HLS interface port = b mode = bus offset = direct
#elif BANKED_BUS == 1
#pragma HLS bus bank_number = 2 chunk_size = 32
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#elif BUS_AXI == 1
#pragma HLS bus mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#pragma HLS interface port = b mode = bus offset = direct
#else
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#endif
#if NONE == 1
#pragma HLS interface port = z mode = none
#endif
unsigned int vector_add_int(int* a, int* b, int z)
{
   unsigned int sum = 0;
   unsigned i;
   for(i = 0; i < N; i++)
   {
      sum += a[i] + b[i] + z;
   }
   return sum;
}

#if BANKED_BUS_AXI == 1
#pragma HLS bus bank_number = 2 chunk_size = 32 mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#pragma HLS interface port = b mode = bus offset = direct
#pragma HLS interface port = z mode = bus offset = direct
#elif BANKED_BUS == 1
#pragma HLS bus bank_number = 2 chunk_size = 32
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = z mode = bus
#elif BUS_AXI == 1
#pragma HLS bus mode = m_axi
#pragma HLS interface port = a mode = bus offset = direct
#pragma HLS interface port = b mode = bus offset = direct
#pragma HLS interface port = z mode = bus offset = direct
#else
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = z mode = bus
#endif
#if CACHES == 1
#pragma HLS cache bundle = bus line_count = 16 line_size = 32 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#endif
unsigned int vector_add_int_pointer(int* a, int* b, int* z)
{
   unsigned int sum = 0;
   unsigned i;
   for(i = 0; i < N; i++)
   {
      sum += a[i] + b[i] + z[0];
   }
   return sum;
}