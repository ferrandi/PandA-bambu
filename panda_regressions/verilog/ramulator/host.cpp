// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#define __BAMBU_IPC_ENTITY MDPI_ENTITY_DRIVER
#include <mdpi/mdpi_driver.h>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>

int main()
{
   const bool wait_for_simulator = std::getenv("MDPI_TEST_WAIT_SIM_EXIT") != nullptr;
   sigset_t child_mask, previous_mask;
   if(wait_for_simulator)
   {
      sigemptyset(&child_mask);
      sigaddset(&child_mask, SIGCHLD);
      if(sigprocmask(SIG_BLOCK, &child_mask, &previous_mask))
      {
         return EXIT_FAILURE;
      }
   }
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
         __m_interface_mem_ramulator(std::getenv("MDPI_TEST_PERIOD_NS") ?
                                        std::strtod(std::getenv("MDPI_TEST_PERIOD_NS"), nullptr) :
                                        10.0,
                                    1);
      }
   }
   __m_sim_start();
   const auto result = __m_sim_end();
   if(wait_for_simulator)
   {
      // Force simulator-first shutdown without sleeps. Keep the child waitable
      // so the driver's SIGCHLD handler can reap it after restoring the mask.
      siginfo_t child_info = {};
      int status;
      do
      {
         status = waitid(P_ALL, 0, &child_info, WEXITED | WNOWAIT);
      } while(status == -1 && errno == EINTR);
      if(status == -1 || sigprocmask(SIG_SETMASK, &previous_mask, nullptr))
      {
         return EXIT_FAILURE;
      }
   }
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
