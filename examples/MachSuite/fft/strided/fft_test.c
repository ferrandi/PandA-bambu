#include "fft.h"

int main(){
    double data_x[size];
    double data_y[size];
    double img[size];
    double real[size];
    int i;

    //set up twiddles...
    double twoPI = 6.28318530717959;
    double typed;
    int n, N;
    N = size;

    //Pre-calc twiddles
    for(n=0; n<(N>>1); n++){
        typed = (double)(twoPI*n/N);
        real[n] = cos(typed);
        img[n] = (-1.0)*sin(typed);
    }

    //Init data
    for(i=0; i < size; i++) {
        data_x[i] = (double)(i);
        data_y[i] = (double)(i);
    }

    /*
    printf("Input:\n");
    for(i=0; i < size; i++) {
        printf("%f : %f\n", data_x[i], data_y[i]);
    }

    printf("fft:\n");
    */

    fft(data_x, data_y, real, img);

    printf("Output:\n");


    for(i=0; i < size; i++) {
        printf("%.2f : %.2f\n", data_x[i], data_y[i]);
    }

    return 0;
} 
