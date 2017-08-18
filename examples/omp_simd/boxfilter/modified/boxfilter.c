#define DIMENSION_X 1000
#define DIMENSION_Y 8

#include <stdio.h>
#include "boxfilter.h"


// The values of the 3x3 box to filter with.
/*
 *  1 2 3
 *  4 5 6
 *  7 8 9
 */
//Constant to filter by
#define BOX1 3
#define BOX2 3
#define BOX3 3
#define BOX4 3
#define BOX5 3
#define BOX6 3
#define BOX7 3
#define BOX8 3
#define BOX9 3

/// a 3x3 box filter. Filter an entire row of the matrix in one call.
void filter (int y, int input[][DIMENSION_X] , int * output)
{
   int x;
   int checksum = 0;
   for (x = 0; x < DIMENSION_X; x++)
   {
      int sum = 0;
      //ex if at the top left corner of the image, we can only average
      //the (y,x),(y+1,x),(y+1,x+1),(y,x+1). therefore pixelsAveraged = 4
      // if we're not at an edge of the image, then pixels averaged is 3x3=9
      int pixelsAveraged = 1;
      // Sum them up
      if (y - 1 >= 0 && x - 1 >= 0)
      {
         sum += input[y - 1][x - 1] * BOX1; //top left
         pixelsAveraged++;
      }
      if (y - 1 >= 0 && x + 1 < DIMENSION_X)
      {
         sum += input[y - 1][x + 1] * BOX3; //top right
         pixelsAveraged++;
      }
      if (y + 1 < DIMENSION_Y && x + 1 < DIMENSION_X)
      {
         sum += input[y + 1][x + 1] * BOX9; //bottom right
         pixelsAveraged++;
      }
      if (y + 1 < DIMENSION_Y && x - 1 >= 0)
      {
         sum += input[y + 1][x - 1] * BOX7; //bottom left
         pixelsAveraged++;
      }
      if (y - 1 >= 0)
      {
         sum += input[y - 1][x] * BOX2; //top
         pixelsAveraged++;
      }
      if (x - 1 >= 0)
      {
         sum += input[y][x - 1] * BOX4; //left
         pixelsAveraged++;
      }
      if (y + 1 < DIMENSION_Y)
      {
         sum += input[y + 1][x] * BOX8; //bottom
         pixelsAveraged++;
      }
      if (x + 1 < DIMENSION_X)
      {
         sum += input[y][x + 1] * BOX6; //right
         pixelsAveraged++;
      }
      sum += input[y][x]; //centre

      checksum += sum / pixelsAveraged;
   }
   output[y] = checksum;
}

int main()
{

   int y, x;
   int result = 0;

   #pragma omp simd
   for (y = 0; y < DIMENSION_Y; y++)
   {
      filter(y, original, checksum_output);
   }

   // check the result
   for (y = 0; y < DIMENSION_Y; y++)
   {
      result += checksum_output[y] == checksum_expected[y];
   }

   printf ("Result: %d\n", result);
   if (result == DIMENSION_Y)
   {
      printf("RESULT: PASS\n");
      return 0;
   }
   else
   {
      printf("RESULT: FAIL\n");
      return 1;
   }
}
