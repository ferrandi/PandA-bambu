#include "mmult.h"
#define rank 32

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   int A[rank * rank];
   int B[rank * rank];
   int C[rank * rank];

   for(int i = 0; i < rank*rank; i++)
   {
      A[i] = 2*i;
      B[i] = -i;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(A));
   m_param_alloc(1, sizeof(B));
   m_param_alloc(2, sizeof(C));
#endif
   mmult(A, B, C);

   return 0;
}