#ifndef NTHREADS
#define NTHREADS 4
#endif

#ifndef MAX_ITERS
#define MAX_ITERS 4
#endif

#ifndef BANK_NUMBER
#define BANK_NUMBER 4
#endif

#ifndef BANK_ALLOCATION
#define BANK_ALLOCATION 0
#endif

#ifndef USE_CACHE
#define USE_CACHE 0
#endif

#if USE_CACHE == 1
#pragma HLS bus bank_number = 4 chunk_size = 32
#elif USE_AXI == 1
#pragma HLS bus bank_number = 4 chunk_size = 32 mode = m_axi
#elif BANK_NUMBER == 2
#pragma HLS bus bank_number = 2 chunk_size = 32 mode = m_axi
#elif BANK_NUMBER == 4
#pragma HLS bus bank_number = 4 chunk_size = 32
#elif BANK_NUMBER == 8
#pragma HLS bus bank_number = 8 chunk_size = 32
#endif
#if CACHE_64b_16d == 1
#pragma HLS cache bundle = bus line_count = 16 line_size = 2 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_64b_32d == 1
#pragma HLS cache bundle = bus line_count = 32 line_size = 2 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_64b_64d == 1
#pragma HLS cache bundle = bus line_count = 64 line_size = 2 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_128b_16d == 1
#pragma HLS cache bundle = bus line_count = 16 line_size = 4 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_128b_32d == 1
#pragma HLS cache bundle = bus line_count = 32 line_size = 4 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_128b_64d == 1
#pragma HLS cache bundle = bus line_count = 64 line_size = 4 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_256b_16d == 1
#pragma HLS cache bundle = bus line_count = 16 line_size = 8 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_256b_32d == 1
#pragma HLS cache bundle = bus line_count = 32 line_size = 8 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_256b_64d == 1
#pragma HLS cache bundle = bus line_count = 64 line_size = 8 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_512b_16d == 1
#pragma HLS cache bundle = bus line_count = 16 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_512b_32d == 1
#pragma HLS cache bundle = bus line_count = 32 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#elif CACHE_512b_64d == 1
#pragma HLS cache bundle = bus line_count = 64 line_size = 16 bus_size = 32 ways = 1 num_write_outstanding = \
    2 rep_policy = lru write_policy = wt
#endif
#if BANK_ALLOCATION == 0
#pragma HLS interface port = a mode = bus
#pragma HLS interface port = b mode = bus
#pragma HLS interface port = c mode = bus
#elif BANK_ALLOCATION == 1
#pragma HLS interface port = a mode = bus bank_allocation = 0, 1
#pragma HLS interface port = b mode = bus bank_allocation = 0, 1
#pragma HLS interface port = c mode = bus bank_allocation = 1
#elif BANK_ALLOCATION == 2
#pragma HLS interface port = a mode = bus bank_allocation = 0
#pragma HLS interface port = b mode = bus bank_allocation = 0
#pragma HLS interface port = c mode = bus bank_allocation = 0
#elif BANK_ALLOCATION == 3
#pragma HLS interface port = a mode = bus bank_allocation = 2, 3
#pragma HLS interface port = b mode = bus bank_allocation = 2, 3
#pragma HLS interface port = c mode = bus bank_allocation = 2, 3
#elif BANK_ALLOCATION == 4
#pragma HLS interface port = a mode = bus bank_allocation = 0, 1, 2, 3
#pragma HLS interface port = b mode = bus bank_allocation = 0
#pragma HLS interface port = c mode = bus bank_allocation = 0, 1, 2, 3
#elif BANK_ALLOCATION == 5
#pragma HLS interface port = a mode = bus bank_allocation = 2
#pragma HLS interface port = b mode = bus bank_allocation = 0
#pragma HLS interface port = c mode = bus bank_allocation = 0
#elif BANK_ALLOCATION == 6
#pragma HLS interface port = a mode = bus bank_allocation = 2
#pragma HLS interface port = b mode = bus bank_allocation = 1, 2
#pragma HLS interface port = c mode = bus bank_allocation = 0
#elif BANK_ALLOCATION == 7
#pragma HLS interface port = a mode = bus bank_allocation = 0
#pragma HLS interface port = b mode = bus bank_allocation = 2
#pragma HLS interface port = c mode = bus bank_allocation = 3
#endif
float __attribute__((noinline)) nested_red(float* const a, float* b, float* c, int n)
{
   float sum = 0;
   int i;
   for(int j = 0; j < MAX_ITERS; j = j + 1)
   {
#pragma omp parallel num_threads(NTHREADS)
      {
#pragma omp for private(i) reduction(+ : sum)
         for(i = 0; i < n; i++)
         {
            sum += a[i] + b[i] + c[i];
         }
      }
   }
   return sum;
}
