#include "fft.h"

int main(){
	TYPE a_x[512];
	TYPE a_y[512];
	int i;
	float max, min;
        max = 2147483646.0;
        min = -2147483646.0;

	for( i = 0; i < 512; i++){
		a_x[i] = (TYPE)(i);//(TYPE)(((double) rand() / (RAND_MAX)) * (max-min) + min);//i;
		a_y[i] = (TYPE)(i);//(((double) rand() / (RAND_MAX)) * (max-min) + min);//i;
	}

    printf("ORIG!\n");
	for( i = 0; i < 512; i++){
	    printf("x = %f y = %f \n", a_x[i], a_y[i]);
	}

	fft1D_512(a_x, a_y);

    printf("OUTPUT\n");
	for( i = 0; i < 512; i++){
	    printf("x = %f y = %f \n", a_x[i], a_y[i]);
	}
    
	fft1D_512(a_x, a_y);

    printf("NORMAL\n");
	for( i = 0; i < 512; i++){
	    printf("x = %f y = %f \n", a_x[i]/512, a_y[i]/512);
	}

	return 0;
}
