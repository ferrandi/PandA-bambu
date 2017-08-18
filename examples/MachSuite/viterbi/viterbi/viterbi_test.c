#include "viterbi.h"

float RR(){
    float x = (float)((float)rand()/(float)RAND_MAX);
    return x;
}

int main() {
    int i, j, k;
    int Obs[numObs];
    float transMat[numStates*numObs], obsLik[numStates*numObs];
    int finalState;
    finalState = 2;

    srandom(1);
    for(i=0;i<numObs;i++){
        Obs[i] = i;
    }

    for(j=0;j<numStates;j++){
        for(i=0;i<numObs;i++){
            transMat[j*numObs + i] = RR();
            obsLik[j*numObs + i] = RR();
        }
    }

    finalState = viterbi(Obs, transMat, obsLik);
    printf("final == %d\n", finalState);

    return 0;
}
