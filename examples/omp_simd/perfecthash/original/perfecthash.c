#define ARRAY_SIZE 24000
#define NUM_ACCELS 6
#define OPS_PER_ACCEL ARRAY_SIZE/NUM_ACCELS

typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned short int  ub2;
typedef  unsigned       char ub1;

#include <stdio.h>
#include "perfecthash.h"


ub4 phash(ub4 val, ub2 *scramble, ub1 *tab)
{
	ub4 a, b, rsl;

	b = (val >> 5) & 0x1fff;
	a = ((val << 22) >> 19);
	rsl = (a^scramble[tab[b]]);
	return rsl;
}

int perfect_hash (int *key_input, int *table, ub2 *scramble, ub1 *tab, int max_idx) {
	int i, hash;
	int result=0;

	for (i=0; i<max_idx; i++) {
		hash=phash(key_input[i], scramble, tab);
		result +=(table[hash] == key_input[i]);
	}

	return result;
}

int result[NUM_ACCELS];

int main() {
	
	int i, main_result = 0;

	#pragma omp parallel for num_threads(NUM_ACCELS) private(i)
	for (i=0; i<NUM_ACCELS; i++) {
		result[i] = perfect_hash(key+(OPS_PER_ACCEL*i), hash_table, scramble, tab, OPS_PER_ACCEL);
	}

	//combine results
	for (i=0; i<NUM_ACCELS; i++) {
		main_result += result[i];
	}

	//check final result
	printf ("Result: %d\n", main_result);
	if (main_result == 24000) {
		printf("RESULT: PASS\n");
	} else {
		printf("RESULT: FAIL\n");
	}
}
