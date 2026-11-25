#include <stdio.h>
#include <stdint.h>
#include <string.h>

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
#error "DUPLEX_ALIAS_CASE must select a supported duplex alias case"
#endif

#if DUPLEX_ALIAS_CASE < 1 || DUPLEX_ALIAS_CASE > 6
#error "unsupported DUPLEX_ALIAS_CASE"
#endif

#define DUPLEX_ALIAS_WORDS 256
#define DUPLEX_ALIAS_N 64
#define DUPLEX_ALIAS_DISJOINT_DST 128

#ifdef DUPLEX_ALIAS_FLOAT_BITS
typedef float duplex_alias_data_t;
#else
typedef int duplex_alias_data_t;
#endif

extern "C" void kernel(duplex_alias_data_t *, duplex_alias_data_t *, int);

static int initial_value(int index) {
  return (index * 7919) - ((index % 7) * 12345) + 0x1357;
}

static uint32_t initial_bits(int index) {
#ifdef DUPLEX_ALIAS_FLOAT_BITS
  static const uint32_t payloads[] = {
      0x3fc00000u, 0xbfc00000u, 0x00000000u, 0x80000000u,
      0x7fc00001u, 0xffc00042u, 0x7f800000u, 0xff800000u};
  if (index < (int)(sizeof(payloads) / sizeof(payloads[0])))
    return payloads[index];
#endif
  return (uint32_t)initial_value(index);
}

static uint32_t load_bits(const duplex_alias_data_t *value) {
#ifdef DUPLEX_ALIAS_FLOAT_BITS
  uint32_t bits;
  memcpy(&bits, value, sizeof(bits));
  return bits;
#else
  return (uint32_t)*value;
#endif
}

static void store_bits(duplex_alias_data_t *value, uint32_t bits) {
#ifdef DUPLEX_ALIAS_FLOAT_BITS
  memcpy(value, &bits, sizeof(bits));
#else
  *value = (int)bits;
#endif
}

int main() {
  static duplex_alias_data_t backing[DUPLEX_ALIAS_WORDS];
  static uint32_t expected[DUPLEX_ALIAS_WORDS];
  duplex_alias_data_t *src;
  duplex_alias_data_t *dst;
  int count = DUPLEX_ALIAS_N;

  for (int i = 0; i < DUPLEX_ALIAS_WORDS; ++i) {
    const uint32_t bits = initial_bits(i);
    store_bits(&backing[i], bits);
    expected[i] = bits;
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
#elif DUPLEX_ALIAS_CASE == 4
  src = backing;
  dst = backing + 1;
  count = 0;
  const char *case_name = "zero-trip";
#elif DUPLEX_ALIAS_CASE == 5
  src = backing;
  dst = backing + DUPLEX_ALIAS_DISJOINT_DST;
  count = 4;
  const char *case_name = "nested-changing-base";
#else
  src = backing;
  dst = backing + DUPLEX_ALIAS_DISJOINT_DST;
  count = 4;
  const char *case_name = "nested-changing-count";
#endif

#if DUPLEX_ALIAS_CASE == 5
  for (int outer = 0; outer < count; ++outer) {
    const unsigned int inner_count = static_cast<unsigned int>(outer);
    for (unsigned int i = 0; i < inner_count; ++i)
      expected[(dst - backing) + outer * 8 + i] = expected[(src - backing) + outer * 8 + i];
  }
#elif DUPLEX_ALIAS_CASE == 6
  for (int outer = 0; outer < count; ++outer) {
    const unsigned int inner_count = static_cast<unsigned int>(outer);
    for (unsigned int i = 0; i < inner_count; ++i)
      expected[(dst - backing) + i] = expected[(src - backing) + i];
  }
#else
  for (int i = 0; i < count; ++i)
    expected[(dst - backing) + i] = expected[(src - backing) + i];
#endif

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(backing));
  m_param_alloc(1, sizeof(backing));
#endif

  kernel(src, dst, count);

  for (int i = 0; i < DUPLEX_ALIAS_WORDS; ++i) {
    const uint32_t actual = load_bits(&backing[i]);
    if (actual != expected[i]) {
      fprintf(stderr,
              "DUPLEX_ALIAS_GOLDEN_FAIL CASE=%s profile=%d index=%d expected=%08x actual=%08x\n",
              case_name, DUPLEX_ALIAS_PROFILE, i, (unsigned)expected[i], (unsigned)actual);
      return 1;
    }
  }

#if DUPLEX_ALIAS_CASE >= 5
  printf("DUPLEX_ALIAS_GOLDEN_PASS CASE=%s profile=%d N=%d OUTER=4\n", case_name,
         DUPLEX_ALIAS_PROFILE, count);
#elif defined(DUPLEX_ALIAS_FLOAT_BITS)
  printf("DUPLEX_ALIAS_GOLDEN_PASS CASE=%s profile=%d N=%d%s PAYLOAD=FLOAT32_BITS\n", case_name,
         DUPLEX_ALIAS_PROFILE, count, count == 0 ? " NO_DATA_CHANGE=1" : "");
#else
  printf("DUPLEX_ALIAS_GOLDEN_PASS CASE=%s profile=%d N=%d%s\n", case_name,
         DUPLEX_ALIAS_PROFILE, count, count == 0 ? " NO_DATA_CHANGE=1" : "");
#endif
  return 0;
}
