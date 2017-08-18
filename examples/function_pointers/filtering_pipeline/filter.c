#include "filter.h"
#include "mypgm.h"


#define MAX_PIPELINE_DEPTH 10
void * pipeline[MAX_PIPELINE_DEPTH];


int add_filter(unsigned int pos, int (*filter)(unsigned char *, unsigned char *, unsigned int, unsigned int))
{
  pipeline[pos] = filter;
  return 0;
}


void execute(unsigned char *in, unsigned char *out, unsigned int x_size, unsigned int y_size)
{
  int i = 0;
  int (*fun)(unsigned char *, unsigned char *, unsigned int, unsigned int);
  for (i = 0; i < MAX_PIPELINE_DEPTH; i++) {
    if (pipeline[i] == 0) break;

    fun = (int (*)(unsigned char *, unsigned char *, unsigned int, unsigned int)) pipeline[i];
    int res = fun(in, out, x_size, y_size);
    if (res != 0) return;
    
    unsigned char * tmp = in;
    in = out;
    out = tmp;
  }

  // copy the content of in on out when the number of applied filter
  // is odd.
  if (!(i % 2))
  {
    int x, y;
    for (y = 0; y < y_size; y++) {
      for (x = 0; x < x_size; x++) {
        *(out + y + x * MAX_IMAGESIZE) = *(in + y + x * MAX_IMAGESIZE);
      }
    }
  }
}

