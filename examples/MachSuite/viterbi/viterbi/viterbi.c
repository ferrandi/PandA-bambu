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

#include "viterbi.h"

int viterbi(int Obs[numObs], float transMat[numStates*numObs], float obsLik[numStates*numObs], float v[numStates*numObs]){
    int i, j, k, finalState;
    float maxProb, temp;

    finalState = 0;

    v[0] = 1.0;

    v1 : for(i=0; i<numObs;i++){  //for each observation
       int baseObs =  Obs[i];
        v2 : for(j=0; j<numStates; j++){    //for each possible state
            v3 : for(k=0; k<numStates; k++){    //for each
                temp = v[j*numObs + i] * transMat[j*numObs + k] * obsLik[k*numObs + baseObs];
                if(temp > v[k*numObs + i+1]){
                    v[k*numObs + i+1] = temp;
                }
            }
        }
    }

    maxProb = (float)0.0;

    v4 : for(i=1;i<numStates+1;i++){
        if(v[i*numObs-1] > maxProb){
            finalState = i - 1;
            maxProb = v[i*numObs-1];
        }
    }

    return finalState;
}
