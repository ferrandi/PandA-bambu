#include "softfloat.c"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main()
{

  VOLATILE_DEF __convert32 a, b, c, golden;
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
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(c.b != golden.b && c.f != golden.f)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
    {
      a.b=((~sign)&1)<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(c.b != golden.b && c.f != golden.f)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
  exp = 1;
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    {
      a.b=(((~sign)&1)<<31)|(exp << 23)|index_a;
      b.b=(sign<<31)|(exp << 23)|index_a;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(c.b != golden.b && c.f != golden.f)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  exp = 0;
  //testing subnormals
  for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=index_b;
      c.b = __addsubFloat32(a.b, b.b, 0);
      golden.f = a.f + b.f;
      if(c.b != golden.b && c.f != golden.f)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 1;
  //testing subnormals
  for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=index_b;
      c.b = __addsubFloat32(a.b, b.b, 0);
      golden.f = a.f + b.f;
      if(c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
  for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  //testing normals
  exp = 224;
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, 0);
      golden.f = a.f + b.f;
      if(c.b != golden.b && c.f != golden.f)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  //testing overflow to infinite
  exp = 254;
  for(index_a = 8388605; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=((exp-1) << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, 0);
      golden.f = a.f + b.f;
      if(c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

  //testing inf/nan
#if 1
  exp = 255;
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=index_b;
      c.b = __addsubFloat32(a.b, b.b, 0);
      golden.f = a.f + b.f;
      if(c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif
#if 1
  exp = 255;
  index_a = 0;
  index_b = 0;
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=((~sign)&1)<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, ((~sign)&1));
      golden.f = a.f + b.f;
      if(!(isnanf(c.f) == isnanf(golden.f)) && c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  exp = 127;
  index_a = 0;
  index_b = 0;
    {
      a.b=index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  exp = 255;
  for(index_a = 0; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
    for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
    {
      a.b=sign<<31|(exp << 23)|index_a;
      b.b=sign<<31|(exp << 23)|index_b;
      c.b = __addsubFloat32(a.b, b.b, sign);
      golden.f = a.f + b.f;
      if(!(isnanf(c.f) == isnanf(golden.f)) && c.b != golden.b)
      {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
        abort();
      }
    }
#endif

#if 1
  #pragma omp parallel for
  for (exp = 0; exp < 255; ++exp)
    for(index_a = 0/*2*/; index_a < VAL_RESIZE((~0ULL), 23); ++index_a)
      for(index_b = 8388600; index_b < VAL_RESIZE((~0ULL), 23); ++index_b)
      {
        VOLATILE_DEF __convert32 a, b, c, golden;
        a.b=sign<<31|(exp << 23)|index_a;
        b.b=((exp>31 ? exp-5 : 0) << 23)|index_b;
        c.b = __addsubFloat32(a.b, b.b, 0);
        golden.f = a.f + b.f;
        if(c.b != golden.b && c.f != golden.f)
        {
        printf("sign=%d exp=%d, index_a=%d, index_b=%d\n", sign, exp, index_a, index_b);
          abort();
        }
      }
#endif
 }
  return 0;
}
