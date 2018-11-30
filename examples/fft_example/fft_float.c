#include <math.h>
/*
      This computes an in-place complex-to-complex FFT 
       x and y are the real and imaginary arrays of 2^m points.
      dir =  1 gives forward transform
      dir = -1 gives reverse transform 
      Code from http://paulbourke.net/miscellaneous/dft/
      by Paul Bourke June 1993
*/
short FFT(short int dir, long m, float *x, float *y) {
      long n, i, i1, j, k, i2, l, l1, l2;
      float c1, c2, tx, ty, t1, t2, u1, u2, z;

      /* Calculate the number of points */
      n = 1;
      for (i = 0; i < m; i++) n *= 2;

      /* Do the bit reversal */
      i2 = n >> 1;
      j = 0;
      for (i = 0; i < n - 1; i++) {
            if (i < j) {
                  tx = x[i];
                  ty = y[i];
                  x[i] = x[j];
                  y[i] = y[j];
                  x[j] = tx;
                  y[j] = ty;
            }
            k = i2;
            while (k <= j) {
                  j -= k;
                  k >>= 1;
            }
            j += k;
      }

      /* Compute the FFT */
      c1 = -1.0;
      c2 = 0.0;
      l2 = 1;
      for (l = 0; l < m; l++) {
            l1 = l2;
            l2 <<= 1;
            u1 = 1.0;
            u2 = 0.0;
            for (j = 0; j < l1; j++) {
                  for (i = j; i < n; i += l2) {
                        i1 = i + l1;
                        t1 = u1 * x[i1] - u2 * y[i1];
                        t2 = u1 * y[i1] + u2 * x[i1];
                        x[i1] = x[i] - t1;
                        y[i1] = y[i] - t2;
                        x[i] += t1;
                        y[i] += t2;
                  }
                  z =  u1 * c1 - u2 * c2;
                  u2 = u1 * c2 + u2 * c1;
                  u1 = z;
            }
            c2 = sqrtf((1.0 - c1) / 2.0);
            if (dir == 1) c2 = -c2;
            c1 = sqrtf((1.0 + c1) / 2.0);
      }

      /* Scaling for forward transform */
      if (dir == 1) {
            for (i = 0; i < n; i++) {
                  x[i] /= n;
                  y[i] /= n;
            }
      }

      return 0;
}


/*
 * Here is an example program which computes the FFT of a short pulse in a sample of length 128. 
 * To make the resulting fourier transform real the pulse is defined for equal positive and 
 * negative times (-10 ... 10), where the negative times wrap around the end of the array.
 * The transformed data is rescaled by 1/\sqrt N so that it fits on the same plot as the input. 
 * Only the real part is shown, by the choice of the input data the imaginary part is zero. 
 * Allowing for the wrap-around of negative times at t=128, and working in units of k/N, 
 * the DFT approximates the continuum fourier transform, giving a modulated \sin function.
*/

int
main (void)
{
  int i;
  float x[128];
  float y[128];

  for (i = 0; i < 128; i++)
    {
       x[i] = 0.0;
       y[i] = 0.0;
    }

  x[0] = 1.0;

  for (i = 1; i <= 10; i++)
    {
       x[i] = x[128-i] = 1.0;
    }

  for (i = 0; i < 128; i++)
    {
      printf ("%d %e %e\n", i, 
              x[i], y[i]);
    }
  printf ("\n");

  i = FFT(1, 7, x, y);
  for (i = 0; i < 128; i++)
    {
      printf ("%d %e %e\n", i, 
              x[i]/sqrtf(128), 
              y[i]/sqrtf(128));
    }

  return 0;
}

