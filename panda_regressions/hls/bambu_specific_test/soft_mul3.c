long long soft_mul(long long a, long long b)
{
   long long res = a * b;
   res = res % 33554432;
   return res;
}
