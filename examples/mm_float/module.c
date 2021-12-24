void mm(float * __restrict__ in_a, float * __restrict__ in_b, float * __restrict__ out_c, unsigned int A_ROWS, unsigned int A_COLS, unsigned int B_COLS)
{
    int i,j,k;
    for (i = 0; i < A_ROWS; i++)
    {
        for (j = 0; j < B_COLS; j++)
        {
            float sum = 0;
            #pragma unroll 4
            for (k = 0; k < A_COLS; k++)
            {
                float a = in_a[i * A_COLS + k];
                float b = in_b[k * B_COLS + j];
                sum += a * b;
            }
            out_c[i * B_COLS + j] = sum;
        }
    }
}
