#include "needwun.h"
//#include "seq.h"

int main(){
    int i;
    char allignedA[N+M];
    char allignedB[M+M];

char seqA[N] = "tcgacgaaataggatgacagcacgttctcgtattagagggccgcggtacaaaccaaatgctgcggcgtacagggcacggggcgctgttcgggagatcgggggaatcgtggcgtgggtgattcgccggc";

  char seqB[M] = "ttcgagggcgcgtgtcgcggtccatcgacatgcccggtcggtgggacgtgggcgcctgatatagaggaatgcgattggaaggtcggacgggtcggcgagttgggcccggtgaatctgccatggtcgat";
    char sA[N];
    char sB[M];
    for(i=0;i<M;i++){
        sA[i] = seqA[i];
        sB[i] = seqB[i];
    }
    
    /*
    char sA[N] = "AGTA";
    char sB[M] = "ATA";
    */
    needwun(sA, sB, allignedA, allignedB);
    
    return 0;
}
