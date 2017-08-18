#include "md.h"

TYPE distance(
        TYPE position_x[nAtoms],
        TYPE position_y[nAtoms],
        TYPE position_z[nAtoms],
        int i,
        int j)
{
    TYPE delx, dely, delz, r2inv;
    delx = position_x[i] - position_x[j];
    dely = position_y[i] - position_y[j];
    delz = position_z[i] - position_z[j];
    r2inv = delx * delx + dely * dely + delz * delz;
    return r2inv;
}

inline void insertInOrder(TYPE currDist[maxNeighbors], 
        int currList[maxNeighbors], 
        int j, 
        TYPE distIJ)
{
    int dist, pos, currList_t;
    TYPE currMax, currDist_t;
    pos = maxNeighbors - 1;
    currMax = currDist[pos];
    if (distIJ > currMax){
        return;
    }
    for (dist = pos; dist > 0; dist--){
        if (distIJ < currDist[dist]){
            currDist[dist] = currDist[dist - 1];
            currList[dist]  = currList[pos  - 1];
        }
        else{
            break;
        }
        pos--;
    }
    currDist[dist] = distIJ;
    currList[dist]  = j;
}

int buildNeighborList(TYPE position_x[nAtoms], 
        TYPE position_y[nAtoms], 
        TYPE position_z[nAtoms], 
        int NL[nAtoms][maxNeighbors]
        )
{
    int totalPairs, i, j, k;
    totalPairs = 0;
    TYPE distIJ;
    for (i = 0; i < nAtoms; i++){
        int currList[maxNeighbors];
        TYPE currDist[maxNeighbors];
        for(k=0; k<maxNeighbors; k++){
            currList[k] = 0;
            currDist[k] = 999999999;
        }
        for (j = 0; j < maxNeighbors; j++){
            if (i == j){
                continue;
            }
            distIJ = distance(position_x, position_y, position_z, i, j);
            currList[j] = j;
            currDist[j] = distIJ;
        }
        totalPairs += populateNeighborList(currDist, currList, i, NL);
    }
    return totalPairs;
}

int populateNeighborList(TYPE currDist[maxNeighbors], 
        int currList[maxNeighbors], 
        const int i, 
        int NL[nAtoms][maxNeighbors])
{
    int idx, validPairs, distanceIter, neighborIter;
    idx = 0; validPairs = 0; 
    for (neighborIter = 0; neighborIter < maxNeighbors; neighborIter++){
        NL[i][neighborIter] = currList[neighborIter];
        validPairs++;
    }
    return validPairs;
}

int main(){
    printf("here");
    int i, iter, j, totalPairs;
    iter = 0; 

    srand(8650341L);

    printf("here");
    TYPE position_x[nAtoms];
    TYPE position_y[nAtoms];
    TYPE position_z[nAtoms];
    TYPE force_x[nAtoms];
    TYPE force_y[nAtoms];
    TYPE force_z[nAtoms];
    TYPE NL[nAtoms][maxNeighbors];
    int neighborList[size];

    printf("here");
    for  (i = 0; i < nAtoms; i++)
    {
        position_x[i] = rand();
        position_y[i] = rand();
        position_z[i] = rand();
        force_x[i] = rand();
        force_y[i] = rand();
        force_z[i] = rand();
    }

    
    printf("here");
    for(i=0; i<nAtoms; i++){
        for(j = 0; j < maxNeighbors; ++j){
            NL[i][j] = 0;
        }
    }
    printf("here");
    totalPairs = buildNeighborList(position_x, position_y, position_z, NL);

    for(i=0; i<nAtoms; i++){
        for(j = 0; j < maxNeighbors; ++j)
            neighborList[i*maxNeighbors + j] = NL[i][j];
    }

    //Function Call
    for(iter = 0; iter< MAX_ITERATION; iter++) {
        md_kernel(force_x, force_y, force_z, position_x, position_y, position_z, neighborList);
    }
    return 0;
}

