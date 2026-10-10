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
 * @file test_ac_channel_abi_recognition.cpp
 * @brief Permanent control for the ac_channel hardware-primitive recognition
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 * Standalone driver (not a plugin) that pins down the positive and negative cases of
 * bambu_ac_channel_primitives::classify, in particular the template wrapper whose argument is
 * the address of an ac_channel member - a name that merely contains a recognized token
 * and must classify to None.
 *
 * CMake also compiles test_ac_channel_abi_fixture.cpp with the corresponding Clang
 * and passes its LLVM IR to this driver. This checks names emitted from ac_channel.h,
 * including substitutions whose indices depend on the payload type.
 *
 * Exit code: number of failed cases (0 = all good).
 */
#include "plugin_includes.hpp"

#include <cstdio>
#include <llvm/AsmParser/Parser.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SourceMgr.h>

static int checkCompilerNames(const char* path)
{
   llvm::LLVMContext context;
   llvm::SMDiagnostic error;
   auto module = llvm::parseAssemblyFile(path, error, context);
   if(!module)
   {
      error.print(path, llvm::errs());
      return 1;
   }
   unsigned counts[4] = {};
   int fails = 0;
   for(auto& F : *module)
   {
      const auto name = F.getName();
      const int expected = name.find("20_read_bambu_internal") != llvm::StringRef::npos  ? 1 :
                           name.find("21_write_bambu_internal") != llvm::StringRef::npos ? 2 :
                           name.find("20_peek_bambu_internal") != llvm::StringRef::npos  ? 3 :
                                                                                           0;
      if(!expected)
      {
         continue;
      }
      ++counts[expected];
      // The host bodies must not be treated as hardware primitives. Then reproduce
      // channelCtorCleanup, which exposes those same symbols as declarations.
      const bool host_ok = F.isDeclaration() ||
                           bambu_ac_channel_primitives::classifyFunction(&F) == bambu_ac_channel_primitives::Op::None;
      F.deleteBody();
      const int got = static_cast<int>(bambu_ac_channel_primitives::classifyFunction(&F));
      const bool ok = host_ok && got == expected;
      fails += !ok;
      std::printf("[%s] compiler symbol expected=%d got=%d  %s\n", ok ? "PASS" : "FAIL", expected, got,
                  name.str().c_str());
   }
   // Three payloads, each with blocking/non-blocking read and peek, plus write.
   if(counts[1] != 6 || counts[2] != 3 || counts[3] != 6)
   {
      std::printf("[FAIL] incomplete fixture: read=%u write=%u peek=%u\n", counts[1], counts[2], counts[3]);
      ++fails;
   }
   return fails;
}

int main(int argc, char** argv)
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
       {"_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_RbS4_", "read, valid+dummy", 1},
       {"_ZN10ac_channelIiE21_write_bambu_internalIiEEbT_", "write", 2},
       {"_ZN10ac_channelI4PairIiiEE21_write_bambu_internalIS1_EEbT_", "write, composite payload", 2},
       {"_ZN10ac_channelIiE20_peek_bambu_internalIiEEKT_v", "peek, by value", 3},
       {"_ZN10ac_channelIiE20_peek_bambu_internalIiEEKT_Rb", "peek, valid bit", 3},
       {"_ZN10ac_channelIiE20_peek_bambu_internalIiEEKT_RbS4_", "peek, valid+dummy", 3},
       {"_ZN10ac_channelIiE20_peek_bambu_internalIDU33_EEKT_Rb", "peek, packed valid", 3},
       // Negative controls.
       {"_Z7wrapperIXadL_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_vEEEvv", "template wrapper of a seam address",
        0},
       {"_ZN6otherE10ac_channelIiE20_read_bambu_internalIiEEKT_v", "namespaced ac_channel", 0},
       {"_ZN10ac_channelIiE20_read_bambu_internalIiEEbPT_Rb", "seam name, wrong params", 0},
       {"_ZN10ac_channelIiE7fake_opIiEv", "other member of ac_channel", 0},
       {"_Z10fake_readv", "free function", 0},
       {"_ZN10ac_channelIiE20_read_bambu_internalIiEET_v", "non-const return", 0},
       {"_ZN10ac_channelIiE20_read_bambu_internalIiEET_RbS3_", "legacy non-const return", 0},
       {"_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_RbS3_", "substitution is not bool reference", 0},
       {"_ZN10ac_channelIiE20_peek_bambu_internalIiEEKT_RiS4_", "two int references", 0},
       {"_ZN10ac_channelIiE20_read_bambu_internalIiEEKT_RbS99_", "invalid substitution index", 0},
   };
   int fails = 0;
   for(const auto& c : cases)
   {
      const int got = (int)bambu_ac_channel_primitives::classify(llvm::StringRef(c.mangled));
      const bool ok = got == c.expected;
      if(!ok)
      {
         ++fails;
      }
      std::printf("[%s] %-34s expected=%d got=%d  %s\n", ok ? "PASS" : "FAIL", c.label, c.expected, got, c.mangled);
   }
   if(argc != 2)
   {
      std::fprintf(stderr, "Usage: %s <compiler-generated fixture.ll>\n", argv[0]);
      return 1;
   }
   fails += checkCompilerNames(argv[1]);
   std::printf("%d failed case(s)\n", fails);
   return fails;
}
