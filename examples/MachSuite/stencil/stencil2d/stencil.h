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
*/

#include <stdio.h>
#include <stdlib.h>

//Define input sizes
#define col_size 64
#define row_size 128
#define f_size 9

//Data Bounds
#define MIN 2147483646
#define MAX -2147483646
#define TYPE int

//Set number of iterations to execute
#define MAX_ITERATION 1

void stencil( TYPE orig[row_size * col_size], 
        TYPE sol[row_size * col_size], 
        TYPE filter[f_size] );

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
    TYPE orig[row_size*col_size];
    TYPE sol[row_size*col_size];
    TYPE filter[f_size];
};
int INPUT_SIZE = sizeof(struct bench_args_t);


__attribute((noinline)) void run_benchmark( void *vargs ) {
    struct bench_args_t *args = (struct bench_args_t *)vargs;
    stencil( args->orig, args->sol, args->filter );
}

////////////////////////////////////////////////////////////////////////////////
