unsigned long long soft_umul(unsigned long long a, unsigned long long b)
{
   unsigned long long res = a * b;
   res = res % 33554432;
   return res;
}
