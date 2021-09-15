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
 *              Copyright (C) 2021 Politecnico di Milano
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
 * @file FunctionCallInline.cpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "FunctionCallInline.hpp"

#include "application_manager.hpp"
#include "basic_block.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "loops.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "Dominance.hpp"
#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

#define PARAMETER_INLINE_MAX_COST "inline-max-cost"

CustomSet<unsigned int> FunctionCallInline::always_inline;
CustomMap<unsigned int, CustomSet<unsigned int>> FunctionCallInline::inline_call;
size_t FunctionCallInline::max_inline_cost = 100;

static CustomUnorderedMap<kind, size_t> op_costs = {{mult_expr_K, 100},      {widen_mult_expr_K, 100}, {widen_mult_hi_expr_K, 100}, {widen_mult_lo_expr_K, 100}, {trunc_div_expr_K, 100},
                                                    {exact_div_expr_K, 100}, {round_div_expr_K, 100},  {ceil_div_expr_K, 100},      {floor_div_expr_K, 100},     {trunc_mod_expr_K, 100},
                                                    {round_mod_expr_K, 100}, {ceil_mod_expr_K, 100},   {floor_mod_expr_K, 100},     {view_convert_expr_K, 0},    {nop_expr_K, 0}};

FunctionCallInline::FunctionCallInline(const ParameterConstRef Param, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FUNCTION_CALL_INLINE, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   if(Param->IsParameter(PARAMETER_INLINE_MAX_COST))
   {
      max_inline_cost = Param->GetParameter<size_t>(PARAMETER_INLINE_MAX_COST);
   }
   if(Param->isOption(OPT_inline_functions))
   {
      const auto TM = AppM->get_tree_manager();
      const auto fnames = SplitString(Param->getOption<std::string>(OPT_inline_functions), ",");
      for(const auto& fname : fnames)
      {
         const auto fnode = TM->GetFunction(fname);
         if(fnode)
         {
            always_inline.insert(fnode->index);
         }
      }
   }
}

FunctionCallInline::~FunctionCallInline() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> FunctionCallInline::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, SAME_FUNCTION));
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_INLINE, CALLED_FUNCTIONS));
         if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
         }
         if(parameters->isOption(OPT_hls_div) && parameters->getOption<std::string>(OPT_hls_div) != "none")
         {
            relationships.insert(std::make_pair(HLS_DIV_CG_EXT, SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   return relationships;
}

bool FunctionCallInline::HasToBeExecuted() const
{
   return FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status FunctionCallInline::InternalExec()
{
   THROW_ASSERT(!parameters->IsParameter("no-inline"), "Function call inlining should not be executed");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
   const auto TM = AppM->get_tree_manager();
   const auto fd = GetPointerS<function_decl>(TM->GetTreeNode(function_id));
   THROW_ASSERT(fd->body, "");
   const auto sl = GetPointerS<statement_list>(GET_NODE(fd->body));
   bool modified = false;
   const auto inline_stmts = inline_call.find(function_id);
   if(inline_stmts != inline_call.end())
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Performing call inlining:");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
      tree_manipulationRef tree_man(new tree_manipulation(TM, parameters, AppM));
      for(const auto& stmt_id : inline_stmts->second)
      {
         const auto stmt = TM->CGetTreeReindex(stmt_id);
         if(stmt)
         {
            const auto gn = GetPointerS<const gimple_node>(GET_CONST_NODE(stmt));
            const auto bb_it = sl->list_of_bloc.find(gn->bb_index);
            if(gn->bb_index != 0 && bb_it != sl->list_of_bloc.end())
            {
               const auto& bb = bb_it->second;
               if(std::find_if(bb->CGetStmtList().cbegin(), bb->CGetStmtList().cend(), [&](const tree_nodeRef& tn) { return GET_INDEX_CONST_NODE(tn) == stmt_id; }) != bb->CGetStmtList().cend())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Inlining required for call statement " + GET_CONST_NODE(stmt)->ToString());
                  tree_man->InlineFunctionCall(stmt, bb, fd);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Function inlined successfully");
                  modified = true;
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Statement " + STR(stmt_id) + " was not present in BB" + STR(bb->number));
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "BB" + STR(gn->bb_index) + " was not found in current function");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Statement " + STR(stmt_id) + " was not in tree manager");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Performed call inlining");
      inline_call.erase(function_id);
   }

   const auto CGM = AppM->CGetCallGraphManager();
   const auto CG = CGM->CGetCallGraph();
   const auto function_v = CGM->GetVertex(function_id);
   const auto call_count = [&]() -> size_t {
      size_t call_points = 0u;
      InEdgeIterator it, it_end;
      for(boost::tie(it, it_end) = boost::in_edges(function_v, *CG); it != it_end; ++it)
      {
         const auto caller_info = CG->CGetFunctionEdgeInfo(*it);
         call_points += static_cast<size_t>(caller_info->direct_call_points.size());
         call_points += static_cast<size_t>(caller_info->indirect_call_points.size());
         call_points += static_cast<size_t>(caller_info->function_addresses.size());
      }
      return call_points;
   }();
   if(call_count == 0)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Current function has zero call points, skipping analysis...");
      return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
   }
   const auto loop_count = detect_loops(sl);
   const auto body_cost = compute_cost(sl);
   const bool force_inline = always_inline.count(function_id) || (body_cost < max_inline_cost && call_count < 5);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Current function information:");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Internal loops : " + STR(loop_count));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Call points    : " + STR(call_count));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Body cost      : " + STR(body_cost));
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Force inline   : " + STR(force_inline ? "true" : "false"));
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(function_v, *CG); ie != ie_end; ++ie)
   {
      const auto caller_id = CGM->get_function(ie->m_source);
      const auto caller_info = CG->CGetFunctionEdgeInfo(*ie);
      for(const auto& call_id : caller_info->direct_call_points)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Analysing direct call point " + STR(TM->CGetTreeNode(call_id)));
         if(force_inline)
         {
            inline_call[caller_id].insert(call_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Forced inlining required for this function");
            continue;
         }
         if(call_count == 1)
         {
            inline_call[caller_id].insert(call_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current call point is unique for this function, inlining required");
            continue;
         }
         const auto all_const_args = HasConstantArgs(TM->CGetTreeReindex(call_id));
         if(all_const_args && loop_count == 0)
         {
            inline_call[caller_id].insert(call_id);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Current call has all constant arguments, inlining required");
            continue;
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void FunctionCallInline::RequestInlineStmt(const tree_nodeConstRef& call_stmt, unsigned int caller_id)
{
#if HAVE_ASSERTS
   const auto stmt_kind = GET_CONST_NODE(call_stmt)->get_kind();
   bool is_call = stmt_kind == gimple_call_K;
   if(stmt_kind == gimple_assign_K)
   {
      const auto rhs = GetPointerS<const gimple_assign>(GET_CONST_NODE(call_stmt))->op1;
      const auto rhs_kind = GET_CONST_NODE(rhs)->get_kind();
      is_call |= rhs_kind == call_expr_K || rhs_kind == aggr_init_expr_K;
   }
#endif
   THROW_ASSERT(is_call, "Statement does not contain a call");
   inline_call[caller_id].insert(GET_INDEX_CONST_NODE(call_stmt));
}

size_t FunctionCallInline::compute_cost(const statement_list* body)
{
   size_t total_cost = 0;
   for(const auto& bb : body->list_of_bloc)
   {
      for(const auto& stmt_rdx : bb.second->CGetStmtList())
      {
         const auto stmt = GET_CONST_NODE(stmt_rdx);
         if(stmt->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(stmt);
            const auto op_kind = GET_CONST_NODE(ga->op1)->get_kind();
            const auto op_cost = op_costs.find(op_kind);
            if(op_cost != op_costs.end())
            {
               total_cost += op_cost->second;
            }
            else
            {
               total_cost += 1;
            }
         }
         else if(stmt->get_kind() == gimple_cond_K)
         {
            total_cost += 8;
         }
         else if(stmt->get_kind() == gimple_multi_way_if_K)
         {
            const auto mw = GetPointerS<const gimple_multi_way_if>(stmt);
            total_cost += mw->list_of_cond.size() * 8ULL;
         }
         else if(stmt->get_kind() == gimple_asm_K)
         {
            const auto ga = GetPointerS<const gimple_asm>(stmt);
            total_cost += ga->str.size() / 5;
         }
         else if(stmt->get_kind() == gimple_return_K)
         {
         }
         else
         {
            return std::numeric_limits<size_t>::max();
         }
      }
   }
   return total_cost;
}

bool FunctionCallInline::HasConstantArgs(const tree_nodeConstRef& call_stmt)
{
   const auto all_constant = [](const std::vector<tree_nodeRef>& tns) {
      for(const auto& tn : tns)
      {
         if(!tree_helper::IsConstant(tn))
         {
            return false;
         }
      }
      return true;
   };
   const auto call_kind = GET_CONST_NODE(call_stmt)->get_kind();
   if(call_kind == gimple_call_K)
   {
      const auto gc = GetPointerS<const gimple_call>(GET_CONST_NODE(call_stmt));
      return all_constant(gc->args);
   }
   else if(call_kind == gimple_assign_K)
   {
      const auto ga = GetPointerS<const gimple_assign>(GET_CONST_NODE(call_stmt));
      THROW_ASSERT(GET_CONST_NODE(ga->op1)->get_kind() == call_expr_K, "");
      const auto ce = GetPointerS<const call_expr>(GET_CONST_NODE(ga->op1));
      return all_constant(ce->args);
   }
   THROW_UNREACHABLE("Expected call statement: " + tree_node::GetString(call_kind) + " - " + STR(call_stmt));
   return false;
}

size_t FunctionCallInline::detect_loops(const statement_list* body) const
{
   /// store the IR BB graph ala boost::graph
   BBGraphsCollectionRef bb_graphs_collection(new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraph cfg(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   for(const auto& block : body->list_of_bloc)
   {
      inverse_vertex_map.insert(std::make_pair(block.first, bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)))));
   }

   /// add edges
   for(const auto& curr_bb_pair : body->list_of_bloc)
   {
      const auto& curr_bbi = curr_bb_pair.first;
      const auto& curr_bb = curr_bb_pair.second;
      for(const auto& lop : curr_bb->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)), "BB" + STR(lop) + " (successor of BB" + STR(curr_bbi) + ") does not exist");
         bb_graphs_collection->AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bbi), CFG_SELECTOR);
      }

      for(const auto& los : curr_bb->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection->AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(curr_bb->list_of_succ.empty())
      {
         bb_graphs_collection->AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);
      }
   }
   bb_graphs_collection->AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);
   std::map<size_t, UnorderedSetStdStable<vertex>> sccs;
   cfg.GetStronglyConnectedComponents(sccs);
   size_t loop_count = 0;
   for(const auto& scc : sccs)
   {
      if(scc.second.size() > 1)
      {
         loop_count++;
      }
   }
   return loop_count;
}