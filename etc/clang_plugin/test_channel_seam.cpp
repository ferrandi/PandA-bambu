/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2018-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file test_channel_seam.cpp
 * @brief Permanent control for the ac_channel seam classification
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 * Standalone driver (not a plugin) that pins down the positive and negative cases of
 * bambu_channel_seam::classify, in particular the template wrapper whose argument is
 * the address of a seam member - a name that merely contains a seam token and must
 * classify to None.
 *
 * Build (LLVM 4 or newer, using C++14 for LLVM 4 and C++17 for newer versions):
 *
 *   g++ -O1 -std=c++17 -I <clang include dir> -I etc/clang_plugin \
 *       -DPANDA_CLANG_MAJOR=<v> etc/clang_plugin/test_channel_seam.cpp \
 *       $(<prefix>/bin/llvm-config --ldflags --libs support --system-libs) \
 *       -o test_channel_seam
 *
 * Exit code: number of failed cases (0 = all good).
 */
#include "plugin_includes.hpp"

#include <cstdio>

int main()
{
   struct Case
   {
      const char* mangled;
      const char* label;
      int expected;
   };
   const Case cases[] = {
      // Positive controls: the five Bambu ABI signatures, int payload and the packed
      // (value + valid) _BitInt(33) form of the non-blocking peek.
      {"_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_v", "read, by value", 1},
      {"_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_Rb", "read, valid bit", 1},
      {"_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_RbRb", "read, valid+dummy", 1},
      {"_ZN10ac_channelIiE21_write_bambu_internalIiEEbT_", "write", 2},
      {"_ZN10ac_channelI4PairIiiEE21_write_bambu_internalIS1_EEbT_", "write, composite payload", 2},
      {"_ZN10ac_channelIiE20_peek_bambu_internalIiEEKT_v", "peek, by value", 3},
      {"_ZN10ac_channelIiE20_peek_bambu_internalIiEEKT_Rb", "peek, valid bit", 3},
      {"_ZN10ac_channelIiE20_peek_bambu_internalIDU33_EEKT_Rb", "peek, packed valid",
       3},
      // Negative controls.
      {"_Z7wrapperIXadL_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_vEEEvv",
       "template wrapper of a seam address", 0},
      {"_ZN6otherE10ac_channelIiE20_read_bambu_internalIiEEKT_v", "namespaced ac_channel", 0},
      {"_ZN10ac_channelIiE20_read_bambu_internalIiEEbPT_Rb", "seam name, wrong params", 0},
      {"_ZN10ac_channelIiE7fake_opIiEv", "other member of ac_channel", 0},
      {"_Z10fake_readv", "free function", 0},
      {"_ZN10ac_channelIiE20_read_bambu_internalIiEET_v", "non-const return", 0},
   };
   int fails = 0;
   for(const auto& c : cases)
   {
      const int got = (int)bambu_channel_seam::classify(llvm::StringRef(c.mangled));
      const bool ok = got == c.expected;
      if(!ok)
      {
         ++fails;
      }
      std::printf("[%s] %-34s expected=%d got=%d  %s\n", ok ? "PASS" : "FAIL", c.label,
                  c.expected, got, c.mangled);
   }
   std::printf("%d failed case(s)\n", fails);
   return fails;
}
