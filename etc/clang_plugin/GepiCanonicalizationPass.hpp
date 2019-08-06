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
//
// Created by Marco Siracusa on 7/27/19.
//

#ifndef SCALAR_REPLACEMENT_OF_AGGREGATES_GEPICANONICALIZATIONPASS_HPP
#define SCALAR_REPLACEMENT_OF_AGGREGATES_GEPICANONICALIZATIONPASS_HPP

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

class GepiCanonicalizationPass : public llvm::FunctionPass {
public:
    char ID = 0;

public:
  explicit GepiCanonicalizationPass(char& _ID) : llvm::FunctionPass(_ID){}
  explicit GepiCanonicalizationPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &function) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
     AU.setPreservesCFG();
  }
};

llvm::Pass *createGepiCanonicalizationPass();


#endif //SCALAR_REPLACEMENT_OF_AGGREGATES_GEPICANONICALIZATIONPASS_HPP
