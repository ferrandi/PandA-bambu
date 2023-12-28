int mixed_operations(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m)
{
#pragma HLS pipeline

   int aa = a * b;
   int ab = aa + c;
   int ac = ab * d;
   int ad = ac + e;
   int ae = ad * f;
   int af = ae + g;
   int ag = af * h;
   int ah = ag + i;
   int ai = ah * j;
   int aj = ai + k;
   int ak = aj * (l + m);

   return ak;
}
