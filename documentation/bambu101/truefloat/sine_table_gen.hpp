#pragma once

#include <cstddef>

typedef double fptype_t;

extern "C" void sine_table_gen(fptype_t points[], fptype_t values[], size_t count, fptype_t frequency, fptype_t amplitude, fptype_t rate);
