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
 *              Copyright (C) 2019-2020 Politecnico di Milano
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
#include "plugin_includes.hpp"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#if __clang_major__ >= 7
#include "llvm/Transforms/Utils.h"
#endif
#include "GepiCanonicalizationPass.hpp"

namespace llvm
{
   template <int OPT_SELECTION>
   struct CLANG_VERSION_SYMBOL(_plugin_GepiCanon);
}

namespace llvm
{
   template <int OPT_SELECTION>
   struct CLANG_VERSION_SYMBOL(_plugin_GepiCanon) : public GepiCanonicalizationPass
   {
    public:
      static char ID;
      CLANG_VERSION_SYMBOL(_plugin_GepiCanon)() : GepiCanonicalizationPass(ID, OPT_SELECTION)
      {
         initializeTargetTransformInfoWrapperPassPass(*PassRegistry::getPassRegistry());
         initializeLoopPassPass(*PassRegistry::getPassRegistry());
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
         else if(OPT_SELECTION == SROA_removeLifetime)
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
   };

   template <int OPT_SELECTION>
   char CLANG_VERSION_SYMBOL(_plugin_GepiCanon)<OPT_SELECTION>::ID = 0;

} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_ptrIteratorSimplification>> XPassPS(CLANG_VERSION_STRING(_plugin_GepiCanon) "PS", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_chunkOperationsLowering>> XPassCOL(CLANG_VERSION_STRING(_plugin_GepiCanon) "COL", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_bitcastVectorRemoval>> XPassBVR(CLANG_VERSION_STRING(_plugin_GepiCanon) "BVR", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_removeLifetime>> XPassLTR(CLANG_VERSION_STRING(_plugin_GepiCanon) "LTR", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_selectLowering>> XPassSL(CLANG_VERSION_STRING(_plugin_GepiCanon) "SL", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_canonicalIdxs>> XPassCIDX(CLANG_VERSION_STRING(_plugin_GepiCanon) "CIDX", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_cleanLCSSA>> XPassCLCSSA(CLANG_VERSION_STRING(_plugin_GepiCanon) "CLCSSA", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_gepiExplicitation>> XPassGEXP(CLANG_VERSION_STRING(_plugin_GepiCanon) "GEXP", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_codeSimplification>> XPassCSIMPL(CLANG_VERSION_STRING(_plugin_GepiCanon) "CSIMPL", "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);
#endif

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon) < SROA_removeLifetime >);
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

#if ADD_RSP
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_GepiCanon_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

#endif

#ifdef _WIN32
using namespace llvm;

INITIALIZE_PASS_BEGIN(clang7_plugin_GepiCanon<SROA_ptrIteratorSimplification>, "clang7_plugin_GepiCanonPS", "GepiCanonicalizationPass", true, false)
INITIALIZE_PASS_END(clang7_plugin_GepiCanon<SROA_ptrIteratorSimplification>, "clang7_plugin_GepiCanonPS", "GepiCanonicalizationPass", true, false)
namespace llvm
{
   void clang7_plugin_GepiCanon_init()
   {
   }
} // namespace llvm
#endif
