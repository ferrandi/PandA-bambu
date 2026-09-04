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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/*
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */
#ifndef NDEBUG
#define NDEBUG
#endif
#include "condInstCombIfTaggedPass.hpp"
#include "debug_print.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>

#define CREATE_FATAL_REPORT(msg) (llvm::Twine(msg) + " (" + __func__ + ":" + llvm::Twine(__LINE__) + ")")
#define REPORT_FATAL_ERROR_WITH_REPORT(msg)         \
   do                                               \
   {                                                \
      report_fatal_error(CREATE_FATAL_REPORT(msg)); \
   } while(false)

namespace llvm
{
   static bool hasTag(Module& M)
   {
      auto* bambuArrPartMetadata = M.getNamedMetadata("bambu_array_partition");
      if(bambuArrPartMetadata == nullptr || bambuArrPartMetadata->getNumOperands() == 0)
         return false;

      auto* node = bambuArrPartMetadata->getOperand(0);
      if(node == nullptr || node->getNumOperands() == 0)
         return false;

      auto* mdStr = llvm::dyn_cast<llvm::MDString>(node->getOperand(0));
      return mdStr != nullptr && mdStr->getString() == "true";
   }

#if LLVM_VERSION_MAJOR >= 13
   PreservedAnalyses CondInstCombIfTaggedPass::run(Module& M, ModuleAnalysisManager& MAM)
   {
      if(!hasTag(M))
      {
         LLVM_DEBUG(
             llvm::dbgs() << "[CondInstCombIfTaggedPass] No tag found, running instruction combining on the module.\n");
         ModulePassManager InnerMPM;
         InnerMPM.addPass(createModuleToFunctionPassAdaptor(llvm::InstCombinePass()));
         return InnerMPM.run(M, MAM);
      }
      LLVM_DEBUG(llvm::dbgs() << "[CondInstCombIfTaggedPass] Tag found, skipping instruction combining.\n");
      return PreservedAnalyses::all();
   }
#endif

   bool CondInstCombIfTaggedPass::runOnModule(Module& M)
   {
#if LLVM_VERSION_MAJOR < 13
      if(!hasTag(M))
      {
         llvm::legacy::PassManager PM;
#if PANDA_LLVM_CLANG_MAJOR >= 11
         PM.add(llvm::createInstructionCombiningPass(1000));
#else
         PM.add(llvm::createInstructionCombiningPass(true));
#endif

         PM.run(M);
         return true;
      }
      return false;
#else
      REPORT_FATAL_ERROR_WITH_REPORT("Call to runOnModule not expected with current LLVM version");
      return false;
#endif
   }

   StringRef CondInstCombIfTaggedPass::getPassName() const
   {
      return "CondInstCombIfTaggedPass";
   }

   char CondInstCombIfTaggedPass::ID = 0;

} // namespace llvm
