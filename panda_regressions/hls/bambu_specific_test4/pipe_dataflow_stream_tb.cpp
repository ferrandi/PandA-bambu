#include <cstdio>
#include <hls_stream.h>
#ifdef __BAMBU__
#include <mdpi/mdpi_user.h>
#endif

#define BATCH_ITEM_NUMBER 16
#define TOTAL_BATCHES 8

extern "C" void top(hls::stream<unsigned>& dma_in, hls::stream<unsigned>& dma_out);

int main()
{
   hls::stream<unsigned> inputData, outputData;

   // Initialise input array with values 1..16
   for(int j = 0; j < TOTAL_BATCHES; j++)
      for(int i = 0; i < 16; i++)
         inputData.write((unsigned long long)(i + 1));

   // Call the kernel under test
   top(inputData, outputData);

   int pass = 1;
   for(int j = 0; j < TOTAL_BATCHES; j++)
   {
      for(int i = 0; i < BATCH_ITEM_NUMBER; i++)
      {
         unsigned expected = (unsigned long long)(i + 1) * (unsigned long long)(i + 1);
         unsigned computed = outputData.read();
         if(computed != expected)
         {
            printf("FAIL at [%d]: got %u, expected %u\n", i, computed, expected);
            pass = 0;
         }
      }
   }

   if(pass)
      printf("PASS\n");

   return pass ? 0 : 1;
}
