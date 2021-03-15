#pragma HLS_interface b none_registered
int test(int a, long long int* b)
{
   *b = a;
   return a;
}
