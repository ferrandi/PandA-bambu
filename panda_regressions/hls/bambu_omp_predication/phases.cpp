// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0
#ifndef THREADS
#define THREADS 4
#endif
extern "C" void omp_predication(const int* a, int* result, int enabled)
{
  int total = 0;
#pragma omp parallel num_threads(THREADS) shared(total)
  {
    for (int phase = 0; phase < 4; ++phase) {
      // Alternating skipped/executed phases, with uniform barrier conditions.
      if ((phase + enabled) & 1) {
#pragma omp for
        for (int i = 0; i < 16; ++i)
          if (a[i] & 1) {
#pragma omp critical
            { total += a[i] + phase; }
          }
#pragma omp barrier
      }
    }
  }
  result[0] = total;
}
