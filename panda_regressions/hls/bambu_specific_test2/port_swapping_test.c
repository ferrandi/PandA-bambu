int ret(int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11) __attribute__((noinline));

int ret(int p1, int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10, int p11){
   return p1 - p2 - p3 - p4 - p5 - p6 - p7 - p8 - p9 - p10 - p11;
}

int function(int a, int b, int c, int d, int e, int f, int g, int h, int i, int l, int m, int n, int o,
   int p, int q, int r)
{
   int v0, v1, v2, v05, v3, v4, v6, v7;
   int p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11;
   v1 = a - b;
   v2 = b - c;
   v05 = e - f;
   v3 = c - d;
   v4 = d - e;
   
   v6 = f - g;
   v7 = g - h;
   v0 = h - i;

   p6 = v05 * v3;
   p1 = v1 * v2;
   p2 = v1 * v4;
   p3 = v2 * v05;
   p4 = v3 * v0;
   p5 = v4 * v7;
   p7 = v0 * v6;
   p8 = v2 * v4;
   p9 = v3 * v2;
   p10 = v6 * v7;
   p11 = v05 * v7;

   return ret(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
}