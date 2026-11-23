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
/**
 * @file DFConstArgPass.hpp
 * @brief Propagation of constant arguments into the dataflow modules.
 *
 * @author Francesco Bazzano <francesco.bazzano@mail.polimi.it>
 *
 */
#ifndef BAMBU_DF_CONST_ARG_PASS_HPP
#define BAMBU_DF_CONST_ARG_PASS_HPP

#include "llvm/Config/llvm-config.h"

#if LLVM_VERSION_MAJOR >= 13
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include <string>

namespace llvm
{
   /// topfname treats every dataflow top and module as a root: noinline, external linkage, not dso_local and a
   /// preserved symbol. LLVM then has to assume callers it cannot see, so IPSCCP and DeadArgumentElimination leave
   /// their parameters alone, and a constant that is always passed in stays behind the parameter. In hls4ml this
   /// is the case of the weights and biases: every layer gets its w/b arrays as pointers, so the kernels read each
   /// weight from memory and multiply by a variable.
   /// This pass carries across that boundary only scalar constants and read-only globals, when every call passes
   /// the same one and the module is only called directly. The parameter is removed: the module is rebuilt under
   /// the same name without it and its calls are rewritten. Leaving it in the signature would leave its bundle in
   /// the interface too, and two instances of the same module template (two hls4ml layers) would share a
   /// "weights" bundle with different data widths. Anything that is written (channels, state, this) keeps going
   /// through the parameter.
   /// architecture.xml is not rewritten here: the remaining arguments keep their bambu.orig_index, which ArrPart
   /// uses to find their entries, and the IR dump drops the entries and bundles of the removed ones.
   struct DFConstArgPass : public PassInfoMixin<DFConstArgPass>
   {
      /// topFnNames: comma separated top functions, which are never rewritten.
      /// outdirName: directory holding the architecture.xml that lists the dataflow modules.
      DFConstArgPass(std::string topFnNames, std::string outdirName)
          : topFnNames(std::move(topFnNames)), outdirName(std::move(outdirName))
      {
      }

      PreservedAnalyses run(Module& M, ModuleAnalysisManager& AM);

    private:
      std::string topFnNames;
      std::string outdirName;

      bool isTop(const Function& F) const;
   };
} // namespace llvm
#endif

#endif
