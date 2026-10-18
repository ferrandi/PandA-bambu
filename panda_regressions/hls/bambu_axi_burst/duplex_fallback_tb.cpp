#include <stdio.h>

#ifdef __BAMBU_SIM__
#include <mdpi/mdpi_user.h>
#endif

#ifndef DUPLEX_FALLBACK_PROFILE
#error DUPLEX_FALLBACK_PROFILE must select a duplex fallback profile
#endif

#if DUPLEX_FALLBACK_PROFILE != 1 && DUPLEX_FALLBACK_PROFILE != 2
#error unsupported DUPLEX_FALLBACK_PROFILE
#endif

#define DUPLEX_FALLBACK_MAXN 4096
#define DUPLEX_FALLBACK_N 38

extern "C" void bambu_axi_burst_duplex_fallback(int *data, int n);

static int initial_value(int index) {
  return (index * 7919) - ((index % 7) * 12345) + 0x1357;
}

static int canary_value(int index) {
  return 17011 - (index * 3571) + ((index % 11) * 991);
}

int main() {
  static int data[DUPLEX_FALLBACK_MAXN];
  static int initial_data[DUPLEX_FALLBACK_MAXN];
  static int expected_data[DUPLEX_FALLBACK_MAXN];

  for (int i = 0; i < DUPLEX_FALLBACK_MAXN; ++i) {
    const int value = i < DUPLEX_FALLBACK_N ? initial_value(i) : canary_value(i);
    data[i] = value;
    initial_data[i] = value;
    expected_data[i] = value;
  }

#if DUPLEX_FALLBACK_PROFILE == 1
  for (int i = 0; i < DUPLEX_FALLBACK_N; ++i) {
    expected_data[i] = initial_data[DUPLEX_FALLBACK_N - 1 - i];
  }
#elif DUPLEX_FALLBACK_PROFILE == 2
  for (int i = 0; i < DUPLEX_FALLBACK_N; ++i) {
    expected_data[i + 1] = expected_data[i] + 1;
  }
#endif

#ifdef __BAMBU_SIM__
  m_param_alloc(0, sizeof(data));
#endif

  bambu_axi_burst_duplex_fallback(data, DUPLEX_FALLBACK_N);

  for (int i = 0; i < DUPLEX_FALLBACK_MAXN; ++i) {
    if (data[i] != expected_data[i]) {
      fprintf(stderr, "DUPLEX_FALLBACK_GOLDEN_FAIL: data index=%d expected=%d actual=%d\n",
              i, expected_data[i], data[i]);
      return 1;
    }
  }

#if DUPLEX_FALLBACK_PROFILE == 1
  printf("DUPLEX_FALLBACK_GOLDEN_PASS PROFILE=phased_erew_reverse_same_bundle N=%d\n",
         DUPLEX_FALLBACK_N);
#elif DUPLEX_FALLBACK_PROFILE == 2
  printf("DUPLEX_FALLBACK_GOLDEN_PASS PROFILE=overlap_rw_same_bundle N=%d\n",
         DUPLEX_FALLBACK_N);
#endif
  return 0;
}
