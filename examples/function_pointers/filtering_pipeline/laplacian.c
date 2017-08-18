/* laplacian.c */
#include "mypgm.h"
#include <float.h>

int laplacian(unsigned char *image1, unsigned char *image2, unsigned int x_size, unsigned int y_size)
     /* Spatial filtering of image data */
     /* 8-neighbor Laplacian filter */
     /* Input: image1[y][x] ---- Outout: image2[y][x] */
{
  /* Definition of 8-neighbor Laplacian filter */
  int weight[3][3] = {{ 1,  1,  1 },
		      { 1,  -8,  1 },
		      { 1,  1,  1 }};
  double pixel_value;
  double min, max;
  int x, y, i, j;  /* Loop variable */
  
  /* Maximum values calculation after filtering*/
  min = DBL_MAX;
  max = -DBL_MAX;
  for (y = 1; y < y_size - 1; y++) {
    for (x = 1; x < x_size - 1; x++) {
      pixel_value = 0.0;
      for (j = - 1; j < 2; j++) {
	    for (i = -1; i < 2; i++) {
	      pixel_value += weight[j + 1][i + 1] * (*(image1 + (y + j) + (x + i) * MAX_IMAGESIZE));
	    }
      }
      if (pixel_value < min) min = pixel_value;
      if (pixel_value > max) max = pixel_value;
    }
  }
  if ((int)(max - min) == 0) {
    return -1;
  }

  /* Initialization of image2[y][x] */
  for (y = 0; y < y_size; y++) {
    for (x = 0; x < x_size; x++) {
      *(image2 + y + x * MAX_IMAGESIZE) = 0;
    }
  }
  
  /* Generation of image2 after linear transformtion */
  for (y = 1; y < y_size - 1; y++) {
    for (x = 1; x < x_size - 1; x++) {
      pixel_value = 0.0;
      for (j = - 1; j < 2; j++) {
	    for (i = -1; i < 2; i++) {
	      pixel_value += weight[j + 1][i + 1] * (*(image1 + (y + j) + (x + i) * MAX_IMAGESIZE));
	    }
      }
      pixel_value = MAX_BRIGHTNESS * (pixel_value - min) / (max - min);
      *(image2 + y + x * MAX_IMAGESIZE) = (unsigned char)pixel_value;
    }
  }
  return 0;
}
