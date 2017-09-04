#include <stdint.h>

#include "checksum.h"

uint8_t checksum(uint64_t in1, uint64_t in2)
{
	uint8_t sum = 0;
	for (int i = 0; i < sizeof(uint64_t); i++)
	{
		uint8_t a = (in1 >> (8*i)) & 0xFF;
		uint8_t b = (in2 >> (8*i)) & 0xFF;
		sum += a + b;
	}
	return sum;
}
