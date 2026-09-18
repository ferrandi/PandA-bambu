#include <stdint.h>

struct aggregate
{
   uint64_t first;
   uint64_t second;
};

uint64_t top1(aggregate input, uint64_t scale)
{
   return (input.first + input.second) * scale;
}

aggregate top2(uint64_t first, uint64_t second)
{
   aggregate output;
   output.first = first;
   output.second = second;
   return output;
}
