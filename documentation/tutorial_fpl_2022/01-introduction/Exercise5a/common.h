#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef float real_t;

real_t* make_empty(size_t size)
{
    return (real_t*)calloc(size, sizeof(real_t));
}

real_t* make_random(size_t size)
{
    real_t* result = make_empty(size);
    if (!result) return NULL;

    real_t* end = result + size;
    for (real_t* ptr = result; ptr != end; ++ptr) {
        *ptr = ((real_t)random() / RAND_MAX) * (real_t)(2) - (real_t)(1);
    }

    return result;
}

real_t* make_copy(const real_t* data, size_t size)
{
    real_t* result = make_empty(size);
    if (!result) return NULL;

    memcpy(result, data, size*sizeof(real_t));
    return result;
}

real_t mse(const real_t* a, const real_t* b, size_t size)
{
    real_t accu = 0;
    const real_t* a_end = a + size;
    for (; a != a_end; ++a,++b) {
        real_t err = (*a - *b);
        accu += err * err;
    }
    return accu / (real_t)(size);
}
