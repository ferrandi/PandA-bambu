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
"Stencil computation optimization and auto-tuning on state-of-the-art multicore architectures"
K. Datta, M. Murphy, V. Volkov, S. Williams, J. Carter, L. Oliker, D. Patterson, J. Shalf, K. Yelick
SC 2008
*/

#include "stencil3d.h"

void stencil3d(TYPE C0, TYPE C1, TYPE orig[size], TYPE sol[size]) {
    int i, j, k;
    TYPE sum1, sum2, mul1, mul2, diff;

    loop_height : for(i = 1; i < height_size - 1; i++){
        loop_col : for(j = 1; j < col_size - 1; j++){
            loop_row : for(k = 1; k < row_size - 1; k++){
                sum1 = orig[indx(row_size, col_size, i, j, k + 1)] +
                       orig[indx (row_size, col_size, i, j, k - 1)] +
                       orig[indx (row_size, col_size, i, j + 1, k)] +
                       orig[indx (row_size, col_size, i, j - 1, k)] +
                       orig[indx (row_size, col_size, i + 1, j, k)] +
                       orig[indx (row_size, col_size, i - 1, j, k)];

                sum2 = orig[indx (row_size, col_size, i, j, k)];

                mul2 = sum2 * C0;
                mul1 = sum1 * C1;
                diff = mul1 - mul2;

                sol[indx(row_size, col_size, i, j, k)] = diff;
            }
        }
    }
}
