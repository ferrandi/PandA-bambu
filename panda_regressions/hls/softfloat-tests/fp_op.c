
#ifndef FPTYPE
#define FPTYPE float
#endif

#ifndef ITYPE
#define ITYPE int
#endif

#ifndef FP_OP
#define FP_OP (x)
#endif

typedef unsigned int ui32;
typedef unsigned long long ui64;
typedef int i32;
typedef long long i64;

FPTYPE fp_op2(FPTYPE x, FPTYPE y)
{
   return FP_OP;
}

ITYPE fp_to_i(FPTYPE x)
{
   return x;
}

FPTYPE i_to_fp(ITYPE x)
{
   return x;
}
