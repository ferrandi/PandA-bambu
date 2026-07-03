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
 * @file function_ordered_instructions.cpp
 * @brief Build a per-function dominator tree and expose instruction ordering on top of it.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "function_ordered_instructions.hpp"

#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "exceptions.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"

namespace
{
   const std::map<unsigned int, blocRef>& getFunctionBlocks(const application_managerRef& AppM, unsigned function_id)
   {
      const auto function_node = GetPointerS<const function_val_node>(AppM->get_ir_manager()->GetIRNode(function_id));
      THROW_ASSERT(function_node->body, "");
      return GetPointerS<const statement_list_node>(function_node->body)->list_of_bloc;
   }
} // namespace

FunctionOrderedInstructions::FunctionOrderedInstructions(const application_managerRef& AppM, unsigned function_id)
    : bbgc(BBGraphInfo(AppM, function_id)),
      dt(buildDominatorTree(bbgc, getFunctionBlocks(AppM, function_id))),
      ordered_instructions(dt)
{
}

const BBGraph& FunctionOrderedInstructions::getDT() const
{
   return dt;
}

const OrderedInstructions& FunctionOrderedInstructions::getOrderedInstructions() const
{
   return ordered_instructions;
}

bool FunctionOrderedInstructions::dominates(const ir_nodeConstRef& A, const ir_nodeConstRef& B) const
{
   return ordered_instructions.dominates(A, B);
}

BBGraph FunctionOrderedInstructions::buildDominatorTree(BBGraphsCollection& bbgc,
                                                        const std::map<unsigned int, blocRef>& list_of_bloc)
{
   BBGraph DT(bbgc, D_SELECTOR);
   auto& inverse_vertex_map = DT.GetGraphInfo().bb_index_map;
   for(const auto& [bbi, bb] : list_of_bloc)
   {
      inverse_vertex_map.try_emplace(bbi, bbgc.AddVertex(BBNodeInfo(bb)));
   }
   THROW_ASSERT(list_of_bloc.size() == inverse_vertex_map.size(), "");

   THROW_ASSERT(inverse_vertex_map.count(bloc::ENTRY_BLOCK_ID), "Entry BB does not exist");
   THROW_ASSERT(inverse_vertex_map.count(bloc::EXIT_BLOCK_ID), "Exit BB does not exist");
   const auto entry_v = inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID);
   const auto exit_v = inverse_vertex_map.at(bloc::EXIT_BLOCK_ID);

   for(const auto& [bbi, bb] : list_of_bloc)
   {
      const auto bbv = inverse_vertex_map.at(bbi);
      for(const auto& lop : bb->list_of_pred)
      {
         THROW_ASSERT(inverse_vertex_map.count(lop),
                      "BB" + STR(lop) + " (successor of BB" + STR(bbi) + ") does not exist");
         bbgc.AddEdge(inverse_vertex_map.at(lop), bbv, CFG_SELECTOR);
      }

      for(const auto& los : bb->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bbgc.AddEdge(bbv, exit_v, CFG_SELECTOR);
         }
      }

      if(bb->list_of_succ.empty())
      {
         bbgc.AddEdge(bbv, exit_v, CFG_SELECTOR);
      }
   }

   bbgc.AddEdge(entry_v, exit_v, CFG_SELECTOR);

   BBGraph cfg(bbgc, CFG_SELECTOR);
   dominance<BBGraph> bb_dominators(cfg, entry_v, exit_v);
   bb_dominators.forEachDominanceRelation(
       [&](const BBGraph::vertex_descriptor child, const BBGraph::vertex_descriptor dom_vertex) {
          if(child != entry_v)
          {
             bbgc.AddEdge(dom_vertex, child, D_SELECTOR);
          }
       });
   return DT;
}
