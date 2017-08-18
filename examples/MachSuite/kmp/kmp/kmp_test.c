#include "kmp.h"
#include <string.h>
#include "TR.h"

int main(){
    char *y = "bull";//moose";
    //char randomletter = 
    FILE *f = fopen("TR.txt", "r");
/*
    fseek(f, 0, SEEK_END);
    long pos = ftell(f);
    fseek(f, 0, SEEK_SET);
    printf("%dpos\n\n", pos);
    char *bytes = malloc(pos);
    
    fread(bytes, pos, 1, f);
    fclose(f);
 */   
    int j;
    int outs;
    outs = 0;
    int i;

    char x[STRING_SIZE];
    for(j=0;j<STRING_SIZE;j++){
        x[j] = 'a';
    }
//    for(i=0; i < STRING_SIZE; i++){
//        fscanf(f, "%s", &x);
//    }



    for(j=0;j<STRING_SIZE;j++){
       // printf("%c", tr[j]);// = bytes[j];
    }

    //char y[STRING_SIZE];

    for(i=0;i<PATTERN_SIZE;i++){
        //y[i] = 'A';//+ (random() % 26);
        //printf("%c", y[i]);
    }

    //for(i=0;i<STRING_SIZE;i++){
        //x[i] = 'A' + (random() % 26);
        //outs[i] = 0;
    //}

    outs = kmp(y, tr);

    //for(i=0;i<STRING_SIZE;i++){
     //   if(outs[i] > 0)
            printf("outs = %d \n",outs);
    //}

    return 0;
}
