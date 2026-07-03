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
 * @file NI_SSA_liveness.cpp
 * @brief Non-Iterative liveness analysis for SSA based descriptions.
 *
 * Compute the liveness sets by exploring paths from variable use (Algorithm 4 and 5).
 * Details of the algorithm can be found in the following technical report:
 * - Florian Brandner, Benoit Boissinot, Alain Darte, Benoît Dupont de Dinechin, Fabrice Rastello
 *   "Computing Liveness Sets for SSA-Form Programs",
 *    inria-00558509, version 2
 *
 *
 *   Research report reference:
 *   Florian Brandner, Benoit Boissinot, Alain Darte, Benoit Dupont de Dinechin, Fabrice Rastello,
 *   "Computing Liveness Sets for SSA-Form Programs", INRIA RR-7503, April 2011.
 *   Available from HAL as inria-00558509: http://hal.inria.fr/inria-00558509
 *
 *   Abstract:
 *   We revisit the problem of computing liveness sets, that is, the variables live-in and live-out of
 *   basic blocks, for programs in strict SSA form. Strict SSA ensures that each definition dominates
 *   all its uses, and this property can be exploited to optimize liveness computation. The report
 *   introduces a fast data-flow algorithm that avoids iterative fixed-point computation by using two
 *   passes: one backward traversal of the control-flow graph in postorder, followed by one traversal
 *   of the loop-nesting forest to update loop-local information. It also revisits path-exploration
 *   algorithms that propagate information from uses to definitions one variable at a time, and shows
 *   how to extend them with data structures suited to SSA-based liveness analysis. Experimental
 *   results on SPECINT 2000 show that the standard iterative data-flow approach is clearly
 *   outperformed, while the loop-forest-based algorithm performs best on optimized programs when
 *   sets are represented as bitsets.
 *   }
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "NI_SSA_liveness.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "string_manipulation.hpp"

#include <fstream>

NI_SSA_liveness::NI_SSA_liveness(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                 unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, NI_SSA_LIVENESS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
NI_SSA_liveness::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, SAME_FUNCTION));
         relationships.insert(std::make_pair(VAR_DECL_FIX, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
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
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void NI_SSA_liveness::Up_and_Mark(blocRef B, ir_nodeRef v, statement_list_node* sl)
{
   /// if def(v) ∈ B (φ excluded) then return > Killed in the block, stop
   auto* v_ssa_name = GetPointer<ssa_node>(v);
   if(!v_ssa_name)
   {
      return;
   }
   unsigned int def_stmt = v_ssa_name->GetDefStmt()->index;
   if(v_ssa_name->GetDefStmt()->get_kind() == nop_stmt_K && v_ssa_name->var->get_kind() == argument_val_node_K)
   {
      return;
   }

   for(const auto& stmt : B->CGetStmtList())
   {
      if(def_stmt == stmt->index)
      {
         return;
      }
   }
   /// if v ∈ LiveIn(B) then return >    Propagation already done, stop
   unsigned int v_index = v->index;
   if(B->live_in.find(v_index) != B->live_in.end())
   {
      return;
   }
   /// LiveIn(B) = LiveIn(B) ∪ {v}
   B->live_in.insert(v_index);
   /// if v ∈ PhiDefs(B) then return >   Do not propagate φ definitions
   for(const auto& phi : B->CGetPhiList())
   {
      auto* pn = GetPointer<phi_stmt>(phi);
      if(pn->res->index == v_index)
      {
         return;
      }
   }
   /// for each P ∈ CFG_preds(B) do >   Propagate backward
   auto lp_it_end = B->list_of_pred.end();
   for(auto lp_it = B->list_of_pred.begin(); lp_it != lp_it_end; ++lp_it)
   {
      const blocRef P = sl->list_of_bloc[*lp_it];
      P->live_out.insert(v_index);
      Up_and_Mark(P, v, sl);
   }
}

DesignFlowStep_Status NI_SSA_liveness::InternalExec()
{
   const ir_managerRef TM = AppM->get_ir_manager();
   ir_nodeRef tn = TM->GetIRNode(function_id);
   auto* fd = GetPointer<function_val_node>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointer<statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   auto B_it_end = sl->list_of_bloc.end();
   /// for each basic block B in CFG do > Consider all blocks successively
   for(auto B_it = sl->list_of_bloc.begin(); B_it != B_it_end; ++B_it)
   {
      blocRef B = B_it->second;
      unsigned int B_id = B->number;
      /// for each v ∈ PhiUses(B) do > Used in the φ of a successor block
      auto ls_it_end = B->list_of_succ.end();
      for(auto ls_it = B->list_of_succ.begin(); ls_it != ls_it_end; ++ls_it)
      {
         const blocRef B_succ = sl->list_of_bloc[*ls_it];
         for(auto const& phi : B_succ->CGetPhiList())
         {
            auto* pn = GetPointer<phi_stmt>(phi);
            bool is_virtual = pn->virtual_flag;
            if(!is_virtual)
            {
               for(const auto& def_edge : pn->CGetDefEdgesList())
               {
                  if(def_edge.second == B_id)
                  {
                     /// in the original algorithm the live out has all the PhiUses of B, that is:
                     /// LiveOut(B) = LiveOut(B) ∪ {v}
                     B->live_out.insert(def_edge.first->index);
                     Up_and_Mark(B, def_edge.first, sl);
                  }
               }
            }
         }
      }

      CustomSet<ir_nodeRef> bb_ssa_uses;
      for(const auto& stmt : B->CGetStmtList())
      {
         const auto stmt_uses = ir_helper::ComputeSsaUses(stmt);
         for(const auto& stmt_use : stmt_uses)
         {
            if(!ir_helper::IsVirtual(stmt_use.first))
            {
               bb_ssa_uses.insert(stmt_use.first);
            }
         }
      }
      /// for each v used in B (φ excluded) do >       Traverse B to find all uses
      for(const auto& ssa_use : bb_ssa_uses)
      {
         Up_and_Mark(B, ssa_use, sl);
      }
   }

#ifndef NDEBUG
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      /// print the analysis result
      const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Liveness for function " + BH->GetFunctionName());

      for(auto B_it = sl->list_of_bloc.begin(); B_it != B_it_end; ++B_it)
      {
         blocRef B = B_it->second;
         auto li_it_end = B->live_in.end();
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Live In for BB" + STR(B->number) + ": ");
         for(auto li_it = B->live_in.begin(); li_it != li_it_end; ++li_it)
         {
            PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, BH->PrintVariable(*li_it) + " ");
         }
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "\n");
         auto lo_it_end = B->live_out.end();
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Live Out for BB" + STR(B->number) + ": ");
         for(auto lo_it = B->live_out.begin(); lo_it != lo_it_end; ++lo_it)
         {
            PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, BH->PrintVariable(*lo_it) + " ");
         }
         PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "\n");
      }
   }
#endif
   return DesignFlowStep_Status::SUCCESS;
}

void NI_SSA_liveness::Initialize()
{
   if(bb_version != 0 and bb_version != function_behavior->GetBBVersion())
   {
      const auto TM = AppM->get_ir_manager();
      auto tn = TM->GetIRNode(function_id);
      auto fd = GetPointer<function_val_node>(tn);
      THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
      auto sl = GetPointer<statement_list_node>(fd->body);
      for(const auto& block : sl->list_of_bloc)
      {
         block.second->live_in.clear();
         block.second->live_out.clear();
      }
   }
}
