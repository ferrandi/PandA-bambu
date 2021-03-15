#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#if defined(FLOAT32_CONFIGURATION) && defined(FLOAT64_CONFIGURATION) || !(defined(FLOAT32_CONFIGURATION) || defined(FLOAT64_CONFIGURATION))
#error Floating-point bitwidth should be configured
#endif

#ifdef FLOAT32_CONFIGURATION
#define SIGN_SHL     31
#define EXP_SHL      23
#define EXP_NORMAL   224
#define EXP_LAST     254
#define EXP_INF      255
union view_convert {
   uint32_t bits;
   float f;
};
#endif

#ifdef FLOAT64_CONFIGURATION
#define SIGN_SHL     63
#define EXP_SHL      52
#define EXP_NORMAL   1120
#define EXP_LAST     2046
#define EXP_INF      2047
union view_convert {
   uint64_t bits;
   double f;
};
#endif

#define MAX_ULP EXP_SHL

#define TEST_ONE_ARG (defined(TEST_FUNC_NAME) && defined(TEST_FUNC_EQ))
#define TEST_TWO_ARG (defined(TEST_FUNC_TWO_NAME) && defined(TEST_FUNC_TWO_EQ))

#ifndef ULP
#error Uint in last place should be specified
#endif

#if (!TEST_ONE_ARG && !TEST_TWO_ARG) || (TEST_ONE_ARG && TEST_TWO_ARG)
#error Tested function should be defined
#endif

#define M_BITS MAX_ULP-ULP

#if M_BITS < 0
#error Uint in last place greater than significand bitwidth
#endif


#if TEST_ONE_ARG
#ifdef FLOAT64_CONFIGURATION
extern double TEST_FUNC_NAME (double a);
extern double TEST_FUNC_EQ (double a);
#else
extern float TEST_FUNC_NAME (float a);
extern float TEST_FUNC_EQ (float a);
#endif

uint64_t operation_check(uint64_t sign, uint64_t exp, uint64_t a_start, uint64_t a_max)
{
   uint64_t ulp = (1ULL << (MAX_ULP - ULP)) - 1;
   if(a_max > (1ULL << ULP))
      a_max = (1ULL << ULP);

   #pragma omp parallel for
   for(uint64_t a = a_start; a < a_max; ++a)
   {
      volatile union view_convert f_a, f_res, f_golden, diff;
      f_a.bits = (sign << SIGN_SHL) | (exp << EXP_SHL) | (a << (MAX_ULP - ULP));
#ifdef NO_SUBNORMAL
      if(!isnormal(f_a.f)) { continue; }
#endif
      f_golden.f = TEST_FUNC_EQ(f_a.f);
#ifdef NO_SUBNORMAL
      if(!isnormal(f_golden.f)) { continue; }
#endif
      f_res.f = TEST_FUNC_NAME(f_a.f);
      if(f_golden.bits == f_res.bits || (isnan(f_res.f) && isnan(f_golden.f))) { continue; }
      double error = fabs(f_golden.f) == 0.0 ? (fabs(f_res.f - f_golden.f) / ldexp((double)1.0, -EXP_SHL)) : (fabs(f_res.f - f_golden.f) / ldexp(1.0, ilogb(f_golden.f) - EXP_SHL));
      if(error > ulp)
      {
         printf("NON VA NIENTE: lhs = %.10f<%llu> \nexpected = %.10f<%llu>\n  result = %.10f<%llu>\nerror = %f iteration = %llu\n", 
            f_a.f, f_a.bits, f_golden.f, f_golden.bits, f_res.f, f_res.bits, error, a - a_start);
         exit(-1);
      }
   }
   
   return a_max - a_start;
}
#endif

#if TEST_TWO_ARG
#ifdef FLOAT64_CONFIGURATION
extern double TEST_FUNC_TWO_NAME (double a, double b);
extern double TEST_FUNC_TWO_EQ (double a, double b);
#else
extern float TEST_FUNC_TWO_NAME (float a, float b);
extern float TEST_FUNC_TWO_EQ (float a, float b);
#endif

uint64_t operation_check(uint64_t a_sign, uint64_t b_sign, uint64_t exp, uint64_t a_start, uint64_t a_max, uint64_t b_start, uint64_t b_max)
{
   uint64_t ulp = (1ULL << (MAX_ULP - ULP)) - 1;
   if(a_max > (1ULL << ULP))
      a_max = (1ULL << ULP);
   if(b_max > (1ULL << ULP))
      b_max = (1ULL << ULP);

   uint64_t a, b;

   #pragma omp parallel for
   for(a = a_start; a < a_max; ++a)
   {
      for(b = b_start; b < b_max; ++b)
      {
         volatile union view_convert f_a, f_b, f_res, f_golden, diff;
         f_a.bits = (a_sign << SIGN_SHL) | (exp << EXP_SHL) | (a << (MAX_ULP - ULP));
         f_b.bits = (b_sign << SIGN_SHL) | (exp << EXP_SHL) | (b << (MAX_ULP - ULP));
#ifdef NO_SUBNORMAL
         if(!isnormal(f_a.f) || !isnormal(f_b.f)) { continue; }
#endif
         f_golden.f = TEST_FUNC_TWO_EQ(f_a.f, f_b.f);
#ifdef NO_SUBNORMAL
         if(!isnormal(f_golden.f)) { continue; }
#endif
         f_res.f = TEST_FUNC_TWO_NAME(f_a.f, f_b.f);
         if(f_golden.bits == f_res.bits || (isnan(f_res.f) && isnan(f_golden.f))) { continue; }
         double error = fabs(f_golden.f) == 0.0 ? (fabs(f_res.f - f_golden.f) / ldexp((double)1.0, -EXP_SHL)) : (fabs(f_res.f - f_golden.f) / ldexp(1.0, ilogb(f_golden.f) - EXP_SHL));
         if(error > ulp)
         {
            printf("NON VA NIENTE: lhs = %.10f<%llu>, rhs = %.10f<%llu> \nexpected = %.10f<%llu>\n  result = %.10f<%llu>\nerror = %f iteration = %llu\n", 
               f_a.f, f_a.bits, f_b.f, f_b.bits, f_golden.f, f_golden.bits, f_res.f, f_res.bits, error, (a - a_start)*(b_max - b_start) + b - b_start);
            exit(-1);
         }
      }
   }

   return (a_max - a_start)*(b_max - b_start);
}
#endif

int main() 
{
   uint64_t tested_ops = 0;

#if TEST_ONE_ARG
   // Testing zeros
   tested_ops += operation_check(0ULL, 0ULL, 0ULL, 1ULL);
   tested_ops += operation_check(1ULL, 0ULL, 0ULL, 1ULL);

   for(uint64_t sign = 0ULL; sign < 2; ++sign)
   {
      // Testing subnormals
#ifndef NO_SUBNORMAL
      tested_ops += operation_check(sign, 0ULL, 1ULL, (1ULL << ULP));
      tested_ops += operation_check(sign, 1ULL, 0ULL, (1ULL << ULP));
#endif

      // Testing normals
      tested_ops += operation_check(sign, EXP_NORMAL, 0ULL, (1ULL << ULP));

      // Testing overflow to infinite
      tested_ops += operation_check(sign, EXP_LAST, (1ULL << ULP) - 3, (1ULL << ULP));

      // Testing inf/nan
      tested_ops += operation_check(sign, EXP_INF, 0ULL, (1ULL << ULP));

#ifdef FLOAT32_CONFIGURATION
      printf("   Full range is beign tested ...\n");
      uint8_t exp;
      for(exp = 2ULL; exp < UINT8_MAX; ++exp)
      {
         tested_ops += operation_check(sign, exp, 0ULL, (1ULL << ULP));
      }
#endif
   }
#endif

#if TEST_TWO_ARG
   // Testing zeros
   tested_ops += operation_check(0ULL, 0ULL, 0ULL, 0ULL, 1ULL, 0ULL, 1ULL);
   tested_ops += operation_check(0ULL, 1ULL, 0ULL, 0ULL, 1ULL, 0ULL, 1ULL);
   tested_ops += operation_check(1ULL, 0ULL, 0ULL, 0ULL, 1ULL, 0ULL, 1ULL);
   tested_ops += operation_check(1ULL, 1ULL, 0ULL, 0ULL, 1ULL, 0ULL, 1ULL);

   for(uint64_t sign = 0ULL; sign < 2; ++sign)
   {
      // Testing subnormals
#ifndef NO_SUBNORMAL
      tested_ops += operation_check(sign, sign, 0ULL, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
      tested_ops += operation_check(sign, !sign, 0ULL, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
      tested_ops += operation_check(sign, sign, 1ULL, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
      tested_ops += operation_check(sign, !sign, 1ULL, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
#endif

      // Testing normals
      tested_ops += operation_check(sign, 0ULL, EXP_NORMAL, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));

      // Testing overflow to infinite
      tested_ops += operation_check(sign, 0ULL, EXP_LAST, (1ULL << ULP) - 3, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));

      // Testing inf/nan
      tested_ops += operation_check(sign, sign, EXP_INF, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
      tested_ops += operation_check(sign, !sign, EXP_INF, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
      tested_ops += operation_check(sign, sign, EXP_INF, 0ULL, 1ULL, 0ULL, 1ULL);
      tested_ops += operation_check(sign, !sign, EXP_INF, 0ULL, 1ULL, 0ULL, 1ULL);

#ifdef FLOAT32_CONFIGURATION
      printf("   Full range is beign tested ...\n");
      uint8_t exp;
      for(exp = 2ULL; exp < UINT8_MAX; ++exp)
      {
         tested_ops += operation_check(sign, sign, exp, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
         tested_ops += operation_check(sign, !sign, exp, 0ULL, (1ULL << ULP), (1ULL << ULP) - 8, (1ULL << ULP));
      }
#endif
   }
#endif

#if TEST_ONE_ARG
   const char* f_name = TOSTRING(TEST_FUNC_NAME);
#endif
#if TEST_TWO_ARG
   const char* f_name = TOSTRING(TEST_FUNC_TWO_NAME);
#endif
   printf("   Test for %s successful on %llu operations\n", f_name, tested_ops);
   return 0;
}
