#ifndef __VECTOR_T_H
#define __VECTOR_T_H

#include <cstdint>

#define MAX_VALUES 10

typedef struct
{
   uint8_t values[MAX_VALUES];
   uint8_t *begin, *end;
} vector_t;

#endif
