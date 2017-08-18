/* inverse.c */
#include "mypgm.h"

int make_inverse_image(unsigned char *image1, unsigned char *image2, unsigned int x_size, unsigned int y_size)
     /* inverse of image1 is output into image2 */
{
  int x, y;    /* control variable */
  
  for (y = 0; y < y_size; y++) {
    for (x = 0; x < x_size; x++) {
      *(image2 + y + x * MAX_IMAGESIZE) = (unsigned char)
        (MAX_BRIGHTNESS - *(image1 + y + x * MAX_IMAGESIZE));
    }
  }

  return 0;
}
