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
 *              Copyright (C) 2004-2020 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   The PandA framework is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
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
/**
 * @file NI_SSA_liveness.cpp
 * @brief Non-Iterative liveness analysis for SSA based gimple descriptions.
 *
 * Compute the liveness sets by exploring paths from variable use (Algorithm 4 and 5).
 * Details of the algorithm can be found in the following technical report:
 * - Florian Brandner, Benoit Boissinot, Alain Darte, Benoît Dupont de Dinechin, Fabrice Rastello
 *   "Computing Liveness Sets for SSA-Form Programs",
 *    inria-00558509, version 2
 *
 *
 *   @techreport{BRANDNER:2011:INRIA-00558509:2,
 *     hal_id = {inria-00558509},
 *     url = {http://hal.inria.fr/inria-00558509},
 *     title = {{Computing Liveness Sets for SSA-Form Programs}},
 *     author = {Brandner, Florian and Boissinot, Benoit and Darte, Alain and Dupont De Dinechin, Beno{\^\i}t and Rastello, Fabrice},
 *     abstract = {{We revisit the problem of computing liveness sets, i.e., the set of variables live-in and live-out of basic blocks, for programs in strict SSA (static single assignment). Strict SSA is also known as SSA with dominance property because
 * it ensures that the definition of a variable always dominates all its uses. This property can be exploited to optimize the computation of liveness sets. Our first contribution is the design of a fast data-flow algorithm, which, unlike traditional
 * approaches, avoids the iterative calculation of a fixed point. Thanks to the properties of strict SSA form and the use of a loop-nesting forest, we show that two passes are sufficient. A first pass, similar to the initialization of iterative data-flow
 * analysis, traverses the control-flow graph in postorder propagating liveness information backwards. A second pass then traverses the loop-nesting forest, updating liveness information within loops. Another approach is to propagate from uses to
 * definition, one variable and one path at a time, instead of unioning sets as in standard data-flow analysis. Such a path-exploration strategy was proposed by Appel in his ''Tiger book'' and is also used in the LLVM compiler. Our second contribution is
 * to show how to extend and optimize algorithms based on this idea to compute liveness sets one variable at a time using adequate data\~structures. Finally, we evaluate and compare the efficiency of the proposed algorithms using the SPECINT 2000 benchmark
 * suite. The standard data-flow approach is clearly outperformed, all algorithms show substantial speed-ups of a factor of 2 on average. Depending on the underlying set implementation either the path-exploration approach or the loop-forest-based approach
 * provides superior performance. Experiments show that our loop-forest-based algorithm provides superior performances (average speed-up of 43\% on the fastest alternative) when sets are represented as bitsets and for optimized programs, i.e., when there
 * are more variables and larger live-sets and live-ranges.}}, keywords = {Liveness Analysis; SSA form; Compilers}, language = {English}, affiliation = {COMPSYS - INRIA Grenoble Rh{\^o}ne-Alpes / LIP Laboratoire de l'Informatique du Parall{\'e}lisme ,
 * Kalray}, pages = {25}, type = {Research Report}, institution = {INRIA}, number = {RR-7503}, collaboration = {Kalray }, year = {2011}, month = Apr, pdf = {http://hal.inria.fr/inria-00558509/PDF/RR-7503.pdf},
 *   }
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "NI_SSA_liveness.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"

/// STD include
#include <fstream>

/// Tree include
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

NI_SSA_liveness::NI_SSA_liveness(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, NI_SSA_LIVENESS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

NI_SSA_liveness::~NI_SSA_liveness() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> NI_SSA_liveness::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COMPLETE_BB_GRAPH, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(USE_COUNTING, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(HDL_VAR_DECL_FIX, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(EXTRACT_PATTERNS, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(VECTORIZE, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(FANOUT_OPT, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(COND_EXPR_RESTRUCTURING, SAME_FUNCTION));
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(SIMPLE_CODE_MOTION, SAME_FUNCTION));
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

void NI_SSA_liveness::Up_and_Mark(blocRef B, tree_nodeRef v, statement_list* sl)
{
   /// if def(v) ∈ B (φ excluded) then return > Killed in the block, stop
   auto* v_ssa_name = GetPointer<ssa_name>(GET_NODE(v));
   if(!v_ssa_name)
      return;
   if(v_ssa_name->volatile_flag)
      return;
   THROW_ASSERT(v_ssa_name->CGetDefStmts().size() == 1, "SSA " + v_ssa_name->ToString() + " (" + STR(v_ssa_name->index) + ") is not in SSA form");
   unsigned int def_stmt = GET_INDEX_NODE(v_ssa_name->CGetDefStmt());
   if(((GET_NODE(v_ssa_name->CGetDefStmt()))->get_kind() == gimple_nop_K && GET_NODE(v_ssa_name->var)->get_kind() == parm_decl_K))
      return;

   for(const auto& stmt : B->CGetStmtList())
      if(def_stmt == GET_INDEX_NODE(stmt))
         return;
   /// if v ∈ LiveIn(B) then return >    Propagation already done, stop
   unsigned int v_index = GET_INDEX_NODE(v);
   if(B->live_in.find(v_index) != B->live_in.end())
      return;
   /// LiveIn(B) = LiveIn(B) ∪ {v}
   B->live_in.insert(v_index);
   /// if v ∈ PhiDefs(B) then return >   Do not propagate φ definitions
   for(const auto& phi : B->CGetPhiList())
   {
      auto* pn = GetPointer<gimple_phi>(GET_NODE(phi));
      if(GET_INDEX_NODE(pn->res) == v_index)
         return;
   }
   /// for each P ∈ CFG_preds(B) do >   Propagate backward
   std::vector<unsigned int>::const_iterator lp_it_end = B->list_of_pred.end();
   for(std::vector<unsigned int>::const_iterator lp_it = B->list_of_pred.begin(); lp_it != lp_it_end; ++lp_it)
   {
      const blocRef P = sl->list_of_bloc[*lp_it];
      P->live_out.insert(v_index);
      Up_and_Mark(P, v, sl);
   }
}

DesignFlowStep_Status NI_SSA_liveness::InternalExec()
{
   const tree_managerRef TM = AppM->get_tree_manager();
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   auto B_it_end = sl->list_of_bloc.end();
   /// for each basic block B in CFG do > Consider all blocks successively
   for(auto B_it = sl->list_of_bloc.begin(); B_it != B_it_end; ++B_it)
   {
      blocRef B = B_it->second;
      unsigned int B_id = B->number;
      /// for each v ∈ PhiUses(B) do > Used in the φ of a successor block
      std::vector<unsigned int>::const_iterator ls_it_end = B->list_of_succ.end();
      for(std::vector<unsigned int>::const_iterator ls_it = B->list_of_succ.begin(); ls_it != ls_it_end; ++ls_it)
      {
         const blocRef B_succ = sl->list_of_bloc[*ls_it];
         for(auto const& phi : B_succ->CGetPhiList())
         {
            auto* pn = GetPointer<gimple_phi>(GET_NODE(phi));
            bool is_virtual = pn->virtual_flag;
            if(!is_virtual)
            {
               for(const auto& def_edge : pn->CGetDefEdgesList())
               {
                  if(def_edge.second == B_id)
                  {
                     /// in the original algorithm the live out has all the PhiUses of B, that is:
                     /// LiveOut(B) = LiveOut(B) ∪ {v}
                     B->live_out.insert(GET_INDEX_NODE(def_edge.first));
                     Up_and_Mark(B, def_edge.first, sl);
                  }
               }
            }
         }
      }

      CustomSet<tree_nodeRef> bb_ssa_uses;
      for(const auto& stmt : B->CGetStmtList())
      {
         const auto stmt_uses = tree_helper::ComputeSsaUses(stmt);
         for(const auto& stmt_use : stmt_uses)
         {
            if(not tree_helper::is_virtual(TM, stmt_use.first->index))
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
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Liveness for function " + BH->get_function_name());

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
      const auto TM = AppM->get_tree_manager();
      auto tn = TM->get_tree_node_const(function_id);
      auto fd = GetPointer<function_decl>(tn);
      THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
      auto sl = GetPointer<statement_list>(GET_NODE(fd->body));
      for(auto block : sl->list_of_bloc)
      {
         block.second->live_in.clear();
         block.second->live_out.clear();
      }
   }
}
