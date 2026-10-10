// Mandelbrot kernel
// Author: Andrew Canis
// Date: July 1, 2012

#include <stdio.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#define DECIMAL_PLACES 28
#define int2fixed(num) ((num) << DECIMAL_PLACES)
#define fixedmul(a, b) ((((long long)a) * ((long long)b)) >> DECIMAL_PLACES)
#define fixed2int(num) ((num) >> DECIMAL_PLACES)

#define WIDTH 128
#define HEIGHT 128
//#define MAX_ITER 10
//#define MAX_ITER 2
#define MAX_ITER 50

#pragma HLS bus bank_number = 16 chunk_size = 1024
#pragma HLS interface port = img mode = bus
__attribute__((noinline)) int mandelbrot(unsigned char* img)
{
   int i, j;
   int count = 0;

#pragma omp parallel num_threads(NUM_ACCELS)
   {
#pragma omp for private(i, j) reduction(+ : count)
      for(j = 0; j < HEIGHT; j++)
      {
         for(i = 0; i < WIDTH; i++)
         {
            int x_0 = -int2fixed(2) + ((((3 << 20) * i / WIDTH)) << 8);
            int y_0 = -int2fixed(1) + ((((2 << 20) * j / HEIGHT)) << 8);

            int x = 0;
            int y = 0;
            unsigned char fiter = 0;

            for(unsigned char iter = 0; iter < MAX_ITER; iter++)
            {
               long long abs_squared = fixedmul(x, x) + fixedmul(y, y);

               int xtmp = fixedmul(x, x) - fixedmul(y, y) + x_0;
               y = fixedmul(int2fixed(2), fixedmul(x, y)) + y_0;
               x = xtmp;

               fiter += abs_squared <= int2fixed(4);
            }

            // get black or white
            unsigned char colour = (fiter >= MAX_ITER) ? 0 : 1;
            // accumulate colour
            count += colour;
            // update image
            img[i + j * WIDTH] = colour;
         }
      }
   }
   return count;
}

int main()
{
   unsigned char img[WIDTH * HEIGHT];

#ifdef __BAMBU_SIM__
   m_param_alloc(0, sizeof(img));
#endif
   int count = mandelbrot(img);

   printf("%d\n", count);
   return count != 12013;
}