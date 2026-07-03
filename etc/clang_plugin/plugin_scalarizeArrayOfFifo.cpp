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
 */
/**
 * @file ScalarizeFifoArrayPass.cpp
 * @brief What it does (LLVM Module Pass — requires LLVM 15+ (opaque pointers))
 * ------------
 * Two complementary transformations:
 *
 * (A) ALLOCA SCALARIZATION
 *     Finds every  alloca [1 x hls::stream<...>]  in every function entry
 *     block and replaces it with  alloca %hls::stream<...>.
 *     GEPs that index through the singleton wrapper are all-zero and thus
 *     no-ops; they are removed and their uses replaced by the new alloca.
 *
 * (B) PARAMETER GEP CLEANUP
 *     After (A), call sites now pass a ptr-to-stream where the callee may
 *     still have GEPs of the form:
 *
 *       %p = getelementptr [1 x %hls::stream<T>], ptr %param, i32 0, i32 0
 *
 *     These GEPs are also all-zero (no-op address-wise) but are now
 *     semantically stale.  The pass finds every function parameter whose
 *     pointee type (as recorded in the source-typed GEPs that use it) was
 *     [1 x hls::stream<T>], checks that all such GEPs are all-zero, and
 *     replaces them with the parameter directly.
 *
 * With LLVM 15+ opaque pointers ("ptr" everywhere) no bitcasts or signature
 * changes are needed — "ptr" is "ptr" regardless of what it points to.
 *
 * Plugin invocation
 * -----------------
 *   opt --load-pass-plugin=libScalarizeFifoArray.so \
 *       --passes=scalarize-fifo-array input.ll -o output.ll
 *
 * @author Tommaso Fellegara <tommaso.fellegara@polimi.it>
 *
 */

#include "llvm/Config/llvm-config.h"

#if LLVM_VERSION_MAJOR >= 16
#include "plugin_scalarizeArrayOfFifo.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

static_assert(LLVM_VERSION_MAJOR >= 15, "ScalarizeFifoArrayPass requires LLVM 15+ (opaque pointers).");

using namespace llvm;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace
{
   Type* getArrayBaseType(Type* ty)
   {
      while(auto* arr = dyn_cast<ArrayType>(ty))
         ty = arr->getElementType();
      return ty;
   }

   bool allArrayDimsAreOne(Type* ty)
   {
      while(auto* arr = dyn_cast<ArrayType>(ty))
      {
         if(arr->getNumElements() != 1)
            return false;
         ty = arr->getElementType();
      }
      return true;
   }

   bool nameIsHlsStream(StringRef name)
   {
      return name.contains("hls::stream");
   }

   bool baseTypeIsHlsStream(Type* ty)
   {
      auto* st = dyn_cast<StructType>(getArrayBaseType(ty));
      return st && st->hasName() && nameIsHlsStream(st->getName());
   }

   bool isSingletonFifoArrayAlloca(const AllocaInst* AI)
   {
      Type* allocTy = AI->getAllocatedType();
      if(!isa<ArrayType>(allocTy))
         return false;
      if(!allArrayDimsAreOne(allocTy))
         return false;
      return baseTypeIsHlsStream(allocTy);
   }

   bool isAllZeroGep(const GetElementPtrInst* gep)
   {
      return llvm::all_of(gep->indices(), [](const Use& idx) {
         auto* ci = dyn_cast<ConstantInt>(idx);
         return ci && ci->isZero();
      });
   }

   bool isSingletonFifoArrayType(Type* ty)
   {
      if(!isa<ArrayType>(ty))
         return false;
      if(!allArrayDimsAreOne(ty))
         return false;
      return baseTypeIsHlsStream(ty);
   }

   // ---------------------------------------------------------------------------
   // (A) alloca [1 x hls::stream] → alloca hls::stream
   // ---------------------------------------------------------------------------
   bool scalarizeAllocas(Module& M)
   {
      bool changed = false;

      SmallVector<AllocaInst*, 16> candidates;
      for(Function& F : M)
      {
         if(F.isDeclaration())
            continue;
         for(Instruction& I : F.getEntryBlock())
            if(auto* AI = dyn_cast<AllocaInst>(&I))
               if(isSingletonFifoArrayAlloca(AI))
                  candidates.push_back(AI);
      }

      for(AllocaInst* oldAI : candidates)
      {
         Type* streamTy = getArrayBaseType(oldAI->getAllocatedType());
         IRBuilder<> builder(oldAI);
         AllocaInst* newAI = builder.CreateAlloca(streamTy, /*ArraySize=*/nullptr, oldAI->getName() + ".scalar");
         newAI->setAlignment(oldAI->getAlign());

         SmallVector<Use*, 16> uses;
         for(Use& U : oldAI->uses())
            uses.push_back(&U);

         SmallVector<GetElementPtrInst*, 8> gepsToErase;

         for(Use* U : uses)
         {
            if(auto* gep = dyn_cast<GetElementPtrInst>(U->getUser()))
            {
               if(isAllZeroGep(gep))
               {
                  gep->replaceAllUsesWith(newAI);
                  gepsToErase.push_back(gep);
               }
               else
               {
                  errs() << "[ScalarizeFifoArray] WARNING: non-zero GEP on "
                            "singleton alloca, left unchanged:\n"
                         << *gep << "\n";
               }
            }
            else
            {
               U->set(newAI);
            }
         }

         for(auto* gep : gepsToErase)
            gep->eraseFromParent();

         if(oldAI->use_empty())
            oldAI->eraseFromParent();

         changed = true;
      }

      return changed;
   }

   // ---------------------------------------------------------------------------
   // (B) Remove stale all-zero GEPs on parameters that now point to hls::stream
   // ---------------------------------------------------------------------------
   bool cleanupParameterGeps(Module& M)
   {
      bool changed = false;

      for(Function& F : M)
      {
         if(F.isDeclaration())
            continue;

         for(Argument& arg : F.args())
         {
            if(!arg.getType()->isPointerTy())
               continue;

            SmallVector<GetElementPtrInst*, 8> gepsOnArg;
            for(Use& U : arg.uses())
               if(auto* gep = dyn_cast<GetElementPtrInst>(U.getUser()))
                  gepsOnArg.push_back(gep);

            if(gepsOnArg.empty())
               continue;

            bool allGepsAreNoOpFifoGeps = llvm::all_of(gepsOnArg, [](GetElementPtrInst* gep) {
               return isSingletonFifoArrayType(gep->getSourceElementType()) && isAllZeroGep(gep);
            });

            if(!allGepsAreNoOpFifoGeps)
               continue;

            for(GetElementPtrInst* gep : gepsOnArg)
            {
               gep->replaceAllUsesWith(&arg);
               gep->eraseFromParent();
               changed = true;
            }
         }
      }

      return changed;
   }

} // anonymous namespace

// ---------------------------------------------------------------------------
// ScalarizeFifoArrayPass method definitions
// ---------------------------------------------------------------------------

char ScalarizeFifoArrayPass::ID = 0;

bool ScalarizeFifoArrayPass::exec(Module& M)
{
   bool changed = scalarizeAllocas(M);
   changed |= cleanupParameterGeps(M);
   return changed;
}

PreservedAnalyses ScalarizeFifoArrayPass::run(Module& M, ModuleAnalysisManager& /*AM*/)
{
   return exec(M) ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool ScalarizeFifoArrayPass::runOnModule(Module& M)
{
   return exec(M);
}

StringRef ScalarizeFifoArrayPass::getPassName() const
{
   return "ScalarizeFifoArrayPass";
}

void ScalarizeFifoArrayPass::getAnalysisUsage(AnalysisUsage& /*AU*/) const
{
}

// ---------------------------------------------------------------------------
// Legacy PM registration
// ---------------------------------------------------------------------------

static RegisterPass<ScalarizeFifoArrayPass> X("scalarize-fifo-array", "Scalarize singleton hls::stream array allocas",
                                              false, false);

// ---------------------------------------------------------------------------
// Plugin registration
// ---------------------------------------------------------------------------
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default")))
llvmGetPassPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "ScalarizeFifoArray", LLVM_VERSION_STRING, [](PassBuilder& PB) {
              PB.registerPipelineParsingCallback(
                  [](StringRef name, ModulePassManager& MPM, ArrayRef<PassBuilder::PipelineElement>) -> bool {
                     if(name == "scalarize-fifo-array")
                     {
                        MPM.addPass(llvm::ScalarizeFifoArrayPass());
                        return true;
                     }
                     return false;
                  });

              PB.registerPipelineStartEPCallback([](ModulePassManager& MPM, OptimizationLevel /*OL*/) {
                 MPM.addPass(llvm::ScalarizeFifoArrayPass());
              });
           }};
}

#endif