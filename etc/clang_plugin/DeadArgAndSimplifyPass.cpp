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
 * @file DeadArgAndSimplifyPass.cpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OmpTaskGraphConstants.hpp"
#include "OpenMPTaskPassConfig.hpp"
#include "Utils.hpp"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <cassert>
#include <string>
#include <vector>

using namespace llvm;
using namespace Bambu::OpenMP::Task;

//-----------------------------------------------------------------------------
// DeadArgAndSimplifyPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace
{
#if PANDA_LLVM_CLANG_MAJOR >= 19
   // TODO: in case pointers are repeated only the first is actively used and substituted from
   // the upper passes (if run inside the pipeline). Clean not used inut ptrs.
   void cleanTaskEntry(Function* F)
   {
      Utils::log(errs(), 0, 1, "Cleaning task entry", F->getName());
      Module* M = F->getParent();
      LLVMContext& Context = M->getContext();
      constexpr unsigned startIndex = 2;

      // Creation of transformed function
      std::vector<Type*> argTypes;
      unsigned idx = 0;
      for(auto& A : F->args())
      {
         if(idx < startIndex)
         {
            idx++;
            continue;
         }
         argTypes.push_back(A.getType());
      }

      FunctionType* NewFuncType = FunctionType::get(F->getReturnType(), argTypes, F->isVarArg());
      Function* newFunction = Function::Create(NewFuncType, F->getLinkage(), F->getName() + "_2", M);

      ValueToValueMapTy vMap;
      auto it = newFunction->arg_begin();
      idx = 0;
      for(auto& oldArg : F->args())
      {
         // Unused args have to be declared as undef values for CloneFunctionInto
         if(idx < startIndex)
         {
            vMap[&oldArg] = UndefValue::get(oldArg.getType());
            idx++;
            continue;
         }
         vMap[&oldArg] = &*it;
         it->setName(oldArg.getName());
         ++it;
      }
      SmallVector<ReturnInst*> returns;
      CloneFunctionInto(newFunction, F, vMap, CloneFunctionChangeType::LocalChangesOnly, returns);

      F->replaceAllUsesWith(newFunction);
      F->eraseFromParent();
   }

   /// Instrument the IR. This call will replace omp fork call with the called fn and remove all not needed openMP
   /// runtime calls
   /// @param F A @ref llvm::Function
   void simplifyForkCall(Function& F)
   {
      std::vector<std::tuple<Instruction*, Function*, std::vector<Value*>>> instructionsToAdd;
      std::vector<Instruction*> deleteList;
      if(F.getName().find("kmp") != std::string::npos)
      {
         return;
      }
      if(F.getName().find("omp") != std::string::npos)
      {
         return;
      }
      if(F.getName().find(Constants::injectedTaskNamePreifx) != std::string::npos)
      {
         return;
      }
      if(F.getName().find(Constants::bambuHostReservedFnName) != std::string::npos)
      {
         return;
      }
      Utils::log(errs(), 0, 1, "Simplifyng omp fork call");
      Utils::log(errs(), 2, 1, "input fn:", F.getName());
      for(BasicBlock& BB : F)
      {
         for(auto it = BB.begin(); it != BB.end(); ++it)
         {
            if(auto* callInst = dyn_cast<CallInst>(&(*it)))
            {
               if(callInst->getCalledFunction() &&
                  callInst->getCalledFunction()->getName().find(Constants::targetOmpForkCall) != std::string::npos)
               {
                  Utils::log(errs(), 2, 1, "got", callInst->getCalledFunction()->getName());
                  Value* lambdaFn = callInst->getArgOperand(2);
                  assert(dyn_cast<Function>(lambdaFn) && "expected a fn");
                  Utils::log(errs(), 2, 1, "called outlined is", lambdaFn->getName());
                  const unsigned totParams = callInst->getNumOperands() - 4;
                  std::vector<Value*> args;
                  for(unsigned i = 0; i < totParams; i++)
                  {
                     args.push_back(callInst->getOperand(3 + i));
                  }
                  Utils::log(errs(), 2, 1, "args:");
                  for(auto& a : args)
                  {
                     Utils::log(errs(), 2, 0, "");
                     if(Bambu::OpenMP::Task::Utils::Verbose)
                     {
                        a->print(errs());
                     }
                     Utils::log(errs(), 2, 1, "");
                  }
                  auto nextIt = it;
                  std::advance(nextIt, 1);
                  instructionsToAdd.push_back(std::make_tuple(&(*nextIt), dyn_cast<Function>(lambdaFn), args));

                  deleteList.push_back(callInst);
               }
               else if(callInst->getCalledFunction() &&
                       callInst->getCalledFunction()->getName().find("kmp") != std::string::npos)
               {
                  deleteList.push_back(callInst);
               }
            }
         }
      }
      for(auto& [insertPoint, called, args] : instructionsToAdd)
      {
         IRBuilder<> builder(insertPoint);
         builder.CreateCall(called, args);
      }

      for(auto& I : deleteList)
      {
         I->eraseFromParent();
      }
   }

   /// Instrument the IR. This call we simplify dummy fn calls that mimic the task
   /// graph calls by calling directly the underlying lambda
   /// @param F A @ref llvm::Function
   void simplifyInstrumentation(Function& F)
   {
      Utils::log(errs(), 0, 1, "Simplifyng task graph", F.getName());
      std::vector<std::tuple<Instruction*, Function*, std::vector<Value*>>> instructionsToAdd;
      std::vector<Instruction*> deleteList;
      for(BasicBlock& BB : F)
      {
         for(auto it = BB.begin(); it != BB.end(); ++it)
         {
            if(auto* callInst = dyn_cast<CallInst>(&(*it)))
            {
               if(callInst->getCalledFunction() == nullptr)
               {
                  continue;
               }
               if(callInst->getCalledFunction()->getName().find(Constants::injectedTaskNamePreifx) != std::string::npos)
               {
                  Utils::log(errs(), 2, 1, "got", callInst->getCalledFunction()->getName());
                  Value* lambdaFn = callInst->getArgOperand(1);
                  assert(dyn_cast<Function>(lambdaFn) && "expected a fn");
                  Utils::log(errs(), 2, 1, "called lambda is", lambdaFn->getName());
                  const unsigned totParams = callInst->getNumOperands() - 3;
                  std::vector<Value*> args;
                  for(unsigned i = 0; i < totParams; i++)
                  {
                     args.push_back(callInst->getOperand(2 + i));
                  }
                  Utils::log(errs(), 2, 1, "args:");
                  for(auto& a : args)
                  {
                     Utils::log(errs(), 2, 0, "");
                     if(Bambu::OpenMP::Task::Utils::Verbose)
                     {
                        a->print(errs());
                     }
                     Utils::log(errs(), 2, 1, "");
                  }
                  auto nextIt = it;
                  std::advance(nextIt, 1);
                  instructionsToAdd.push_back(std::make_tuple(&(*nextIt), dyn_cast<Function>(lambdaFn), args));

                  deleteList.push_back(callInst);
               }
            }
         }
      }
#ifndef REMOVE_TASK_GRAPH_FN_CONTENT
      for(auto& [insertPoint, called, args] : instructionsToAdd)
      {
         IRBuilder<> builder(insertPoint);
         builder.CreateCall(called, args);
      }
#else
      // Prevent inlining
      F.addFnAttr(Attribute::NoInline);

      // Prevent optimizations
      F.addFnAttr(Attribute::OptimizeNone);
#endif

      for(auto& I : deleteList)
      {
         I->eraseFromParent();
      }
   }

   void cleanOmpOutlined(Function* F)
   {
      Utils::log(errs(), 0, 1, "Cleaning omp outlined", F->getName());
      Module* M = F->getParent();
      LLVMContext& Context = M->getContext();
      constexpr unsigned startIndex = 2;

      // Creation of transformed function
      std::vector<Type*> argTypes;
      unsigned idx = 0;
      for(auto& A : F->args())
      {
         if(idx < startIndex)
         {
            idx++;
            continue;
         }
         argTypes.push_back(A.getType());
      }

      FunctionType* NewFuncType = FunctionType::get(F->getReturnType(), argTypes, F->isVarArg());
      Function* newFunction = Function::Create(NewFuncType, F->getLinkage(), Constants::taskGraphName, M);

      ValueToValueMapTy vMap;
      auto it = newFunction->arg_begin();
      idx = 0;
      for(auto& oldArg : F->args())
      {
         // Unused args have to be declared as undef values for CloneFunctionInto
         if(idx < startIndex)
         {
            vMap[&oldArg] = UndefValue::get(oldArg.getType());
            idx++;
            continue;
         }
         vMap[&oldArg] = &*it;
         it->setName(oldArg.getName());
         ++it;
      }
      SmallVector<ReturnInst*> returns;
      CloneFunctionInto(newFunction, F, vMap, CloneFunctionChangeType::LocalChangesOnly, returns);

      F->replaceAllUsesWith(newFunction);
      F->eraseFromParent();
   }

   struct DeadArgAndSimplifyPass : PassInfoMixin<DeadArgAndSimplifyPass>
   {
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM)
      {
         if(NamedMDNode* NMD = M.getNamedMetadata(Constants::llvmIndentName))
         {
            for(unsigned i = 0, e = NMD->getNumOperands(); i < e; ++i)
            {
               MDNode* Node = NMD->getOperand(i);
               if(auto* MS = dyn_cast<MDString>(Node->getOperand(0)))
               {
                  const auto versionStr = MS->getString();
                  const auto pos = versionStr.find(Constants::llvmVersionPrefix);
                  assert(pos != std::string::npos);
                  const auto version = versionStr.substr(pos + Constants::llvmVersionPrefix.size(), 2);
                  assert(version == Constants::supportedLlvmVersion &&
                         "IR input generated from a not supported clang version");
               }
            }
         }
         else
         {
            assert(false && "Can not find clang version in metadata");
         }
         // Cache the Function reference for recursive usage later inside the
         // visitor. Some visitor fn may access some Function which has not
         // been visited yet in the pass.
         std::vector<Function*> taskFunctions;
         std::vector<Function*> outlinedFunctions;
         for(Function& F : M)
         {
            if(F.isDeclaration())
            {
               continue;
            }
            const auto fnName = F.getName().str();

            if(fnName.find(Constants::targetTaskEntryNamePrefix) != std::string::npos &&
               fnName.find(Constants::targetTransformedTaskEntryNameSuffix) != std::string::npos)
            {
               taskFunctions.push_back(&F);
            }
            if(fnName.find(Constants::targetOmpOutlinedFnNamePrefix) != std::string::npos &&
               fnName.find(Constants::bambuHostReservedFnName) == std::string::npos)
            {
               outlinedFunctions.push_back(&F);
            }
         }

         for(Function* F : taskFunctions)
         {
            cleanTaskEntry(F);
         }
         for(Function* F : outlinedFunctions)
         {
            cleanOmpOutlined(F);
         }

         for(Function& F : M)
         {
            if(F.isDeclaration())
            {
               continue;
            }
            if(F.getName().find(Constants::taskGraphName) != std::string::npos)
            {
               simplifyInstrumentation(F);
            }
            else
            {
               simplifyForkCall(F);
            }
         }

         return PreservedAnalyses::none();
      }

      static bool isRequired()
      {
         return true;
      }
   };

#else

   struct DeadArgAndSimplifyPass : PassInfoMixin<DeadArgAndSimplifyPass>
   {
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& MAM)
      {
         return PreservedAnalyses::all();
      }

      static bool isRequired()
      {
         return true;
      }
   };

#endif
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
PassPluginLibraryInfo getDeadArgPluginInfo()
{
   return {LLVM_PLUGIN_API_VERSION, "DeadArgAndSimplifyPass", LLVM_VERSION_STRING, [](PassBuilder& PB) {
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, ModulePassManager& MPM, ArrayRef<PassBuilder::PipelineElement>) {
                     if(Name == "openmpTaskdeadArgAndOutlinedSimplify")
                     {
                        MPM.addPass(DeadArgAndSimplifyPass());
                        return true;
                     }
                     return false;
                  });
           }};
}

// This is the core interface for pass plugins. It guarantees that 'opt'
// will be able to recognize DeadArgAndSimplifyPass when added to the pass
// pipeline on the command line, i.e. via '-passes=fn-pass-and-analysis'
extern "C" PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK __attribute__((visibility("default"))) llvmGetPassPluginInfo()
{
   return getDeadArgPluginInfo();
}

#else

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/IR/LegacyPassManager.h>

namespace
{
   struct DeadArgAndSimplifyPass : public llvm::ModulePass
   {
      char ID = 0;
      DeadArgAndSimplifyPass() : llvm::ModulePass(ID)
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
   PM.add(new DeadArgAndSimplifyPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses DeadArgAndSimplifyPass_Ox(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
                                                              loadPass);
#endif

#endif
