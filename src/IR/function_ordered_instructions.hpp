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
 *   Copyright (C) 2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
