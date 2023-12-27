#pragma HLS interface port = b mode = none_registered
int test(int a, long long int* b)
{
   *b = a;
   return a;
}
