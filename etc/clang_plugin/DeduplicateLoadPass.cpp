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
#include "panda_clang_compat.hpp"

/**
 * @file DeduplicateLoadPass.cpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OmpTaskGraphConstants.hpp"
#include "OpenMPTaskPassConfig.hpp"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>

#include <cassert>
#include <map>

using namespace llvm;

namespace
{
#if PANDA_LLVM_CLANG_MAJOR >= 19
   bool isValidPtr(Instruction& I)
   {
      std::string stringBuff;
      raw_string_ostream stream(stringBuff);
      I.print(stream);

      const auto pos = stringBuff.find('@');
      return pos == std::string::npos;
   }

   constexpr uint8_t OPEN_FLAG = 0;
   constexpr uint8_t CLOSE_FLAG_WITH_DEPS = 1;
   constexpr uint8_t CLOSE_FLAG = 2;
   const std::string targetOmpTaskAllocName = "__kmpc_omp_task_alloc";
   const std::string targetOmpTaskSubmissionDepsName = "__kmpc_omp_task_with_deps";
   const std::string targetOmpTaskSubmissionName = "__kmpc_omp_task";

   bool isTarget(Instruction const& inst, const uint8_t flag)
   {
      StringRef instruction_name = inst.getName();
      StringRef opcode_name = inst.getOpcodeName();
      if(auto* CI = dyn_cast<CallInst>(&inst))
      {
         StringRef name = CI->getCalledFunction()->getName();
         if(flag == OPEN_FLAG && name.data() == targetOmpTaskAllocName)
         {
            return true;
         }
         else if(flag == CLOSE_FLAG_WITH_DEPS && name.data() == targetOmpTaskSubmissionDepsName)
         {
            return true;
         }
         else if(flag == CLOSE_FLAG && name.data() == targetOmpTaskSubmissionName)
         {
            return true;
         }
      }
      return false;
   }

   bool isValidBlock(BasicBlock& BB)
   {
      for(auto& I : BB)
      {
         if(isTarget(I, OPEN_FLAG) || isTarget(I, CLOSE_FLAG) || isTarget(I, CLOSE_FLAG_WITH_DEPS))
         {
            return true;
         }
      }
      return false;
   }

   struct DeduplicateLoadPass : public PassInfoMixin<DeduplicateLoadPass>
   {
      PreservedAnalyses run(Function& F, FunctionAnalysisManager&)
      {
         // Map to keep track of unique loads per pointer operand
         std::map<Value*, LoadInst*> uniqueLoads;
         std::vector<Instruction*> toRemove;

         if(F.getName().str().find(Bambu::OpenMP::Task::Constants::targetOmpOutlinedFnNamePrefix) ==
                std::string::npos ||
            F.getName().find(Bambu::OpenMP::Task::Constants::bambuHostReservedFnName) != std::string::npos)
         {
            return PreservedAnalyses::all();
         }

         errs() << "Visiting target function: " << F.getName() << "\n";

         bool modified = false;

         for(auto& BB : F)
         {
            if(!isValidBlock(BB))
            {
               continue;
            }
            for(auto& I : BB)
            {
               if(!isa<LoadInst>(I))
               {
                  continue;
               }
               if(auto* load = dyn_cast<LoadInst>(&I))
               {
                  std::string stringBuff;
                  raw_string_ostream stream(stringBuff);
                  I.print(stream);
                  if(!isValidPtr(I))
                  {
                     continue;
                  }

                  Value* ptrOperand = load->getPointerOperand();

                  // Check if we've already seen a load from this pointer operand
                  if(uniqueLoads.find(ptrOperand) != uniqueLoads.end())
                  {
                     std::string stringBuff2;
                     raw_string_ostream stream2(stringBuff2);
                     uniqueLoads[ptrOperand]->print(stream2);
                     errs() << "Replacing " << stringBuff << " with " << stringBuff2 << '\n';
                     LoadInst* firstLoad = uniqueLoads[ptrOperand];
                     assert(firstLoad);

                     // Replace the current load with the cached unique load
                     load->replaceAllUsesWith(firstLoad);
                     modified = true;
                     if(load->use_empty())
                     {
                        toRemove.push_back(load);
                     }
                  }
                  else
                  {
                     // Cache the first occurrence of the load
                     uniqueLoads[ptrOperand] = load;
                  }
               }
            }
         }

         // Delete marked instructions safely after traversal
         for(Instruction* inst : toRemove)
         {
            inst->eraseFromParent();
            modified = true;
         }

         return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
      }
   };

#else

   struct DeduplicateLoadPass : public PassInfoMixin<DeduplicateLoadPass>
   {
      PreservedAnalyses run(Function& F, FunctionAnalysisManager&)
      {
         return PreservedAnalyses::all();
      }
   };

#endif
} // namespace

// Registration for the new pass
llvm::PassPluginLibraryInfo getDeduplicateLoadPassPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "DeduplicateLoadPass", LLVM_VERSION_STRING, [](PassBuilder& PB)
           {
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>)
                  {
                     if(Name == "deduplicateLoadInst")
                     {
                        FPM.addPass(DeduplicateLoadPass());
                        return true;
                     }
                     return false;
                  });
           }};
}

extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default"))) llvmGetPassPluginInfo()
{
   return getDeduplicateLoadPassPluginInfo();
}

#else

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/IR/LegacyPassManager.h>

namespace
{
   struct DeduplicateLoadPass : public llvm::ModulePass
   {
      char ID = 0;
      DeduplicateLoadPass() : llvm::ModulePass(ID)
      {
      }

      bool runOnModule(llvm::Module& M) override
      {
         return false;
      }
   };

} // namespace

#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(new DeduplicateLoadPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses DeduplicateLoadPass_Ox(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

#endif

#endif