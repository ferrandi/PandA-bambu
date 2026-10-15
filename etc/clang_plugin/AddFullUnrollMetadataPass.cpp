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
 * @file AddFullUnrollMetadataPass.cpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OmpTaskGraphConstants.hpp"
#include "OpenMPTaskPassConfig.hpp"
#include "Utils.hpp"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PassManager.h"
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace
{

#if PANDA_LLVM_CLANG_MAJOR >= 19
   /**
    * @brief LLVM Pass that adds full unroll metadata to loops within OpenMP outlined functions
    *
    * This pass identifies loops in OpenMP outlined functions and adds the
    * 'llvm.loop.unroll.full' metadata to ensure they are fully unrolled during optimization.
    * This is crucial for proper task dependency analysis in subsequent passes.
    */
   struct AddFullUnrollMetadataPass : PassInfoMixin<AddFullUnrollMetadataPass>
   {
      bool changed = false;
      /**
       * @brief Processes a loop and its nested loops to add full unroll metadata
       *
       * @param F The function containing the loop
       * @param L The loop to process
       *
       * The method recursively:
       * 1. Processes all nested subloops
       * 2. Checks if the loop already has unroll metadata
       * 3. Adds full unroll metadata if not present
       */
      void processLoop(Function& F, llvm::Loop* L)
      {
         LLVMContext& Ctx = F.getContext();
         if(!L)
         {
            return;
         }
         // Process the current loop, e.g., print loop header.
         // For each nested loop, recursively call processLoop.
         for(llvm::Loop* SubLoop : L->getSubLoops())
            processLoop(F, SubLoop);

         // Iterate over all loops in the function.
         Bambu::OpenMP::Task::Utils::log(errs(), 0, 1, "Visiting loop");
         L->print(errs(), false);
         errs() << "\n\n";
         MDNode* LoopID = L->getLoopID();
         if(!LoopID)
         {
            Bambu::OpenMP::Task::Utils::log(errs(), 0, 1, "Loop has no ID");
            return;
         }

         // Add the full unroll metadata and ignore any other defined loop unroll metadata
         bool hasFullUnroll = false;
         SmallVector<Metadata*> NewOperands;
         NewOperands.push_back(nullptr);
         for(unsigned i = 1, e = LoopID->getNumOperands(); i < e; ++i)
         {
            std::string stringBuff;
            raw_string_ostream stream(stringBuff);
            LoopID->getOperand(i)->print(stream);
            if(stringBuff.find(Bambu::OpenMP::Task::Constants::llmvLoopUnrollMetadataNamePrefix) == std::string::npos)
            {
               NewOperands.push_back(LoopID->getOperand(i));
            }
         }

         // Create a new distinct metadata node for the full unroll hint.
         MDNode* FullUnrollMD =
             MDNode::get(Ctx, MDString::get(Ctx, Bambu::OpenMP::Task::Constants::llmvLoopUnrollFullMetadata));
         NewOperands.push_back(FullUnrollMD);

         // Rebuild the loop metadata node.
         MDNode* NewLoopID = MDNode::get(Ctx, NewOperands);
         // Set self-reference in the first operand.
         NewLoopID->replaceOperandWith(0, NewLoopID);
         L->setLoopID(NewLoopID);

         changed = true;
      }

      /**
       * @brief Checks if a loop has metadata matching the given prefix
       *
       * @param L The loop to check
       * @param Prefix The metadata prefix to search for (e.g., "llvm.loop.unroll.")
       * @return true if matching metadata is found, false otherwise
       *
       * Source: https://llvm.org/doxygen/LoopUnrollAndJamPass_8cpp.html
       */
      bool hasAnyUnrollPragma(const Loop* L, StringRef Prefix)
      {
         if(MDNode* LoopID = L->getLoopID())
         {
            // First operand should refer to the loop id itself.
            assert(LoopID->getNumOperands() > 0 && "requires at least one operand");
            assert(LoopID->getOperand(0) == LoopID && "invalid loop id");

            for(unsigned I = 1, E = LoopID->getNumOperands(); I < E; ++I)
            {
               MDNode* MD = dyn_cast<MDNode>(LoopID->getOperand(I));
               if(!MD)
                  continue;

               MDString* S = dyn_cast<MDString>(MD->getOperand(0));
               if(!S)
                  continue;

               if(S->getString().starts_with(Prefix))
                  return true;
            }
         }
         return false;
      }

      /**
       * @brief Debug helper that verifies unroll metadata presence in loops
       *
       * @param L The loop to check
       *
       * Recursively checks all nested loops and prints diagnostic information
       * about the presence of unroll metadata.
       */
      void checkMeta(Loop* L)
      {
         if(!L)
         {
            return;
         }

         for(llvm::Loop* SubLoop : L->getSubLoops())
         {
            checkMeta(SubLoop);
         }

         const auto res = hasAnyUnrollPragma(L, Bambu::OpenMP::Task::Constants::llmvLoopUnrollFullMetadata);
         Bambu::OpenMP::Task::Utils::log(errs(), 0, 1, "Checking loop:");
         L->print(errs(), false);
         Bambu::OpenMP::Task::Utils::log(errs(), 0, 1, "\nhas loop unroll meta:", res);
      }

      /**
       * @brief Main entry point for the pass
       *
       * @param F The function to process
       * @param FAM Function analysis manager providing loop analysis
       * @return PreservedAnalyses indicating which analyses are preserved
       *
       * Only processes functions with OpenMP outlined function prefix.
       * Returns PreservedAnalyses::none() if changes were made,
       * PreservedAnalyses::all() otherwise.
       */
      PreservedAnalyses run(Function& F, FunctionAnalysisManager& FAM)
      {
         if(F.getName().find(Bambu::OpenMP::Task::Constants::targetOmpOutlinedFnNamePrefix) != std::string::npos &&
            F.getName().find(Bambu::OpenMP::Task::Constants::bambuHostReservedFnName) == std::string::npos)
         {
            Bambu::OpenMP::Task::Utils::log(errs(), 0, 1, "Adding unroll metadata to", F.getName());
            auto& LI = FAM.getResult<LoopAnalysis>(F);

            // Iterate over all loops in the function.
            for(Loop* L : LI)
            {
               processLoop(F, L);
            }

            for(Loop* L : LI)
            {
               checkMeta(L);
            }

            // Use O3 to unroll omp outlined loops
            PassBuilder PB;
            PB.registerFunctionAnalyses(FAM);
            FunctionPassManager FPM =
                PB.buildFunctionSimplificationPipeline(OptimizationLevel::O3, llvm::ThinOrFullLTOPhase::None);
            FPM.run(F, FAM);
         }

         return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
      }
   };

#else

   struct AddFullUnrollMetadataPass : PassInfoMixin<AddFullUnrollMetadataPass>
   {
      PreservedAnalyses run(Function& F, FunctionAnalysisManager& FAM)
      {
         return PreservedAnalyses::all();
      }
   };

#endif

} // end anonymous namespace

// Registration for the new Pass Manager.
extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default"))) llvmGetPassPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "AddFullUnrollMetadataPass", "v0.1.0", [](PassBuilder& PB)
           {
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>)
                  {
                     if(Name == "addFullUnrollMetadata")
                     {
                        FPM.addPass(AddFullUnrollMetadataPass());
                        return true;
                     }
                     return false;
                  });
           }};
}

#else

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/IR/LegacyPassManager.h>

namespace
{
   struct AddFullUnrollMetadataPass : public llvm::ModulePass
   {
      char ID = 0;
      AddFullUnrollMetadataPass() : llvm::ModulePass(ID)
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
   PM.add(new AddFullUnrollMetadataPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses AddFullUnrollMetadataPass_Ox(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
                                                                 loadPass);

#endif

#endif