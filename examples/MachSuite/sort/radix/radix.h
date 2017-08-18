/*
Copyright (c) 2014, the President and Fellows of Harvard College.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of Harvard University nor the names of its contributors may 
  be used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

implementation based on algorithm described in:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter. 
The scalable heterogeneous computing (shoc) benchmark suite. 
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE int

#define N 2048
#define NUMOFBLOCKS 512

#define ELEMENTSPERBLOCK 4
#define RADIXSIZE 4
#define BUCKETSIZE NUMOFBLOCKS*RADIXSIZE
#define MASK 0x3

#define SCAN_BLOCK 16
#define SCAN_RADIX BUCKETSIZE/SCAN_BLOCK

void local_scan(int bucket[BUCKETSIZE]);
void sum_scan(int sum[SCAN_RADIX], int bucket[BUCKETSIZE]);
void last_step_scan(int bucket[BUCKETSIZE], int sum[SCAN_RADIX]);
void init(int bucket[BUCKETSIZE]);
void hist(int bucket[BUCKETSIZE], int a[N], int exp);
void update(int b[N], int bucket[BUCKETSIZE], int a[N], int exp);
void ss_sort(int a[N], int b[N], int bucket[BUCKETSIZE], int sum[SCAN_RADIX]);

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  int a[N];
  int b[N];
  int bucket[BUCKETSIZE];
  int sum[SCAN_RADIX];
};
int INPUT_SIZE = sizeof(struct bench_args_t);


void run_benchmark( void *vargs ) {
  struct bench_args_t *args = (struct bench_args_t *)vargs;
  ss_sort( args->a, args->b, args->bucket, args->sum );
}

////////////////////////////////////////////////////////////////////////////////
