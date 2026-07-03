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
 *                Copyright (C) 2026 Politecnico di Milano
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
 * @file function_ordered_instructions.hpp
 * @brief Build a per-function dominator tree and expose instruction ordering on top of it.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef FUNCTION_ORDERED_INSTRUCTIONS_HPP
#define FUNCTION_ORDERED_INSTRUCTIONS_HPP

#include "behavior/OrderedInstructions.hpp"
#include "behavior/application_manager.hpp"
#include "behavior/basic_block.hpp"

#include <map>

REF_FORWARD_DECL(bloc);
struct function_val_node;

class FunctionOrderedInstructions
{
   BBGraphsCollection bbgc;
   BBGraph dt;
   OrderedInstructions ordered_instructions;

   static BBGraph buildDominatorTree(BBGraphsCollection& bbgc, const std::map<unsigned int, blocRef>& list_of_bloc);

 public:
   FunctionOrderedInstructions(const application_managerRef& AppM, unsigned function_id);

   const BBGraph& getDT() const;

   const OrderedInstructions& getOrderedInstructions() const;

   bool dominates(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const;
};

#endif // FUNCTION_ORDERED_INSTRUCTIONS_HPP
