#pragma HLS_interface i1 none
#pragma HLS_interface o2 none
#pragma HLS_interface o1 none
typedef short int mysint;

void kim(mysint* i1, short int i2, short int i3, short int* i4, short int i5, short int i6, const short int c1, const short int c2, const short int c3, const short int c4, const short int c6, _Bool COND1, _Bool COND2, short int* o1, short int* o2)
{
   int a, e, m, n, p, u, i, b, f, g, q, r, c, h, j, s, t, w, d, k, l;
   if(COND1)
   {
      if(COND2)
      {
         a = i3 - *i1;
         e = i2 + a;
         m = c3 + e;
         n = e - c2;
         p = a + c3;
         u = m + n;
         i = u + p;
      }
      else
      {
         b = *i1 + *i4;
         f = b - c2;
         g = c3 - b;
         q = f + c3;
         r = c2 + g;
         i = q + r;
      }
      *o1 = i + c6 + o1[0];
   }
   else
   {
      c = *i1 - i5;
      h = c2 + c;
      j = c - c2;
      s = c3 + h;
      t = j - c3;
      w = t + c4;
      *o1 = s + w;
   }
   d = i6 + c1;
   k = c2 + d;
   l = d - c4;
   *o2 = k - l;
}
