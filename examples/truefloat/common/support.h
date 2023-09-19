#include <inttypes.h>
#include <stdlib.h>

///// File and section functions
char* readfile(int fd);
char* find_section_start(char* s, int n);

///// Array read functions
#define SECTION_TERMINATED -1
int parse_string(char* s, char* arr, int n); // n==-1 : %%-terminated
int parse_uint8_t_array(char* s, uint8_t* arr, int n);
int parse_uint16_t_array(char* s, uint16_t* arr, int n);
int parse_uint32_t_array(char* s, uint32_t* arr, int n);
int parse_uint64_t_array(char* s, uint64_t* arr, int n);
int parse_int8_t_array(char* s, int8_t* arr, int n);
int parse_int16_t_array(char* s, int16_t* arr, int n);
int parse_int32_t_array(char* s, int32_t* arr, int n);
int parse_int64_t_array(char* s, int64_t* arr, int n);
int parse_float_array(char* s, float* arr, int n);
int parse_double_array(char* s, double* arr, int n);

///// Array write functions
int write_string(int fd, char* arr, int n);
int write_uint8_t_array(int fd, uint8_t* arr, int n);
int write_uint16_t_array(int fd, uint16_t* arr, int n);
int write_uint32_t_array(int fd, uint32_t* arr, int n);
int write_uint64_t_array(int fd, uint64_t* arr, int n);
int write_int8_t_array(int fd, int8_t* arr, int n);
int write_int16_t_array(int fd, int16_t* arr, int n);
int write_int32_t_array(int fd, int32_t* arr, int n);
int write_int64_t_array(int fd, int64_t* arr, int n);
int write_float_array(int fd, float* arr, int n);
int write_double_array(int fd, double* arr, int n);

int write_section_header(int fd);

///// Per-benchmark files
void run_benchmark(void* vargs);
void input_to_data(int fd, void* vdata);
void data_to_input(int fd, void* vdata);
void output_to_data(int fd, void* vdata);
void data_to_output(int fd, void* vdata);
int check_data(void* vdata);

extern int INPUT_SIZE;

///// TYPE macros
// Macro trick to automatically expand TYPE into the appropriate function
// (S)et (T)ype (A)nd (C)oncatenate
#define __STAC_EXPANDED(f_pfx, t, f_sfx) f_pfx##t##f_sfx
#define STAC(f_pfx, t, f_sfx) __STAC_EXPANDED(f_pfx, t, f_sfx)
// Invoke like this:
//   #define TYPE int32_t
//   STAC(write_,TYPE,_array)(fd, array, n);
// where array is of type (TYPE *)
// This translates to:
//   write_int32_t_array(fd, array, n);

#ifndef MAX_ULP
#define MAX_ULP 1.0l
#endif

#define FLOAT_EXC_OVF 0
#define FLOAT_EXC_STD 1
#define FLOAT_EXC_SAT 2

#define FLOAT_RND_NONE 0
#define FLOAT_RND_NEVN 1

#define IEEE64_FRAC_BITS 52
#define IEEE64_EXP_BITS 11
#define IEEE64_EXP_BIAS -1023
#define IEEE_RND FLOAT_RND_NEVN
#define IEEE_EXC FLOAT_EXC_STD
#define IEEE_ONE 1
#define IEEE_SUBNORM 1
#define IEEE_SIGN -1

#define IEEE64_SPEC_ARGS \
   IEEE64_EXP_BITS, IEEE64_FRAC_BITS, IEEE64_EXP_BIAS, IEEE_RND, IEEE_EXC, IEEE_ONE, IEEE_SUBNORM, IEEE_SIGN

#ifndef TEST_FRAC_BITS
#define TEST_FRAC_BITS IEEE64_FRAC_BITS
#endif
#ifndef TEST_EXP_BITS
#define TEST_EXP_BITS IEEE64_EXP_BITS
#endif
#ifndef TEST_EXP_BIAS
#define TEST_EXP_BIAS IEEE64_EXP_BIAS
#endif
#ifndef TEST_RND
#define TEST_RND IEEE_RND
#endif
#ifndef TEST_EXC
#define TEST_EXC IEEE_EXC
#endif
#ifndef TEST_ONE
#define TEST_ONE IEEE_ONE
#endif
#ifndef TEST_SUBNORM
#define TEST_SUBNORM 0
#endif
#ifndef TEST_SIGN
#define TEST_SIGN IEEE_SIGN
#endif
#define TEST_SPEC_ARGS \
   TEST_EXP_BITS, TEST_FRAC_BITS, TEST_EXP_BIAS, TEST_RND, TEST_EXC, TEST_ONE, TEST_SUBNORM, TEST_SIGN

#ifdef CUSTOM_INTERFACE
#ifdef __BAMBU_SIM__
#define DATA_TYPE unsigned long long
#else
#define DATA_TYPE double
#endif

#include <milieu.h>
#include <softfloat.h>

#define __float_to_in(in) __float_cast(in, IEEE64_SPEC_ARGS, TEST_SPEC_ARGS)
#define __out_to_float(out) __float_cast(out, TEST_SPEC_ARGS, IEEE64_SPEC_ARGS)

typedef union
{
   double d;
   unsigned long long u;
} vc_t;
#else
#define DATA_TYPE double

#define __float_to_in(in) in
#define __out_to_float(out) out

typedef union
{
   double d;
   double u;
} vc_t;
#endif

#define __float_recast(in) __out_to_float(__float_to_in(in))

#include <mdpi/mdpi_user.h>
