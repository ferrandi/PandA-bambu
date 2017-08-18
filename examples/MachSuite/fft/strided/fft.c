#include "fft.h"

void fft(double real[size], double img[size], double real_twid[size], double img_twid[size]){
    int even, odd, span, log, rootindex;
    double temp;
    log = 0;

    for(span=size>>1; span; span>>=1, log++){
        for(odd=span; odd<size; odd++){
            odd |= span;
            even = odd ^ span;

            temp = real[even] + real[odd];
            real[odd] = real[even] - real[odd];
            real[even] = temp;

            temp = img[even] + img[odd];
            img[odd] = img[even] - img[odd];
            img[even] = temp;

            rootindex = (even<<log) & (size - 1);
            if(rootindex){
                temp = real_twid[rootindex] * real[odd] - 
                    img_twid[rootindex]  * img[odd];
                img[odd] = real_twid[rootindex]*img[odd] +
                    img_twid[rootindex]*real[odd];
                real[odd] = temp;
            }
        }
    }
}
