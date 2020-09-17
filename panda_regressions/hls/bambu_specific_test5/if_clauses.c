int int adders(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l)
{
   #pragma HLS_simple_pipeline
   
   int aa = a + b;
   int ab = aa + c;
   int ac = ab + d;
   int ad = ac + e;
   int ae = ad + f;
   int af = ae + g;
   int ag = af + h;
   int ah = ag + i;
   int ai = ah + j;
   int aj = ai + k;
   int ak = aj + l;
   
   return ak;
}

int multipliers(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l)
{

   #pragma HLS_simple_pipeline
   
   int aa = a * b;
   int ab = aa * c;
   int ac = ab * d;
   int ad = ac * e;
   int ae = ad * f;
   int af = ae * g;
   int ag = af * h;
   int ah = ag * i;
   int ai = ah * j;
   int aj = ai * k;
   int ak = aj * l;
   
   return ak;
}

int if_clauses(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l)
{

   #pragma HLS_simple_pipeline

   if(a < 13)
   {
     return adders(a, b, c, d, e, f, g, h, i, j, k, l);
   }
   return multipliers(a, b, c, d, e, f, g, h, i, j, k, l);
 }
