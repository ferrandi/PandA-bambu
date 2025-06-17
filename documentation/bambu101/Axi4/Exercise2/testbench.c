#include "sum.h"

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

int main()
{
   int A[20];
   unsigned n = 20U;

   for(int i = 0; i < 20; i++)
   {
      A[i] = i;
   }

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(A));
#endif
   return sum(A, &n);
}