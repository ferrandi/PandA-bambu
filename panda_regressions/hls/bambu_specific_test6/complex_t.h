#ifndef COMPLEX_T_H
#define COMPLEX_T_H

typedef struct c_t
{
   unsigned nrow;
   unsigned nnz;
   float value[20];
   unsigned cindex[20];
   unsigned rowstart[20];
} complex_t;

#endif