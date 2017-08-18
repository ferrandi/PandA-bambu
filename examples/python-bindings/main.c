#include <stdio.h>

// for sum()
#include "sum.h"

int main(long a, long b)
{
	long result = 0;
	result = sum(a, b);
	printf("%ld + %ld = %ld\n", a, b, result);
	return !(a + b == result);
}
