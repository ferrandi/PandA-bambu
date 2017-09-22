#include <stdio.h>

long fact(long);

main()
{
    long n=0;

    printf("Compute the factorial of n (n!) from 0 to 7\n\n");
    for(n=0; n < 8; ++n)
        printf("Factorial of %d is %d.\n", n, fact(n));
    return 0;

}
long fact(long n)
{
    if (n==0)
        return(1);
    else
        return(n*fact(n-1));
}

