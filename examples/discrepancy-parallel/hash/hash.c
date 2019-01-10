#include <stdio.h>

#define SIZE 500
#include "hash.h"

//#define PRINT_RESULT
//#define CHECK_COLLISION
//#define CHECK_RESULT

int result1[SIZE];
int result2[SIZE];
int result3[SIZE];
int result4[SIZE];

#define BINSIZE 500

int hash1()
{
	int result[BINSIZE] = {0};
	int i, a, collision=0;
	for (i=0; i<SIZE; i++) {
		a = input[i];
		a = (a+0x7ed55d16) + (a<<12);
		a = (a^0xc761c23c) ^ (a>>19);
		a = (a+0x165667b1) + (a<<5);
		a = (a+0xd3a2646c) ^ (a<<9);
		a = (a+0xfd7046c5) + (a<<3);
		a = (a^0xb55a4f09) ^ (a>>16);
		a = (a<0) ? -1*a : a;		
		int hash = a%BINSIZE;
		int temp = result[hash]++;
        collision = (temp!=0) ? collision+1 : collision;
	}
    return collision;
}

int hash2()
{
	int result[BINSIZE] = {0};
	int i, a, collision=0;
	for (i=0; i<SIZE; i++) {
		a = input[i];
		a -= (a<<6);
		a ^= (a>>17);
		a -= (a<<9);
		a ^= (a<<4);
		a -= (a<<3);
		a ^= (a<<10);
		a ^= (a>>15);
		a = (a<0) ? -1*a : a;		
		int hash = a%BINSIZE;
		int temp = result[hash]++;
        collision = (temp!=0) ? collision+1 : collision;
	}
    return collision;
}

int hash3() {
	int result[BINSIZE] = {0};
	int i, a, collision=0;
	for (i=0; i<SIZE; i++) {
		a = input[i];
	    a = (a ^ 61) ^ (a >> 16);
		a = a + (a << 3);
	    a = a ^ (a >> 4);
	    a = a * 0x27d4eb2d;
	    a = a ^ (a >> 15);
		a = (a<0) ? -1*a : a;		
		int hash = a%BINSIZE;
		int temp = result[hash]++;
        collision = (temp!=0) ? collision+1 : collision;
	}
    return collision;
}

int hash4()
{
	int result[BINSIZE] = {0};
	int i, a, collision=0;
	for (i=0; i<SIZE; i++) {
		a = input[i];
	    a = (a+0x479ab41d) + (a<<8);
	    a = (a^0xe4aa10ce) ^ (a>>5);
	    a = (a+0x9942f0a6) - (a<<14);
	    a = (a^0x5aedd67d) ^ (a>>3);
	    a = (a+0x17bea992) + (a<<7);
		a = (a<0) ? -1*a : a;		
		int hash = a%BINSIZE;
		int temp = result[hash]++;
        collision = (temp!=0) ? collision+1 : collision;
	}
    return collision;
}

#define CHECK_RESULT
int main(int argc, char ** argv) {
	int i;
	int collisions[4] = {0};
	int (*hash[4])() = {hash1, hash2, hash3, hash4};
#pragma omp parallel for
	for (i = 0; i < 4; i++)
		collisions[i] = hash[i]();
	int count=0;
    	count += (collisions[0] == 178);
    	count += (collisions[1] == 182);
    	count += (collisions[2] == 196);
    	count += (collisions[3] == 179);
	printf("result = %d\n", count);
	if (count == 4) {
		printf("PASS\n");
	}
	else {
		printf("FAIL\n");
	}
	return 0;
}
