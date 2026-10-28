// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Exercise the real comparison macros without pulling in channel interface templates.
#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER
#include "mdpi/mdpi_wrapper_compare.h"
#include <cstdlib>
#include <cstring>

typedef unsigned int payload_t __attribute__((vector_size(8)));

static bool compare(size_t size, bool corrupt_tail)
{
   size_t i;
   unsigned mismatch_count = 0;
   payload_t* P0 = static_cast<payload_t*>(std::malloc(size));
   const size_t P0_size_ref = size;
   void* P0_ref = std::malloc(size);
   if(!P0 || !P0_ref)
   {
      std::free(P0);
      std::free(P0_ref);
      return false;
   }
   std::memset(P0, 0x5a, size);
   std::memcpy(P0_ref, P0, size);
   if(corrupt_tail)
   {
      static_cast<unsigned char*>(P0_ref)[size - 1] ^= 1;
   }
   _ms_argcmp(ref, 0, mem);
   std::free(P0);
   return mismatch_count == (corrupt_tail ? 1U : 0U);
}

int main()
{
   const size_t sizes[] = {1, 7, 8, 9, 16, 17};
   for(size_t size : sizes)
   {
      if(!compare(size, false) || !compare(size, true))
      {
         return 1;
      }
   }
   return 0;
}
