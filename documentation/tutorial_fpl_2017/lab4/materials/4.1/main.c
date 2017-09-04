#include <stdbool.h>
#include <stdint.h>

#include "checksum.h"

bool have_same_checksum(uint64_t original1, uint64_t original2, uint64_t reference1, uint64_t reference2) {
	uint8_t sum_or = checksum(original1, original2);
	uint8_t sum_ref = checksum(reference1, reference2);
	return sum_or != sum_ref;
}

int test(uint64_t original1, uint64_t original2, uint64_t reference1, uint64_t reference2, bool must_be_equal)
{
	return !(have_same_checksum(1ull,2ull,1ull,2ull) == must_be_equal);
}
