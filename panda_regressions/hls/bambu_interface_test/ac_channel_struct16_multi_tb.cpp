#include <ac_channel.h>
#include <cstdio>
#ifdef  __BAMBU__
#include <mdpi/mdpi_user.h>
#endif

struct Data16
{
   unsigned long long val[16];
};

extern "C" void top(Data16* inData1, Data16* inData2, ac_channel<Data16>& out1, ac_channel<Data16>& out2, int n);

int main()
{
   ac_channel<Data16> outCh1, outCh2;
   Data16 inputData1[2], inputData2[2], resultData1[2], resultData2[2];

   // Initialise input array with values 1..16
   for(int j = 0; j < 2; j++)
      for(int i = 0; i < 16; i++)
         inputData1[j].val[i] = (unsigned long long)(i + 1);
   for(int j = 0; j < 2; j++)
      for(int i = 0; i < 16; i++)
         inputData2[j].val[i] = (unsigned long long)(i + 1);

#ifdef  __BAMBU__
   m_param_alloc(0, sizeof(inputData1));
   m_param_alloc(1, sizeof(inputData2));
#endif


   // Call the kernel under test
   top(inputData1, inputData2, outCh1, outCh2, 2);

   for(int j = 0; j < 2; j++)
      resultData1[j] = outCh1.read();
   for(int j = 0; j < 2; j++)
      resultData2[j] = outCh2.read();

   // Verify pairwise reduction: val[i] = inputData[i] + inputData[i+1] for even i
   int pass = 1;
   for(int j = 0; j < 2; j++)
      for(int i = 0; i < 16; i++)
      {
         if(i % 2 == 0)
         {
            unsigned long long expected = (unsigned long long)(i + 1) + (unsigned long long)(i + 2);
            if(resultData1[j].val[i] != expected)
            {
               printf("FAIL at [%d]: got %llu, expected %llu\n", i, resultData1[j].val[i], expected);
               pass = 0;
            }
            if(resultData2[j].val[i] != expected)
            {
               printf("FAIL at [%d]: got %llu, expected %llu\n", i, resultData2[j].val[i], expected);
               pass = 0;
            }
         }
         else
         {
            if(resultData1[j].val[i] != 0)
            {
               printf("FAIL at [%d]: got %llu, expected 0\n", i, resultData1[j].val[i]);
               pass = 0;
            }
            if(resultData2[j].val[i] != 0)
            {
               printf("FAIL at [%d]: got %llu, expected 0\n", i, resultData2[j].val[i]);
               pass = 0;
            }
         }
      }

   if(pass)
      printf("PASS\n");

   return pass ? 0 : 1;
}
