#include <stdio.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#ifndef DUPLEX_ALIAS_PROFILE
#error "DUPLEX_ALIAS_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

#if DUPLEX_ALIAS_PROFILE != 1 && DUPLEX_ALIAS_PROFILE != 2
#error "DUPLEX_ALIAS_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif

#ifndef DUPLEX_ALIAS_CASE
#error "DUPLEX_ALIAS_CASE must select 1=disjoint, 2=forward-overlap, 3=reverse-overlap, or 4=zero-trip"
#endif

#if DUPLEX_ALIAS_CASE < 1 || DUPLEX_ALIAS_CASE > 4
#error "unsupported DUPLEX_ALIAS_CASE"
#endif

#define DUPLEX_ALIAS_WORDS 256
#define DUPLEX_ALIAS_N 64
#define DUPLEX_ALIAS_DISJOINT_DST 128

extern "C" void kernel(int *, int *, int);

static int initial_value(int index) {
  return (index * 7919) - ((index % 7) * 12345) + 0x1357;
}

int main() {
  static int backing[DUPLEX_ALIAS_WORDS];
  static int expected[DUPLEX_ALIAS_WORDS];
  int *src;
  int *dst;
  int count = DUPLEX_ALIAS_N;

  for (int i = 0; i < DUPLEX_ALIAS_WORDS; ++i) {
    backing[i] = initial_value(i);
    expected[i] = backing[i];
  }

#if DUPLEX_ALIAS_CASE == 1
  src = backing;
  dst = backing + DUPLEX_ALIAS_DISJOINT_DST;
  const char *case_name = "disjoint";
#elif DUPLEX_ALIAS_CASE == 2
  src = backing;
  dst = backing + 1;
  const char *case_name = "forward-overlap";
#elif DUPLEX_ALIAS_CASE == 3
  src = backing + 1;
  dst = backing;
  const char *case_name = "reverse-overlap";
#else
  src = backing;
  dst = backing + 1;
  count = 0;
  const char *case_name = "zero-trip";
#endif

  for (int i = 0; i < count; ++i)
    expected[(dst - backing) + i] = expected[(src - backing) + i];

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(backing));
  m_param_alloc(1, sizeof(backing));
#endif

  kernel(src, dst, count);

  for (int i = 0; i < DUPLEX_ALIAS_WORDS; ++i) {
    if (backing[i] != expected[i]) {
      fprintf(stderr,
              "DUPLEX_ALIAS_GOLDEN_FAIL CASE=%s profile=%d index=%d expected=%08x actual=%08x\n",
              case_name, DUPLEX_ALIAS_PROFILE, i, (unsigned)expected[i],
              (unsigned)backing[i]);
      return 1;
    }
  }

  printf("DUPLEX_ALIAS_GOLDEN_PASS CASE=%s profile=%d N=%d%s\n", case_name,
         DUPLEX_ALIAS_PROFILE, count, count == 0 ? " NO_DATA_CHANGE=1" : "");
  return 0;
}
