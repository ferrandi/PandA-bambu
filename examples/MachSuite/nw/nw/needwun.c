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

#include "needwun.h"

void needwun(char SEQA[N], char SEQB[M], char allignedA[sum_size], char allignedB[sum_size], 
             int A[dyn_size], char ptr[dyn_size]){

    int score, match, mismatch, gap,
        choice1, choice2, choice3, max,
        i, j, i_t, j_t,
        Mul1, Mul2, Mul3;

    match       = 1;
    mismatch    = -1;
    gap         = -1;

    init_row : for(i = 0; i < N1; i++){
        A[i]   = i * mismatch;
    }

    init_col : for(i = 0; i <M1; i++){
        A[i * N1] = i * mismatch;
    }

    //matrix Filling Loop
    fill_out : for(i = 1; i < M1; i++){
        fill_in : for(j = 1; j < N1; j++){
            if(SEQA[j - 1] == SEQB[i - 1]){
                score = match;
            }
            else{
                score = mismatch;
            }

            Mul1 = (i-1) * N1;
            Mul2 = (i*N1);

            choice1 = A[Mul1 + (j - 1)] + score;
            choice2 = A[Mul1 + (j)]     + gap;
            choice3 = A[Mul2 + (j-1)]   + gap;

            if(choice1 > choice2){
                max = choice1;
            }
            else{
                max = choice2;
            }
            if(choice3 > max){
                max = choice3;
            }

            A[Mul2 + j] = max;
            if(max == choice1){
                ptr[Mul2 + j] = 0;
            }
            else if(max == choice2){
                ptr[Mul2 + j] = 1;
            }
            else{
                ptr[Mul2 + j] = -1;
            }
        }
    }

    //TraceBack
    i = M;
    j = N;
    i_t = 0;
    j_t = 0;

    trace : while(i > 0 || j > 0){
        Mul3 = j*M;
        if (ptr[i + Mul3] == 0){
            allignedA[i_t] = SEQA[i];
            allignedB[j_t] = SEQB[j];
            j_t++;
            i_t++;
            i--;
            j--;
        }
        else if(ptr[i + Mul3] == 1){
            allignedA[i_t] = SEQA[i];
            allignedB[j_t] = 'X';
            j_t++;
            i_t++;
            i--;
        }
        else{
            allignedA[i_t] = 'X';
            allignedB[j_t] = SEQB[j];
            j_t++;
            i_t++;
            j--;
        }
    }
}
