#include "softfloat.c"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main()
{

  unsigned int exp, frac, sign; 
  unsigned int index_a, index_b;

 for(sign=0; sign < 2; ++sign)
 {
#if 1
  //testing zeros
  exp = 0;
  index_a = 0;
  index_b = 0;
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("A) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=((~sign)&1)<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("B) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 1;
  #pragma omp parallel for
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=(((~sign)&1)<<31)|(exp << 23)|index_a;
      b.b=(sign<<31)|(exp << 23)|index_a;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("C) sign=%u exp=%u, index_a=%u\n", sign, exp, index_a);
        abort();
      }
    }
#endif

#if 1
  exp = 0;
  //testing subnormals
  #pragma omp parallel for
  for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("D) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 1;
  //testing subnormals
  #pragma omp parallel for
  for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("E) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
  #pragma omp parallel for
  for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("F) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  //testing normals
  exp = 224;
  #pragma omp parallel for
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("G) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  //testing overflow to infinite
  exp = 254;
  #pragma omp parallel for
  for(index_a = 8388605; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=((126) << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("H) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

  //testing inf/nan
#if 1
  exp = 255;
  #pragma omp parallel for
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388606; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("I) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 255;
  index_a = 0;
  index_b = 0;
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = 0.0f / 0.0f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("J) a.b=%u b.b=%u, c.b=%u, d.b=%u, golden.b=%u \n", a.b, b.b, c.b, d.b, golden.b);
        printf("J) sign=%u exp=%u, index_a=%u, index_b=%u \n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 255;
  index_a = 0;
  index_b = 0;
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=((~sign)&1)<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("K) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  exp = 255;
  #pragma omp parallel for
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("L) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 255;
  #pragma omp parallel for
  for(index_a = 8388600; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      VOLATILE_DEF __convert32 a, b, c, d, golden;
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=((~sign)&1)<<31|(exp << 23)|index_b;
      c.b = __float32_divSRT4(a.b, b.b);
      d.b = __float32_divG(a.b, b.b);
      golden.f = a.f / b.f;
      if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
      {
        printf("M) a.b=%u b.b=%u, c.b=%u, d.b=%u, golden.b=%u \n", a.b, b.b, c.b, d.b, golden.b);
        printf("M) sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  #pragma omp parallel for
  for (exp = 1; exp < 255; ++exp)
    for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
      for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
      {
        VOLATILE_DEF __convert32 a, b, c, d, golden;
        a.b=sign<<31|(exp << 23)|index_a;
        b.b=((exp>31 ? exp-5 : 1) << 23)|index_b;
        c.b = __float32_divSRT4(a.b, b.b);
        d.b = __float32_divG(a.b, b.b);
        golden.f = a.f / b.f;
        if((c.b != golden.b && c.f != golden.f) || (d.b != golden.b && d.f != golden.f))
        {
          printf("sign=%u exp=%u, index_a=%u, index_b=%u\n", sign, exp, index_a, index_b);
          abort();
        }
      }
#endif
 }
  return 0;
}
