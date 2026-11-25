#include <stdio.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#ifndef DUPLEX_INDEPENDENT_PROFILE
#error "DUPLEX_INDEPENDENT_PROFILE must be defined (1=symmetric, 2=asymmetric)"
#endif

#if DUPLEX_INDEPENDENT_PROFILE != 1 && DUPLEX_INDEPENDENT_PROFILE != 2
#error "DUPLEX_INDEPENDENT_PROFILE must be 1 (symmetric) or 2 (asymmetric)"
#endif

#define DUPLEX_INDEPENDENT_MAXN 4096
#define DUPLEX_INDEPENDENT_N 38

extern "C" unsigned bambu_axi_burst_duplex_independent(unsigned *, unsigned *, unsigned);

static unsigned source_value(unsigned index) {
  return 0x10203040u + index * 0x01010101u;
}

static unsigned destination_canary(unsigned index) {
  return 0xdead0000u ^ (index * 0x045d9f3bu);
}

static unsigned pattern_value(unsigned index) {
  return (index * 0x9e3779b9u) ^ 0xa5a55a5au;
}

int main() {
  static unsigned src[DUPLEX_INDEPENDENT_MAXN];
  static unsigned dst[DUPLEX_INDEPENDENT_MAXN];
  static unsigned source_before[DUPLEX_INDEPENDENT_MAXN];
  static unsigned expected_dst[DUPLEX_INDEPENDENT_MAXN];
  unsigned expected_checksum = 0;

  for (unsigned i = 0; i < DUPLEX_INDEPENDENT_MAXN; ++i) {
    src[i] = source_value(i);
    source_before[i] = src[i];
    dst[i] = destination_canary(i);
    expected_dst[i] = dst[i];
  }
  for (unsigned i = 0; i < DUPLEX_INDEPENDENT_N; ++i) {
    expected_checksum += source_before[i];
    expected_dst[i] = pattern_value(i);
  }

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(src));
  m_param_alloc(1, sizeof(dst));
#endif

  const unsigned checksum = bambu_axi_burst_duplex_independent(
      src, dst, DUPLEX_INDEPENDENT_N);
  if (checksum != expected_checksum) {
    fprintf(stderr, "DUPLEX_INDEPENDENT_GOLDEN_FAIL checksum expected=%08x actual=%08x\n",
            expected_checksum, checksum);
    return 1;
  }
  for (unsigned i = 0; i < DUPLEX_INDEPENDENT_MAXN; ++i) {
    if (src[i] != source_before[i]) {
      fprintf(stderr, "DUPLEX_INDEPENDENT_GOLDEN_FAIL source index=%u expected=%08x actual=%08x\n",
              i, source_before[i], src[i]);
      return 1;
    }
    if (dst[i] != expected_dst[i]) {
      fprintf(stderr, "DUPLEX_INDEPENDENT_GOLDEN_FAIL destination index=%u expected=%08x actual=%08x\n",
              i, expected_dst[i], dst[i]);
      return 1;
    }
  }

#if DUPLEX_INDEPENDENT_PROFILE == 1
  printf("DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE=symmetric N=%u\n", DUPLEX_INDEPENDENT_N);
#elif DUPLEX_INDEPENDENT_PROFILE == 2
  printf("DUPLEX_INDEPENDENT_GOLDEN_PASS PROFILE=asymmetric N=%u\n", DUPLEX_INDEPENDENT_N);
#endif
  return 0;
}
