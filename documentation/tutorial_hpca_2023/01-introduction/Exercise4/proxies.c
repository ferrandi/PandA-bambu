int __attribute__ ((noinline)) funcC(int a[2]){
    return a[0] * a[0] + a[1] * a[1];
}

int __attribute__ ((noinline)) funcB(int a[2]){
    int i;
    for(i=0; i<2; i++)
        a[i] = a[i] + 1;
    return funcC(a);
}

int funcA(){
    int temp1, temp2;
    int a[2] = {0,1};
    temp1 = funcC(a);
    temp2 = funcB(a);
    return temp1 + temp2;
}