#include <stdint.h>
#include <math.h>

static inline void fun1(const float (*arr1)[2][2], const float (*arr2)[2][2], float (*arr3)[2][2]) 
{
     for (int i = 0; i < 2; i += 1) 
     {
         for (int j = 0; j < 2; j += 1) 
         {
             for (int k = 0; k < 2; k += 1)
                     arr3[i][j][k] = arr1[i][j][k] + arr2[i][j][k];
         }
     }
}

void fun2(const float (*arr1)[2][2], const float (*arr2)[2][2], float (*arr3)[2][2]) 
{
     fun1(arr1, arr2, arr3);
}

