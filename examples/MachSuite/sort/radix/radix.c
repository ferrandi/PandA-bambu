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

#include "radix.h"

void local_scan(int bucket[BUCKETSIZE])
{
    int radixID, i, bucket_indx;
    local_1 : for (radixID = 0; radixID < SCAN_RADIX; radixID++){
        local_2 : for (i = 1; i < SCAN_BLOCK; i++){
            bucket_indx = radixID * SCAN_BLOCK + i;
            bucket[bucket_indx] += bucket[bucket_indx - 1];
        }
    }
}

void sum_scan(int sum[SCAN_RADIX], int bucket[BUCKETSIZE])
{
    int radixID, bucket_indx;
    sum[0] = 0;
    sum_1 : for (radixID = 1; radixID < SCAN_RADIX; radixID++){
        bucket_indx = radixID *SCAN_BLOCK - 1;        
        sum[radixID] = sum[radixID - 1] + bucket[bucket_indx];
    }
}

void last_step_scan(int bucket[BUCKETSIZE], int sum[SCAN_RADIX])
{
    int radixID, i, bucket_indx;
    last_1:for (radixID = 0; radixID < SCAN_RADIX; radixID++){
        last_2:for (i = 0; i < SCAN_BLOCK; i++){
            bucket_indx = radixID * SCAN_BLOCK + i;
            bucket[bucket_indx] = bucket[bucket_indx] + sum[radixID];
         }
    }
}

void init(int bucket[BUCKETSIZE])
{
    int i;
    init_1 : for (i = 0; i < BUCKETSIZE; i++){
        bucket[i] = 0;
    }
}

void hist(int bucket[BUCKETSIZE], int a[N], int exp)
{
    int blockID, i, bucket_indx, a_indx;
    blockID = 0;
    hist_1 : for (blockID = 0; blockID < NUMOFBLOCKS; blockID++){
        hist_2 : for(i = 0; i< 4; i++){
            a_indx = blockID * ELEMENTSPERBLOCK + i;
            bucket_indx = ((a[a_indx] >> exp)  & 0x3) * NUMOFBLOCKS + blockID + 1;
            bucket[bucket_indx]++;
        }
    }
}

void update(int b[N], int bucket[BUCKETSIZE], int a[N], int exp)
{
    int i, blockID, bucket_indx, a_indx;
    blockID = 0;
    
    update_1 : for (blockID = 0; blockID < NUMOFBLOCKS; blockID++){
        update_2 : for(i = 0; i < 4; i++){
            bucket_indx = ((a[blockID * ELEMENTSPERBLOCK + i] >> exp)  & 0x3) * NUMOFBLOCKS + blockID;
            a_indx = blockID * ELEMENTSPERBLOCK + i;
            b[bucket[bucket_indx]] = a[a_indx];
            bucket[bucket_indx]++;
        }
    }
}

void ss_sort(int a[N], int b[N], int bucket[BUCKETSIZE], int sum[SCAN_RADIX]){
    int exp, flag;
    exp = 0; flag = 0;

    sort_1 : for (exp = 0; exp < 32; exp+=2){
        init(bucket);
        if (flag == 0){
            hist(bucket, a, exp);
        }
        else{
            hist(bucket, b, exp);
        }

        local_scan(bucket);
        sum_scan(sum, bucket);
        last_step_scan(bucket, sum);

        if (flag == 0){
            update(b, bucket, a, exp);
            flag = 1;
        }
        else{
            update(a, bucket, b, exp);
            flag = 0;
        }
    }
}
