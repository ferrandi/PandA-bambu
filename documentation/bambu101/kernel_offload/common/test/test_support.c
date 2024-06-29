#include "support.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

// Fake stubs, usually implemented per-benchmark
void run_benchmark( void *vargs ) {}
void input_to_data(int fd, void *vdata) {}
void output_to_data(int fd, void *vdata) {}
void data_to_output(int fd, void *vdata) {}
int check_data(void *vdata, void *vref) {return 0;}
int INPUT_SIZE;


#define generate_test_TYPE_array(TYPE) \
void test_##TYPE##_array() { \
  char *p; \
  TYPE a[10]; \
  int i, fd; \
  \
  /* Write output */ \
  fd = open("testfile", O_WRONLY|O_CREAT|O_TRUNC, 0666); \
  assert(fd>1 && "Couldn't open file to write test output"); \
  for(i=0; i<10; i++) { \
    a[i] = (TYPE)i; \
  } \
  write_##TYPE##_array(fd, a, 10); \
  close(fd); \
  fd = open("testfile", O_RDONLY); \
  assert(fd>1 && "Couldn't open file to read test input"); \
  p = readfile(fd); \
  close(fd); \
  assert( parse_##TYPE##_array(p, a, 10)==0 ); \
  for(i=0; i<10; i++) { \
    assert(a[i]==((TYPE)i)); \
  } \
}
generate_test_TYPE_array(uint8_t)
generate_test_TYPE_array(uint16_t)
generate_test_TYPE_array(uint32_t)
generate_test_TYPE_array(uint64_t)
generate_test_TYPE_array(float)
generate_test_TYPE_array(double)

void test_section_jumping() {
  int fd;
  char *p, *s;

  fd = open("input_sections", O_RDONLY);
  assert(fd>1 && "Couldn't open file to read test input");
  p = readfile(fd);
  s = find_section_start(p, 0);
  assert(p==s && "Couldn't find zeroth section");
  s = find_section_start(p, 1);
  assert(p+3==s && "Couldn't find first section");
  s = find_section_start(p, 2);
  assert(p+3+5==s && "Couldn't find third section");
}

#define STRLEN 11
#define TESTSTR "hello world"
void test_strings() {
  char test[STRLEN+1] = TESTSTR;
  char a[STRLEN+1];
  char *p;
  int fd;

  test[STRLEN]=(char)0;

  // Fixed-length
  fd = open("testfile", O_WRONLY|O_CREAT|O_TRUNC, 0666);
  assert(fd>1 && "Couldn't open file to write test output");
  write_string(fd, test, STRLEN);
  close(fd);
  fd = open("testfile", O_RDONLY);
  assert(fd>1 && "Couldn't open file to read test input");
  p = readfile(fd);
  parse_string(p, a, STRLEN);
  assert( parse_string(p, a, STRLEN)==0 );
  assert( !memcmp(test, a, STRLEN) );
  close(fd);

  // Auto-length
  fd = open("testfile", O_WRONLY|O_CREAT|O_TRUNC, 0666);
  assert(fd>1 && "Couldn't open file to write test output");
  write_string(fd, test, SECTION_TERMINATED);
  write_section_header(fd);
  close(fd);
  fd = open("testfile", O_RDONLY);
  assert(fd>1 && "Couldn't open file to read test input");
  p = readfile(fd);
  assert( parse_string(p, a, SECTION_TERMINATED)==0 );
  assert( !memcmp(test, a, STRLEN+1) ); // include null-terminator
  close(fd);
}

void test_prng() {
  struct prng_rand_t S;
  uint64_t RA[3], RB[3];

  prng_srand(1, &S);
  RA[0] = prng_rand(&S);
  RA[1] = prng_rand(&S);
  RA[2] = prng_rand(&S);

  prng_srand(1, &S);
  RB[0] = prng_rand(&S);
  RB[1] = prng_rand(&S);
  RB[2] = prng_rand(&S);

  assert( RA[0]==RB[0] && "PRNG non-deterministic" );
  assert( RA[1]==RB[1] && "PRNG non-deterministic" );
  assert( RA[2]==RB[2] && "PRNG non-deterministic" );
}

int main(int argc, char **argv)
{
  test_section_jumping();
  test_uint8_t_array();
  test_uint16_t_array();
  test_uint32_t_array();
  test_uint64_t_array();
  test_float_array();
  test_double_array();
  test_strings();
  test_prng();

  printf("Success.\n");
  return 0;
}
