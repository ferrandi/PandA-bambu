#include "div.h"
#include <string.h>

#include <mdpi/mdpi_user.h>

int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark(void* vargs)
{
   size_t i;
   struct bench_args_t* args = (struct bench_args_t*)vargs;
   for(i = 0; i < TESTS_COUNT; ++i)
   {
      vc_t lhs, rhs, res;
      lhs.d = args->a[i];
      rhs.d = args->b[i];
      printf("lhs: %.8e 0x%016llX\nrhs: %.8e 0x%016llX\n", lhs.d, lhs.u, rhs.d, rhs.u);
      lhs.u = __float_to_in(lhs.u);
      rhs.u = __float_to_in(rhs.u);
      printf("lhs: 0x%016llX\nrhs: 0x%016llX\n", lhs.u, rhs.u);
      res.u = double_prec_division(lhs.u, rhs.u);
      printf("res: 0x%016llX\n", res.u);
      res.u = __out_to_float(res.u);
      printf("res: %.8e 0x%016llX\n\n", res.d, res.u);
      args->c[i] = res.d;
   }
}

/* Input format:
%%: Section 1
double[TESTS_COUNT]: lhs
%%: Section 2
double[TESTS_COUNT]: rhs
*/

void input_to_data(int fd, void* vdata)
{
   struct bench_args_t* data = (struct bench_args_t*)vdata;
   char *p, *s;
   // Zero-out everything.
   memset(vdata, 0, sizeof(struct bench_args_t));
   // Load input string
   p = readfile(fd);
   // Section 1: a
   s = find_section_start(p, 1);
   parse_double_array(s, data->a, TESTS_COUNT);
   // Section 2: b
   s = find_section_start(p, 2);
   parse_double_array(s, data->b, TESTS_COUNT);
   free(p);
}

void data_to_input(int fd, void* vdata)
{
   struct bench_args_t* data = (struct bench_args_t*)vdata;
   // Section 1
   write_section_header(fd);
   write_double_array(fd, data->a, TESTS_COUNT);
   // Section 2
   write_section_header(fd);
   write_double_array(fd, data->b, TESTS_COUNT);
}

/* Output format:
%% Section 1
double[TESTS_COUNT]: result
*/

void output_to_data(int fd, void* vdata)
{
   struct bench_args_t* data = (struct bench_args_t*)vdata;

   char *p, *s;
   // Zero-out everything.
   memset(vdata, 0, sizeof(struct bench_args_t));
   // Load input string
   p = readfile(fd);
   // Section 1: c
   s = find_section_start(p, 1);
   parse_double_array(s, data->c, TESTS_COUNT);
   free(p);
}

void data_to_output(int fd, void* vdata)
{
   struct bench_args_t* data = (struct bench_args_t*)vdata;
   // Section 1
   write_section_header(fd);
   write_double_array(fd, data->c, TESTS_COUNT);
}

int check_data(void* vdata)
{
   struct bench_args_t* data = (struct bench_args_t*)vdata;
   int has_errors = 0;
   int i;

   for(i = 0; i < TESTS_COUNT; i++)
   {
      vc_t lhs, rhs;
      double ref;
      lhs.d = data->a[i];
      rhs.d = data->b[i];
      printf("GOLD:\nlhs: %.8e 0x%016llX\nrhs: %.8e 0x%016llX\n", lhs.d, lhs.u, rhs.d, rhs.u);
      lhs.u = __float_recast(lhs.u);
      rhs.u = __float_recast(rhs.u);
      printf("lhs: 0x%016llX\nrhs: 0x%016llX\n", lhs.u, rhs.u);
      ref = lhs.d / rhs.d;
      printf("res: %.8e 0x%016llX\n\n", ref, ref);
      has_errors |= m_float_distance(data->c[i], ref) > MAX_ULP;
   }

   // Return true if it's correct.
   return !has_errors;
}
