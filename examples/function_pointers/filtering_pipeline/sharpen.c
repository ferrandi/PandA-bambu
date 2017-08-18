/* sharpen.c */
#include "mypgm.h"

int sharpen(unsigned char *image1, unsigned char *image2, unsigned int x_size, unsigned int y_size)
     /* Spatial filtering of image data */
     /* Sharpening filter by 8-neighbor Laplacian subtraction */
     /* Input: image1[y][x] ---- Outout: image2[y][x] */
{
  /* Definition of sharpening filter */
  int weight[3][3] = {{ 1,  1,  1 },
		      { 1,  -8,  1 },
		      { 1,  1,  1 }};
  const double alpha = 0.2;
  double pixel_value;
  int x, y, i, j;  /* Loop variable */
  int new_value;
  
  /* Initialization of image2[y][x] */
  for (y = 0; y < y_size; y++) {
    for (x = 0; x < x_size; x++) {
      *(image2 + y + x * MAX_IMAGESIZE) = *(image1 + y + x * MAX_IMAGESIZE);
    }
  }
  
  /* Original image minus Laplacian image */
  for (y = 1; y < y_size - 1; y++) {
    for (x = 1; x < x_size - 1; x++) {
      pixel_value = 0.0;
      for (j = - 1; j < 2; j++) {
	    for (i = -1; i < 2; i++) {
	      pixel_value += weight[j + 1][i + 1] * *(image1 + (y + j) + (x + i) * MAX_IMAGESIZE);
	    }
      }
      new_value = (int)(*(image1 + y + x * MAX_IMAGESIZE) - alpha * pixel_value);
      if (new_value < 0) new_value = 0;
      if (new_value > MAX_BRIGHTNESS) new_value = MAX_BRIGHTNESS;
      *(image2 + y + x * MAX_IMAGESIZE) = (unsigned char)new_value;
    }
  }

  return 0;
}
