#define A_ROWS 16
#define A_COLS 16
#define B_ROWS 16
#define B_COLS 16
// matrix multiplication of a A*B matrix
void mm (int in_a[A_ROWS][A_COLS], int in_b[A_COLS][B_COLS], int out_c[A_ROWS][B_COLS])
{
    int i,j,k;
#ifdef UNROLL_LOOPS1
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
#endif
    for (i = 0; i < A_ROWS; i++)
    {
#ifdef UNROLL_LOOPS2
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
#endif
        for (j = 0; j < B_COLS; j++)
        {
            int sum_mult = 0;
#ifdef UNROLL_LOOPS3
#if defined(__clang__)
#pragma clang loop unroll(full)
#endif
#endif
            for (k = 0; k < A_COLS; k++)
            {
                sum_mult += in_a[i][k] * in_b[k][j];
            }
            out_c[i][j] = sum_mult;
        }
    }
}
