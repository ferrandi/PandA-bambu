/*
 *
 *                   _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                  _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *                 _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *                _/      _/    _/ _/    _/ _/   _/ _/    _/
 *               _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *             ***********************************************
 *                              PandA Project
 *                     URL: http://panda.dei.polimi.it
 *                       Politecnico di Milano - DEIB
 *                        System Architectures Group
 *             ***********************************************
 *                Copyright (C) 2025-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
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
