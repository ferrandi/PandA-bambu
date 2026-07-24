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
#ifndef BAMBU_POINTER_RESOLUTION_PASS_HPP
#define BAMBU_POINTER_RESOLUTION_PASS_HPP

#include "panda_clang_compat.hpp"
#include "llvm/IR/PassManager.h"
#include <llvm/ADT/StringRef.h>
#include <llvm/Pass.h>
#include <string>
#include <utility>

namespace llvm
{
   struct PointerResolutionPass : public ModulePass
#if LLVM_VERSION_MAJOR >= 13
       ,
                                  public PassInfoMixin<PointerResolutionPass>
#endif
   {
    public:
      static char ID;
      std::string outdirNameCmd;

      PointerResolutionPass(std::string outdirNameCmd) : ModulePass(ID), outdirNameCmd(std::move(outdirNameCmd))
      {
      }

#if LLVM_VERSION_MAJOR >= 13
      PointerResolutionPass(const PointerResolutionPass& other) : PointerResolutionPass(other.outdirNameCmd)
      {
      }
#endif

      bool exec(Module& M);
      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);
      bool runOnModule(Module& M) override;
      StringRef getPassName() const override;
      void getAnalysisUsage(AnalysisUsage& AU) const override;
   };

} // end namespace llvm

#endif // BAMBU_POINTER_RESOLUTION_PASS_HPP
