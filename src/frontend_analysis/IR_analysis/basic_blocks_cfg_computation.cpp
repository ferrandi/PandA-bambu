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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file basic_blocks_cfg_computation.cpp
 * @brief Build basic block control flow graph data structure starting from the ir_manager.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "basic_blocks_cfg_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "op_graph.hpp"
#include "operations_cfg_computation.hpp"
#include "string_manipulation.hpp"

#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp"
#endif

#include <fstream>
#include <list>

BasicBlocksCfgComputation::BasicBlocksCfgComputation(const ParameterConstRef _parameters,
                                                     const application_managerRef _AppM, unsigned int _function_id,
                                                     const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BASIC_BLOCKS_CFG_COMPUTATION, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
BasicBlocksCfgComputation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, SAME_FUNCTION));
         relationships.insert(std::make_pair(VAR_DECL_FIX, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(SELECT_TREE_BALANCING, SAME_FUNCTION));
         relationships.insert(std::make_pair(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FANOUT_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_OPT, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void BasicBlocksCfgComputation::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      function_behavior->bbgc->Clear();
#if HAVE_HOST_PROFILING_BUILT
      function_behavior->profiling_information->Clear();
#endif
   }
}

DesignFlowStep_Status BasicBlocksCfgComputation::InternalExec()
{
   const auto TM = AppM->get_ir_manager();
   const auto& bbgc = function_behavior->bbgc;
   const auto fd = GetPointer<const function_val_node>(TM->GetIRNode(function_id));
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto sl = GetPointer<const statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   for(const auto& id_bb : sl->list_of_bloc)
   {
      const auto& bb = id_bb.second;
      if(bb->number != BB_ENTRY && bb->number != BB_EXIT)
      {
         continue;
      }
      bbgc->add_vertex(bb);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added basic block with index " + STR(bb->number));
      if(bb->number == BB_EXIT)
      {
         const auto exit = bbgc->Cget_vertex(BB_EXIT);
         bbgc->connect_to_entry(exit);
      }
   }
   for(const auto& id_bb : sl->list_of_bloc)
   {
      const auto& bb = id_bb.second;
      if(bb->number == BB_ENTRY || bb->number == BB_EXIT)
      {
         continue;
      }
      bbgc->add_vertex(bb);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Added basic block with index " + STR(bb->number));
   }
   for(const auto& id_bb : sl->list_of_bloc)
   {
      const auto& bb = id_bb.second;
      if(bb->number == BB_ENTRY || bb->number == BB_EXIT)
      {
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering connections for BB" + STR(bb->number));
      const auto current = bbgc->Cget_vertex(bb->number);
      if(bb->list_of_pred.empty() || std::count(bb->list_of_pred.begin(), bb->list_of_pred.end(), bloc::ENTRY_BLOCK_ID))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Connecting Basic block " + STR(bb->number) + " to ENTRY");
         bbgc->connect_to_entry(current);
      }
      for(const auto su : bb->list_of_succ)
      {
         if(su == bloc::EXIT_BLOCK_ID)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Connecting Basic block " + STR(bb->number) + " to EXIT");
            bbgc->connect_to_exit(current);
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---Connecting Basic block " + STR(bb->number) + " to " + STR(su));
            bbgc->AddEdge(current, bbgc->Cget_vertex(su), CFG_SELECTOR);
         }
      }
      const auto& statements = bb->CGetStmtList();
      if(!statements.empty())
      {
         const auto& last = statements.back();
         /// multi-way if
         if(last->get_kind() == multi_way_if_stmt_K)
         {
            const auto gmwi = GetPointerS<const multi_way_if_stmt>(last);
            for(const auto& cond : gmwi->list_of_cond)
            {
               bbgc->add_bb_edge_info(current, bbgc->Cget_vertex(cond.second), CFG_SELECTOR,
                                      cond.first ? cond.first->index : default_COND);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered connections for BB" + STR(bb->number));
   }
   const auto exit = bbgc->Cget_vertex(BB_EXIT);
   const auto fcfg = function_behavior->GetBBGraph(FunctionBehavior::FBB);
   for(const auto& v : fcfg.vertices())
   {
      if(fcfg.out_degree(v) == 0 && v != exit)
      {
         bbgc->AddEdge(v, exit, CFG_SELECTOR);
      }
   }
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      function_behavior->GetBBGraph(FunctionBehavior::BB).writeDot(function_behavior->GetDotPath() / "BB_CFG.dot");
   }
   return DesignFlowStep_Status::SUCCESS;
}
