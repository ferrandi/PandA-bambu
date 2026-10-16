// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "mdpi/mdpi_banked.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static void check(bool condition, const char* message)
{
   if(!condition)
   {
      std::fprintf(stderr, "FAIL: %s\n", message);
      std::exit(1);
   }
}

int main()
{
   const size_t sizes[] = {1, 7, 8, 9, 17, 31, 32, 33, 65};
   const unsigned counts[] = {1, 2, 3, 5, 8};
   const unsigned chunks[] = {8, 64, 1024};
   const unsigned alignments[] = {8, 32, 64};
   for(unsigned count : counts)
      for(unsigned chunk : chunks)
         for(unsigned align : alignments)
         {
            unsigned ids[8];
            for(unsigned i = 0; i < count; ++i)
            {
               ids[i] = (i * 3 + 2) % 8;
            }
            unsigned effective = 1;
            while(effective < count)
            {
               effective <<= 1;
            }
            unsigned partial = 0, selector = 0;
            size_t required[8] = {}, used[8] = {};
            char* banks[8] = {};
            char* objects[9];
            size_t aligned[9];
            std::vector<unsigned char> expected[8];
            size_t global_offset = 0;
            for(unsigned obj = 0; obj < 9; ++obj)
            {
               const size_t size = sizes[obj];
               aligned[obj] = ((size + align - 1) / align) * align;
               // Exact-size allocations expose even one-byte source overreads under ASan.
               objects[obj] = static_cast<char*>(std::malloc(size));
               for(size_t i = 0; i < size; ++i)
               {
                  objects[obj][i] = static_cast<char>((obj * 37 + i) & 255);
               }
               __m_banked_reserve(aligned[obj], ids, count, chunk, &selector, &partial, required);
               // Independent bytewise reference based on the logical address, including padding.
               for(size_t i = 0; i < aligned[obj]; ++i, ++global_offset)
               {
                  const unsigned bank = ids[(global_offset / chunk) % effective % count];
                  expected[bank].push_back(i < size ? (obj * 37 + i) & 255 : 0);
               }
            }
            for(unsigned bank = 0; bank < 8; ++bank)
            {
               if(required[bank])
               {
                  banks[bank] = static_cast<char*>(std::malloc(required[bank] * chunk));
               }
            }
            partial = selector = 0;
            for(unsigned obj = 0; obj < 9; ++obj)
            {
               __m_banked_transfer(aligned[obj], sizes[obj], objects[obj], banks, ids, count, chunk, &selector,
                                   &partial, used, 0);
            }
            for(unsigned bank = 0; bank < 8; ++bank)
            {
               check(used[bank] == expected[bank].size(), "bank size differs from logical-address reference");
               check(__m_banked_space_fits(required[bank], chunk, required[bank] * chunk), "exact capacity rejected");
               check(!__m_banked_space_fits(required[bank] + 1, chunk, required[bank] * chunk), "overflow accepted");
               if(used[bank])
               {
                  check(!std::memcmp(banks[bank], expected[bank].data(), used[bank]), "data/padding mapping mismatch");
               }
               for(size_t i = 0; i < used[bank]; ++i)
               {
                  banks[bank][i] ^= 0xa5;
               }
            }
            std::memset(used, 0, sizeof(used));
            partial = selector = 0;
            for(unsigned obj = 0; obj < 9; ++obj)
            {
               __m_banked_transfer(aligned[obj], sizes[obj], objects[obj], banks, ids, count, chunk, &selector,
                                   &partial, used, 1);
               for(size_t i = 0; i < sizes[obj]; ++i)
               {
                  check(static_cast<unsigned char>(objects[obj][i]) == (((obj * 37 + i) & 255) ^ 0xa5),
                        "writeback corrupted a valid byte");
               }
               std::free(objects[obj]);
            }
            for(auto bank : banks)
            {
               std::free(bank);
            }
         }
   // Sizes and counts must not truncate above UINT_MAX; no 4 GiB allocation needed.
   if(sizeof(size_t) > 4)
   {
      const unsigned ids[] = {0};
      unsigned selector = 0, partial = 0;
      size_t required[] = {0};
      __m_banked_reserve(static_cast<size_t>((uint64_t(1) << 32) + 17), ids, 1, 1U << 20, &selector, &partial,
                         required);
      check(required[0] == 4097 && partial == 17, "reservation size truncated above 4 GiB");
      check(!__m_banked_space_fits(SIZE_MAX, 1024, SIZE_MAX), "capacity multiplication overflow accepted");
   }
   std::puts("BANKED_MEMORY_PASS: 405 objects, 45 layouts, capacity limits and size_t arithmetic");
   return 0;
}
