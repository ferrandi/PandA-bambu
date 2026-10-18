// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>

#ifndef __MDPI_BANKED_H
#define __MDPI_BANKED_H

#include <stddef.h>
#include <string.h>

// The selector spans the next power of two. Extra selectors repeat the first
// physical banks; allocation, copying and writeback must advance the same cursor.
static inline unsigned __m_banked_selector_count(unsigned bank_count)
{
   unsigned count = 1;
   while(count < bank_count)
   {
      count <<= 1;
   }
   return count;
}

static inline int __m_banked_space_fits(size_t chunks, size_t chunk_size, size_t capacity)
{
   return chunk_size != 0 && chunks <= capacity / chunk_size;
}

static inline void __m_banked_reserve(size_t memory_size, const unsigned* banks, unsigned bank_count,
                                      unsigned chunk_size, unsigned* selector, unsigned* partial,
                                      size_t* required_chunks)
{
   const unsigned selector_count = __m_banked_selector_count(bank_count);
   while(memory_size > 0)
   {
      const unsigned bank = banks[*selector % bank_count];
      const size_t chunk_left = chunk_size - *partial;
      const size_t bytes = memory_size < chunk_left ? memory_size : chunk_left;
      if(*partial == 0)
      {
         ++required_chunks[bank];
      }
      memory_size -= bytes;
      // bytes never exceeds chunk_size, so the cursor stays representable.
      *partial += static_cast<unsigned>(bytes);
      if(*partial == chunk_size)
      {
         *partial = 0;
         *selector = (*selector + 1) % selector_count;
      }
   }
}

// memory_size includes reserved padding; valid_size is the caller's actual object.
// Padding advances the cursor in both directions, but never accesses the object.
static inline void __m_banked_transfer(size_t memory_size, size_t valid_size, char* external_addr, char** bank_pointers,
                                       const unsigned* banks, unsigned bank_count, unsigned chunk_size,
                                       unsigned* selector, unsigned* partial, size_t* space_used, int writeback)
{
   const unsigned selector_count = __m_banked_selector_count(bank_count);
   size_t offset = 0;
   while(memory_size > 0)
   {
      const unsigned bank = banks[*selector % bank_count];
      const size_t chunk_left = chunk_size - *partial;
      const size_t reserved = memory_size < chunk_left ? memory_size : chunk_left;
      const size_t valid_left = offset < valid_size ? valid_size - offset : 0;
      const size_t copied = valid_left < reserved ? valid_left : reserved;
      char* bank_addr = bank_pointers[bank] + space_used[bank];
      if(copied > 0)
      {
         if(writeback)
         {
            memcpy(external_addr + offset, bank_addr, copied);
         }
         else
         {
            memcpy(bank_addr, external_addr + offset, copied);
         }
      }
      if(!writeback && copied < reserved)
      {
         memset(bank_addr + copied, 0, reserved - copied);
      }
      space_used[bank] += reserved;
      // reserved never exceeds chunk_size, so the cursor stays representable.
      *partial += static_cast<unsigned>(reserved);
      if(*partial == chunk_size)
      {
         *partial = 0;
         *selector = (*selector + 1) % selector_count;
      }
      offset += reserved;
      memory_size -= reserved;
   }
}

#endif
