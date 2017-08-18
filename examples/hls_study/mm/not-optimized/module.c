#include "module.h"
// matrix multiplication of a A*B matrix
void 
__attribute__ ((noinline))  
mm (int in_a[A_ROWS][A_COLS], int in_b[A_COLS][B_COLS], int out_c[A_ROWS][B_COLS])
//void mm(int(* __restrict__ in_a)[A_COLS], int(* __restrict__ in_b)[B_COLS], int(* __restrict__ out_c)[B_COLS])
{
    int i,j,k;
    for (i = 0; i < A_ROWS; i++)
    {
        for (j = 0; j < B_COLS; j++)
        {
            int sum_mult = 0;
            for (k = 0; k < A_COLS; k++)
            {
                sum_mult += in_a[i][k] * in_b[k][j];
            }
            out_c[i][j] = sum_mult;
        }
    }
}
