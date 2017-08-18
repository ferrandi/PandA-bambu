//http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
#include "ellpack.h"

#define ran (TYPE)(((double) rand() / (RAND_MAX)) * (MAX-MIN) + MIN)

void fillVal(TYPE nzval[N*L], int colind[N*L], TYPE x[N]){
	int j, cur_indx, i;
    srand48(8650341L);
	for (i = 0; i < N; i++){
        x[i] = ran;
        cur_indx = 0;
        for(j=0; j < L; j++){
            //MAKE BETTER!...
            ///With... you know... stats.
            cur_indx = (TYPE)(((double) rand() / (RAND_MAX)) * ((L-1) - cur_indx) + cur_indx);
            printf("idx %d \n",cur_indx);
            if(cur_indx < L){
		        nzval[i*L + j] = ran;
                colind[i*L +j] = cur_indx;
            }
        }
	}
}

void initOut(TYPE y[N]){
    int i;
    for (i=0; i<N; i++){
        y[i] = 0;
    }
}

int main(){
    int colind[N*L];
    TYPE nzval[N*L];
    TYPE x[N];
    TYPE y[N];
    int i;

    srand48(8650341L);

	fillVal(nzval, colind, x);
    initOut(y);
    
	ellpack(nzval, colind, x, y);

	printf("\n");
	for(i = 0; i < N; i++){
		printf("%d ", y[i]);
	}
	printf("\n");

	return 0;
}
