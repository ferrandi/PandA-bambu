#include "sine_table_gen.hpp"

#include <cmath>

extern "C" void sine_table_gen(fptype_t points[], fptype_t values[], size_t count, fptype_t frequency, fptype_t amplitude, fptype_t rate)
{
  size_t i;
  for(i = 0; i < count; ++i)
  {
    fptype_t t = (fptype_t)i / rate;
    points[i] = t;
    values[i] = amplitude * (fptype_t)sin(2.0 * M_PI * frequency * t);
  }
}
