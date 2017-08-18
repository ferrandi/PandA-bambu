// Mandelbrot kernel
// Author: Andrew Canis
// Date: July 1, 2012

#include <stdio.h>
#include <pthread.h>

inline void print_image(int width, int height, int max, unsigned char img[width][height])
{
   int i, j;
   printf("P2\n%d %d\n%d\n", width, height, max);
   for (j = 0; j < height; j++)
   {
      for (i = 0; i < width; i++)
      {
         // assume grayscale image
         printf("%d ", img[i][j]);
      }
      printf("\n");
   }
}

//#define PRINT_IMG 1

#define DECIMAL_PLACES 28
#define int2fixed(num) ((num) << DECIMAL_PLACES)
#define fixedmul(a, b) ((((long long)a) * ((long long)b)) >> DECIMAL_PLACES)
#define fixed2int(num) ((num) >> DECIMAL_PLACES)

#define HEIGHT 128
#define WIDTH 128
//#define MAX_ITER 10
//#define MAX_ITER 2
#define MAX_ITER 50
#define NUM_ACCEL 4
#define OPS_PER_ACCEL HEIGHT/NUM_ACCEL
#define OMP_ACCEL 4

#define LEGUP 1

struct thread_data
{
   int  startidx;
   int  maxidx;
};

volatile unsigned char img[WIDTH][HEIGHT];

void *mandelbrot(void *threadarg)
{
   int i, j, tid;
   int count = 0;
   struct thread_data* arg = (struct thread_data*) threadarg;
   int startidx = arg->startidx;
   int maxidx = arg->maxidx;

   for (j = startidx; j < maxidx; j++)
   {
      for (i = 0; i < WIDTH; i++)
      {

         int x_0 = int2fixed(-2) + ((((3 << 20) * i / WIDTH) ) << 8);
         int y_0 = int2fixed(-1) + ((((2 << 20) * j / HEIGHT) ) << 8);

         int x = 0;
         int y = 0;
         int xtmp;
         unsigned char iter;
         unsigned char fiter = 0;

         for (iter = 0; iter < MAX_ITER; iter++)
         {
            long long abs_squared = fixedmul(x, x) + fixedmul(y, y);

            xtmp = fixedmul(x, x) - fixedmul(y, y) + x_0;
            y = fixedmul(int2fixed(2), fixedmul(x, y)) + y_0;
            x = xtmp;

            fiter  +=  abs_squared <= int2fixed(4);
         }

         //get black or white
         unsigned char colour = (fiter >= MAX_ITER) ? 0 : 1;
         //accumulate colour
         count += colour;
         //update image
         img[i][j] = colour;
      }
   }

   pthread_exit((void*)count);
}


int main()
{
#ifdef LEGUP
   legup_start_counter(0);
#endif
   int final_result = 0;
   int i, j;
   int count[NUM_ACCEL];
   pthread_t threads[NUM_ACCEL];
   struct thread_data data[NUM_ACCEL];

   //initialize structs to pass into accels
   for (i = 0; i < NUM_ACCEL; i++)
   {
      data[i].startidx = i * OPS_PER_ACCEL;
      if (i == NUM_ACCEL - 1)
      {
         data[i].maxidx = HEIGHT;
      }
      else
      {
         data[i].maxidx = (i + 1) * OPS_PER_ACCEL;
      }
   }

   //launch threads
   //for (i=0; i<1; i++) {
   for (i = 0; i < NUM_ACCEL; i++)
   {
      pthread_create(&threads[i], NULL, mandelbrot, (void *)&data[i]);
   }

   //join the threads
   //for (i=0; i<1; i++) {
   for (i = 0; i < NUM_ACCEL; i++)
   {
      pthread_join(threads[i], (void**)&count[i]);
   }

   //sum the results
   for (i = 0; i < NUM_ACCEL; i++)
   {
      final_result += count[i];
   }
   //count = mandelbrot();
   /*
      //check results
      for (j = 0; j < HEIGHT; j++) {
         for (i = 0; i < WIDTH; i++) {
   //       printf("img[%d][%d] = %d\n", i, j, img[i][j]);
            final_result += img[i][j];
           }
       }*/
#ifdef PRINT_IMG
   unsigned char final[WIDTH][HEIGHT];
   for (j = 0; j < HEIGHT; j++)
   {
      for (i = 0; i < WIDTH; i++)
      {
         final[i][j] = img[i][j];
         /*       final[i+1][j] = img1[i+1][j];
                  final[i+2][j] = img2[i+2][j];
                  final[i+3][j] = img3[i+3][j];*/
      }
   }
   print_image(WIDTH, HEIGHT, 1, final);
   //print_image(WIDTH, HEIGHT, 1, img);
   //print_image(WIDTH, HEIGHT, MAX_ITER, img);

#else
#ifdef LEGUP
   int perf_counter = legup_stop_counter(0);
   printf("perf_counter = %d\n", perf_counter);
#endif
   printf("Count: %d\n", final_result);
   if (final_result == 12013)
   {
      printf("PASS\n");
   }
   else
   {
      printf("FAIL\n");
   }
#endif

}
