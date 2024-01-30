#pragma HLS interface port = b mode = none register
int test(int a, long long int* b)
{
   *b = a;
   return a;
}
