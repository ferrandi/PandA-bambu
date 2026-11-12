// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER
#include <mdpi/mdpi_driver.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

int main()
{
   static uint32_t memory[262144];
   for(unsigned i = 0; i < 262144; ++i)
   {
      memory[i] = i ^ 0x13579bdfU;
   }
   static struct
   {
      uint32_t padding[32];
      uint32_t guard[32];
   } storage;
   for(auto& word : storage.padding)
   {
      word = 0xaaaaaaaa;
   }
   for(auto& word : storage.guard)
   {
      word = 0x5a5a5a5a;
   }
   __m_memmap_init(MDPI_MEMMAP_DEVICE);
   if(__m_memmap(0x40000000, memory, sizeof(memory)))
   {
      return EXIT_FAILURE;
   }
   if(__m_memmap(0x40100000, storage.guard, sizeof(storage.guard)))
   {
      return EXIT_FAILURE;
   }
   const bool plain = std::getenv("MDPI_TEST_PLAIN") != nullptr;
   const unsigned count = std::getenv("MDPI_TEST_MEMORIES") ? 2 : 1;
   for(unsigned i = 0; i < count; ++i)
   {
      if(plain)
      {
         __m_interface_mem();
      }
      else
      {
         __m_interface_mem_ramulator(10.0, 1);
      }
   }
   __m_sim_start();
   const auto result = __m_sim_end();
   __m_interface_fini();
   for(auto word : storage.padding)
   {
      if(word != 0xaaaaaaaa)
      {
         std::fprintf(stderr, "MDPI access escaped a mapped host buffer\n");
         return EXIT_FAILURE;
      }
   }
   return result;
}
