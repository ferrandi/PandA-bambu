__attribute__((optnone)) 
long long mul_pow2(long long a)
{
   return a * (1LL << 10);
}

long long mul_neg_pow2(long long a)
{
   return a * -(1LL << 7);
}

long long mul_dense(long long a)
{
   return a * 0x1F3D5B7LL;
}

long long mul_alt_aa(long long a)
{
   return a * 0xAAAAAAAAULL;
}

long long mul_alt_55(long long a)
{
   return a * 0x55555555ULL;
}

long long mul_small_3(long long a)
{
   return a * 3;
}

long long mul_neg_3(long long a)
{
   return a * -3;
}

long long mul_small_5(long long a)
{
   return a * 5;
}

long long mul_small_7(long long a)
{
   return a * 7;
}

long long mul_small_9(long long a)
{
   return a * 9;
}

long long mul_pow2_minus1(long long a)
{
   return a * ((1LL << 8) - 1);
}

long long mul_neg_pow2_minus1(long long a)
{
   return a * -((1LL << 8) - 1);
}

long long mul_pow2_plus1(long long a)
{
   return a * ((1LL << 8) + 1);
}

long long mul_neg_pow2_plus1(long long a)
{
   return a * -((1LL << 8) + 1);
}

long long mul_sparse(long long a)
{
   return a * ((1LL << 12) + (1LL << 5) + 1);
}

long long mul_balanced_case(long long a)
{
   return a * ((1LL << 9) + (1LL << 6) + (1LL << 3) + 1);
}

long long mul_balanced_case_neg(long long a)
{
   return a * -((1LL << 9) + (1LL << 6) + (1LL << 3) + 1);
}

long long mul_neg_dense(long long a)
{
   return a * -0x12345LL;
}

long long mul_zero(long long a)
{
   return a * 0;
}

long long mul_one(long long a)
{
   return a * 1;
}

long long mul_neg_one(long long a)
{
   return a * -1;
}

unsigned long long umul_pow2(unsigned long long a)
{
   return a * (1ULL << 20);
}

unsigned long long umul_pow2_minus1(unsigned long long a)
{
   return a * ((1ULL << 16) - 1);
}

unsigned long long umul_pow2_plus1(unsigned long long a)
{
   return a * ((1ULL << 16) + 1);
}

unsigned long long umul_dense(unsigned long long a)
{
   return a * 0xF0F0F0F0ULL;
}
