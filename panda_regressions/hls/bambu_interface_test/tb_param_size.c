#include <stdio.h>
#include <assert.h>
int __attribute__((noinline)) tb_param_size(int a[], int b[])
{
    return a[5] + b[5];
}

int main()
{
   int a[10];
   int b[10];

   for(int i = 0; i < 10; i++) {
       a[i] = i;
       b[i] = i * 2;
   }

   int res = tb_param_size(a, b);
   printf("res = %d\n", res);
   assert(res==15);
   return 0;
}
