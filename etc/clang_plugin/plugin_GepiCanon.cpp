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
 *              Copyright (C) 2019 Politecnico di Milano
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
   struct CLANG_VERSION_SYMBOL(_plugin_GepiCanon);
}

namespace llvm
{
   struct CLANG_VERSION_SYMBOL(_plugin_GepiCanon) : public GepiCanonicalizationPass
   {
    public:
      static char ID;
      CLANG_VERSION_SYMBOL(_plugin_GepiCanon)() : GepiCanonicalizationPass(ID)
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
         return CLANG_VERSION_STRING(_plugin_GepiCanon);
      }
   };

   char CLANG_VERSION_SYMBOL(_plugin_GepiCanon)::ID = 0;

} // namespace llvm

#ifndef _WIN32
static llvm::RegisterPass<llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon)> XPassFV(CLANG_VERSION_STRING(_plugin_GepiCanon), "GepiCanonicalizationPass", true /* Only looks at CFG */, false /* Analysis Pass */);

#endif

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase& PM)
{
   PM.add(llvm::createPromoteMemoryToRegisterPass());
   PM.add(new llvm::CLANG_VERSION_SYMBOL(_plugin_GepiCanon));
   PM.add(llvm::createVerifierPass());
}

#if ADD_RSP
// These constructors add our pass to a list of global extensions.
static llvm::RegisterStandardPasses CLANG_VERSION_SYMBOL(_plugin_GepiCanon_Ox)(llvm::PassManagerBuilder::EP_ModuleOptimizerEarly, loadPass);

#endif

#ifdef _WIN32
using namespace llvm;

INITIALIZE_PASS_BEGIN(clang7_plugin_GepiCanon, "clang7_plugin_GepiCanonFV", "GepiCanonicalizationPass", true, false)
INITIALIZE_PASS_END(clang7_plugin_GepiCanon, "clang7_plugin_GepiCanon", "GepiCanonicalizationPass", true, false)
namespace llvm
{
   void clang7_plugin_GepiCanon_init()
   {
   }
} // namespace llvm
#endif
