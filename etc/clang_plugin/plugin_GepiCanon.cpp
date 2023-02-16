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
 *              Copyright (C) 2019-2023 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*
 * Clang plugin for GEPI canonicalization
 *
 * @author Fabrizio Ferrandi <Fabrizio.ferrandi@polimi.it>
 *
 */
#include "GepiCanonicalizationPass.hpp"
#include "plugin_includes.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>

#if __clang_major__ >= 7 && !defined(VVD)
#include <llvm/Transforms/Utils.h>
#endif
#if __clang_major__ >= 10
#include <llvm/InitializePasses.h>
#endif
#if __clang_major__ >= 13
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#endif

namespace llvm
{
   template <int OPT_SELECTION>
   struct CLANG_VERSION_SYMBOL(_plugin_GepiCanon);
}

namespace llvm
{
   template <int OPT_SELECTION>
   struct CLANG_VERSION_SYMBOL(_plugin_GepiCanon)
       : public GepiCanonicalizationPass
#if __clang_major__ >= 13
         ,
         public PassInfoMixin<CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < OPT_SELECTION> >
#endif
   {
    public:
      static char ID;

      CLANG_VERSION_SYMBOL(_plugin_GepiCanon)() : GepiCanonicalizationPass(ID, OPT_SELECTION)
      {
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
      }

      CLANG_VERSION_SYMBOL(_plugin_GepiCanon)
      (const CLANG_VERSION_SYMBOL(_plugin_GepiCanon) &) : CLANG_VERSION_SYMBOL(_plugin_GepiCanon)()
      {
      }

      bool runOnFunction(llvm::Function& function) override
      {
         return GepiCanonicalizationPass::runOnFunction(function);
      }

      StringRef getPassName() const override
      {
         if(OPT_SELECTION == SROA_ptrIteratorSimplification)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "PS";
         else if(OPT_SELECTION == SROA_chunkOperationsLowering)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "COL";
         else if(OPT_SELECTION == SROA_bitcastVectorRemoval)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "BVR";
         else if(OPT_SELECTION == SROA_intrinsic)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "LTR";
         else if(OPT_SELECTION == SROA_selectLowering)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "SL";
         else if(OPT_SELECTION == SROA_canonicalIdxs)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "CIDX";
         else if(OPT_SELECTION == SROA_cleanLCSSA)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "CLCSSA";
         else if(OPT_SELECTION == SROA_gepiExplicitation)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "GEXP";
         else if(OPT_SELECTION == SROA_codeSimplification)
            return CLANG_VERSION_STRING(_plugin_GepiCanon) "CSIMPL";
      }

#if __clang_major__ >= 13
      llvm::PreservedAnalyses run(llvm::Function& Func, llvm::FunctionAnalysisManager& FAM)
      {
         const auto changed = runOnFunction(Func);
         return (changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all());
      }
#endif
   };

   template <int OPT_SELECTION>
   char CLANG_VERSION_SYMBOL(_plugin_GepiCanon)<OPT_SELECTION>::ID = 0;

} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification> >
    XPassPS(CLANG_VERSION_STRING(_plugin_GepiCanonPS), "GepiCanonicalizationPass", true /* Only looks at CFG */,
            false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_chunkOperationsLowering> >
    XPassCOL(CLANG_VERSION_STRING(_plugin_GepiCanonCOL), "GepiCanonicalizationPass", true /* Only looks at CFG */,
             false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_bitcastVectorRemoval> >
    XPassBVR(CLANG_VERSION_STRING(_plugin_GepiCanonBVR), "GepiCanonicalizationPass", true /* Only looks at CFG */,
             false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_intrinsic> >
    XPassLTR(CLANG_VERSION_STRING(_plugin_GepiCanonLTR), "GepiCanonicalizationPass", true /* Only looks at CFG */,
             false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_selectLowering> >
    XPassSL(CLANG_VERSION_STRING(_plugin_GepiCanonSL), "GepiCanonicalizationPass", true /* Only looks at CFG */,
            false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs> >
    XPassCIDX(CLANG_VERSION_STRING(_plugin_GepiCanonCIDX), "GepiCanonicalizationPass", true /* Only looks at CFG */,
              false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_cleanLCSSA> >
    XPassCLCSSA(CLANG_VERSION_STRING(_plugin_GepiCanonCLCSSA), "GepiCanonicalizationPass", true /* Only looks at CFG */,
                false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation> >
    XPassGEXP(CLANG_VERSION_STRING(_plugin_GepiCanonGEXP), "GepiCanonicalizationPass", true /* Only looks at CFG */,
              false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_codeSimplification> >
    XPassCSIMPL(CLANG_VERSION_STRING(_plugin_GepiCanonCSIMPL), "GepiCanonicalizationPass", true /* Only looks at CFG */,
                false /* Analysis Pass */);
#endif

#if __clang_major__ >= 13
llvm::PassPluginLibraryInfo CLANG_PLUGIN_INFO(_plugin_GepiCanon)()
{
   return {LLVM_PLUGIN_API_VERSION, CLANG_VERSION_STRING(_plugin_GepiCanon), "v0.12", [](llvm::PassBuilder& PB) {
              PB.registerPipelineParsingCallback([](llvm::StringRef Name, llvm::ModulePassManager& MPM,
                                                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                 llvm::FunctionPassManager FPM;
                 if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonPS))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonCOL))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_chunkOperationsLowering > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonBVR))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_bitcastVectorRemoval > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonLTR))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_intrinsic > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonSL))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_selectLowering > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonCIDX))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonCLCSSA))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_cleanLCSSA > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonGEXP))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation > ());
                 }
                 else if(Name == CLANG_VERSION_STRING(_plugin_GepiCanonCSIMPL))
                 {
                    FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_codeSimplification > ());
                 }
                 else
                 {
                    return false;
                 }
                 FPM.addPass(llvm::VerifierPass());
                 MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
                 return true;
              });
              PB.registerPipelineEarlySimplificationEPCallback(
                  [](llvm::ModulePassManager& MPM, llvm::PassBuilder::OptimizationLevel) {
                     llvm::FunctionPassManager FPM;
                     FPM.addPass(llvm::PromotePass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_intrinsic > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_chunkOperationsLowering > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_bitcastVectorRemoval > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_selectLowering > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs > ());
                     FPM.addPass(llvm::VerifierPass());
                     FPM.addPass(llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation > ());
                     FPM.addPass(llvm::VerifierPass());
                     MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
                     return true;
                  });
           }};
}

// This part is the new way of registering your pass
extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo()
{
   return CLANG_PLUGIN_INFO(_plugin_GepiCanon)();
}
#else
#if ADD_RSP
// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_intrinsic >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_chunkOperationsLowering >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_bitcastVectorRemoval >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_selectLowering >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs >);
   PM.add(llvm::createVerifierPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation >);
   PM.add(llvm::createVerifierPass());
}

// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses
    CLANG_VERSION_SYMBOL(_plugin_GepiCanon_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);
#endif
#endif

// using namespace llvm;
//
// namespace llvm
// {
//    void CLANG_PLUGIN_INIT(_plugin_GepiCanon)(PassRegistry&);
// } // namespace llvm
//
// INITIALIZE_PASS_BEGIN(CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification >,
//                       CLANG_VERSION_STRING(_plugin_GepiCanon) "PS", "GepiCanonicalizationPass", true, false)
// INITIALIZE_PASS_END(CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification >,
//                     CLANG_VERSION_STRING(_plugin_GepiCanon) "PS", "GepiCanonicalizationPass", true, false)
