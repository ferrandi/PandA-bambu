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
   __m_memmap_init(MDPI_MEMMAP_DEVICE);
   if(__m_memmap(0x40000000, memory, sizeof(memory)))
   {
      return EXIT_FAILURE;
   }
   __m_interface_mem(10.0, 1);
   __m_sim_start();
   const auto result = __m_sim_end();
   __m_interface_fini();
   return result;
}
