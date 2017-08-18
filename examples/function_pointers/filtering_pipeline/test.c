#include "filter.h"

int laplacian(unsigned char * in, unsigned char *out, unsigned int x_size, unsigned int y_size);
int sobel(unsigned char * in, unsigned char *out, unsigned int x_size, unsigned int y_size);
int sharpen(unsigned char * in, unsigned char *out, unsigned int x_size, unsigned int y_size);
int make_inverse_image(unsigned char * in, unsigned char *out, unsigned int x_size, unsigned int y_size);

void test(unsigned int *in, unsigned int *out, unsigned int x_size, unsigned int y_size, int type)
{
  // Initialize the filtering pipeline
  if (type == 0)
    add_filter(0, laplacian);
  else if (type == 1)
    add_filter(0, sharpen);
  else if (type == 2)
  {
    add_filter(0, make_inverse_image);
    add_filter(1, sharpen);
  }

  execute((unsigned char *)in, (unsigned char *)out, x_size, y_size);
}
