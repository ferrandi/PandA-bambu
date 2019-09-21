/************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software 
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.// Copyright (c) 2016 Xilinx, Inc.
************************************************/

#include <stdio.h>
#include "../src/diff_sq_acc.h"



void ref_diff_sq_acc(int a[N], int b[N], long long int *dout)
{
	int i;
	long long int acc  = 0;
    int diff;
    long long int diff2;

	for(i=0; i<N; i++)
	{
		diff = a[i]-b[i];
		diff2 = (long long int) diff * (long long int) diff;
		acc += diff2;
	}

	*dout = acc;

}

int main(void)
{
	int i, k, cnt;
	int ret_val = 0;

	int    a[N],  b[N];
	din_t aa[N], bb[N];

	long long int ref_res;
	dout_t    res;

	cnt = 0;
	for (k=0; k<10; k++)
	{
		//create random data
		for(i=0; i<N; i++)
		{
			a[i] =  rand() % (1024*16);
			b[i] =  rand() % (1024*16);
			aa[i] =  (din_t) a[i];
			bb[i] =  (din_t) b[i];
		}

		//call reference function
		ref_diff_sq_acc(a, b, &ref_res);

		//call design Under Test
		diff_sq_acc( aa, bb, &res);

		//check results
		printf("got %lld expected %lld\n", (long long int) res.to_double(), (long long int) ref_res);
		if ( (ref_res - (long long int) res) !=0 ) cnt++;
	}

	if (cnt>0)
	{
		printf("TEST FAILED: %d errors\n", cnt);
		ret_val = 1;
	}
	else
	{
		printf("TEST SUCCESS!\n");
		ret_val = 0;
	}


	return ret_val;

}

