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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file BitValueIPA.cpp
 *
 * Created on: June 27, 2016
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

// include class header
#include "BitValueIPA.hpp"

// include from src/
#include "Parameter.hpp"

// include from src/behavior/
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// include from src/design_flow/
#include "application_manager.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

// include from src/frontend_analysis/
#include "function_frontend_flow_step.hpp"

#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"      // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// STD include
#include <string>
#include <utility>

BitValueIPA::BitValueIPA(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par)
    : ApplicationFrontendFlowStep(AM, BIT_VALUE_IPA, dfm, par), BitLatticeManipulator(AM->get_tree_manager(), parameters->get_class_debug_level(GET_CLASS(*this)))
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

BitValueIPA::~BitValueIPA() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> BitValueIPA::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(BIT_VALUE, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(MEM_CG_EXT, SAME_FUNCTION));
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(IR_LOWERING, ALL_FUNCTIONS));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, ALL_FUNCTIONS));
         break;
      }
      case INVALIDATION_RELATIONSHIP:
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

void BitValueIPA::ComputeRelationships(DesignFlowStepSet& relationships, const DesignFlowStep::RelationshipType relationship_type)
{
   if(relationship_type == INVALIDATION_RELATIONSHIP)
   {
      for(const auto i : fun_id_to_restart)
      {
         const std::string step_signature = FunctionFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BIT_VALUE, i);
         vertex frontend_step = design_flow_manager.lock()->GetDesignFlowStep(step_signature);
         THROW_ASSERT(frontend_step != NULL_VERTEX, "step " + step_signature + " is not present");
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const DesignFlowStepRef design_flow_step = design_flow_graph->CGetDesignFlowStepInfo(frontend_step)->design_flow_step;
         relationships.insert(design_flow_step);
         AppM->GetFunctionBehavior(i)->UpdateBitValueVersion();
      }
   }
   ApplicationFrontendFlowStep::ComputeRelationships(relationships, relationship_type);
}

bool BitValueIPA::HasToBeExecuted() const
{
   std::map<unsigned int, unsigned int> cur_bitvalue_ver;
   std::map<unsigned int, unsigned int> cur_bb_ver;
   const CallGraphManagerConstRef CGMan = AppM->CGetCallGraphManager();
   for(const auto i : CGMan->GetReachedBodyFunctions())
   {
      const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(i);
      cur_bitvalue_ver[i] = FB->GetBitValueVersion();
      cur_bb_ver[i] = FB->GetBBVersion();
   }
   return cur_bb_ver != last_bb_ver || cur_bitvalue_ver != last_bitvalue_ver;
}

DesignFlowStep_Status BitValueIPA::Exec()
{
#ifndef NDEBUG
   if(not AppM->ApplyNewTransformation())
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
#endif

   BitLatticeManipulator::clear();
   fun_id_to_restart.clear();

   const CallGraphManagerConstRef CGMan = AppM->CGetCallGraphManager();
   const CallGraphConstRef cg = CGMan->CGetCallGraph();
   CustomOrderedSet<unsigned int> reached_body_fun_ids = CGMan->GetReachedBodyFunctions();
   CustomOrderedSet<unsigned int> root_fun_ids = CGMan->GetRootFunctions();

   /// In case of indirect calls (e.g., pointer to function) no Bit Value IPA can be done.
   CustomUnorderedSet<vertex> vertex_subset;
   for(auto cvertex : reached_body_fun_ids)
      vertex_subset.insert(CGMan->GetVertex(cvertex));
   const CallGraphConstRef subgraph = CGMan->CGetCallSubGraph(vertex_subset);
   EdgeIterator e_it, e_it_end;
   for(boost::tie(e_it, e_it_end) = boost::edges(*subgraph); e_it != e_it_end; ++e_it)
   {
      const auto* info = Cget_edge_info<FunctionEdgeInfo, const CallGraph>(*e_it, *subgraph);
      if(info->indirect_call_points.size())
         return DesignFlowStep_Status::UNCHANGED;
   }

   // ---- initialization phase ----
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Initialize data structures");
   for(unsigned int fu_id : reached_body_fun_ids)
   {
      const std::string fu_name = AppM->CGetFunctionBehavior(fu_id)->CGetBehavioralHelper()->get_function_name();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing function \"" + fu_name + "\": id = " + STR(fu_id));
      const tree_nodeRef fu_node = TM->get_tree_node_const(fu_id);
      const function_decl* fd = GetPointer<function_decl>(fu_node);
      THROW_ASSERT(fd and fd->body, "Node is not a function or it hasn't a body");
      const tree_nodeConstRef fu_type = tree_helper::CGetType(fu_node);
      THROW_ASSERT(fu_type->get_kind() == function_type_K || fu_type->get_kind() == method_type_K, "node " + STR(fu_id) + " is " + fu_type->get_kind_text());
      unsigned int fret_type_id;
      tree_nodeRef fret_type_node;
      if(fu_type->get_kind() == function_type_K)
      {
         const auto* ft = GetPointer<const function_type>(fu_type);
         fret_type_id = GET_INDEX_NODE(ft->retn);
         fret_type_node = GET_NODE(ft->retn);
      }
      else
      {
         const auto* mt = GetPointer<const method_type>(fu_type);
         fret_type_id = GET_INDEX_NODE(mt->retn);
         fret_type_node = GET_NODE(mt->retn);
      }
#ifndef NDEBUG
      bool is_root = root_fun_ids.find(fu_id) != root_fun_ids.end();
#endif
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is_root = " + STR(is_root));

      // -- process parameters --
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing parameters");
      for(const auto& parm_decl_node : fd->list_of_args)
      {
         const unsigned int p_decl_id = GET_INDEX_NODE(parm_decl_node);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "parm_decl id: " + STR(p_decl_id));
         auto* p = GetPointer<parm_decl>(GET_NODE(parm_decl_node));
         unsigned int p_type_id = GET_INDEX_NODE(p->type);
         if(not is_handled_by_bitvalue(p_type_id))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "parameter type is not considered: " + STR(p_decl_id));
            continue;
         }
         std::deque<bit_lattice> b;
         if(p->bit_values.empty())
         {
            b = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(p->type)));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "new bitvalue: " + bitstring_to_string(b));
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "found bitvalue: " + p->bit_values);
            b = string_to_bitstring(p->bit_values);
         }
         best[p_decl_id] = b;
         if(tree_helper::is_int(TM, p_type_id))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
            signed_var.insert(p_decl_id);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed parameters");

      // -- process function returned value --
      if(not is_handled_by_bitvalue(fret_type_id))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--function return type is not considered: " + STR(fret_type_id));
         continue;
      }
      std::deque<bit_lattice> b;
      if(fd->bit_values.empty())
      {
         b = create_u_bitstring(BitLatticeManipulator::Size(fret_type_node));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "new bitvalue: " + bitstring_to_string(b));
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "found bitvalue: " + fd->bit_values);
         b = string_to_bitstring(fd->bit_values);
      }
      best[fu_id] = b;

      if(tree_helper::is_int(TM, fret_type_id))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
         signed_var.insert(fu_id);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed function \"" + fu_name + "\": id = " + STR(fu_id));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Initialized data structures");

   // ---- propagation phase ----
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Start BitValueIPA propagation");
   for(unsigned int fu_id : reached_body_fun_ids)
   {
      const std::string fu_name = AppM->CGetFunctionBehavior(fu_id)->CGetBehavioralHelper()->get_function_name();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing function \"" + fu_name + "\": id = " + STR(fu_id));

      const tree_nodeRef fu_node = TM->get_tree_node_const(fu_id);
      const auto* fd = GetPointer<const function_decl>(fu_node);
      THROW_ASSERT(fd and fd->body, "Node is not a function or it hasn't a body");
      const tree_nodeConstRef fu_type = tree_helper::CGetType(fu_node);
      THROW_ASSERT(fu_type->get_kind() == function_type_K || fu_type->get_kind() == method_type_K, "node " + STR(fu_id) + " is " + fu_type->get_kind_text());
      tree_nodeRef fret_type_node;
      if(fu_type->get_kind() == function_type_K)
      {
         const auto* ft = GetPointer<const function_type>(fu_type);
         fret_type_node = GET_NODE(ft->retn);
      }
      else
      {
         const auto* mt = GetPointer<const method_type>(fu_type);
         fret_type_node = GET_NODE(mt->retn);
      }
      bool is_root = root_fun_ids.find(fu_id) != root_fun_ids.end();

      // --- propagation through return values ---
      if(best.find(fu_id) != best.end())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Propagating bitvalue of the return value of function " + fu_name);

         bool fu_signed = signed_var.find(fu_id) != signed_var.cend();

         /*
          * for root functions, don't perform backward propagation from assigned
          * ssa to returned value, because this could lead to unsafe
          * optimizations if some external piece of code that was not
          * synthesized with bambu calls the top function from the bus
          */
         // --- backward ----
         if(not is_root)
         {
#ifndef NDEBUG
            if(not AppM->ApplyNewTransformation())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
               break;
            }
#endif

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Backward");

            current.insert(std::make_pair(fu_id, best.at(fu_id)));

            std::deque<bit_lattice> res = create_x_bitstring(1);

            const vertex fu_cgv = CGMan->GetVertex(fu_id);
            InEdgeIterator ie_it, ie_end;
            boost::tie(ie_it, ie_end) = boost::in_edges(fu_cgv, *cg);
            for(; ie_it != ie_end; ie_it++)
            {
               const unsigned int caller_id = CGMan->get_function(boost::source(*ie_it, *cg));
               const auto tmp_it = reached_body_fun_ids.find(caller_id);
               if(tmp_it == reached_body_fun_ids.cend())
                  continue;
               const std::string caller_name = AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->get_function_name();
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->examining caller \"" + caller_name + "\": id = " + STR(caller_id));
               const tree_nodeRef tn = TM->get_tree_node_const(caller_id);
               const FunctionEdgeInfoConstRef call_edge_info = cg->CGetFunctionEdgeInfo(*ie_it);
               bool hard_break = false;
               for(const auto i : call_edge_info->direct_call_points)
               {
                  if(i == 0)
                  {
                     THROW_ASSERT(fu_name == "__builtin_memcpy", "function " + caller_name + " calls function " + fu_name + " with an artificial call: this should not happen");
                     continue;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->examining direct call point " + STR(i));
                  const tree_nodeConstRef call_node = TM->get_tree_node_const(i);
                  if(call_node->get_kind() == gimple_assign_K)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "gimple_assign");
                     const auto* ga = GetPointer<const gimple_assign>(call_node);
                     THROW_ASSERT(ga, STR(i) + " is not a gimple assign");
                     if(GET_NODE(ga->op0)->get_kind() == ssa_name_K)
                     {
                        THROW_ASSERT(GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K, GET_NODE(ga->op1)->ToString() + " kind = " + GET_NODE(ga->op1)->get_kind_text());
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "assigns ssa_name");
                        const auto* s = GetPointer<const ssa_name>(GET_NODE(ga->op0));
                        THROW_ASSERT(s, "not ssa");
                        THROW_ASSERT(is_handled_by_bitvalue(s->index), "ssa is not handled by bitvalue");
                        THROW_ASSERT(tree_helper::is_int(TM, s->index) == fu_signed, "function " + caller_name + " calls function " + fu_name + " with return type = " + STR(tree_helper::GetFunctionReturnType(TM->get_tree_node_const(fu_id))) +
                                                                                         " and assigns the return value to ssa " + STR(s) + " of type = " + STR(tree_helper::CGetType(GET_NODE(ga->op0))) + "\ndifferent signedness!");
                        THROW_ASSERT(not s->bit_values.empty(), "unexpected assignment of return value to ssa " + STR(s) + " with id " + STR(s->index) + " and empty bit_values");
                        std::deque<bit_lattice> res_fanout = string_to_bitstring(s->bit_values);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res_fanout from ssa " + STR(s) + " id: " + STR(s->index) + " bitstring: " + bitstring_to_string(res_fanout));
                        if(res_fanout.size() > 0)
                        {
                           res = inf(res, res_fanout, fu_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "fu_id: " + STR(fu_id) + " bitstring: " + bitstring_to_string(res));
                        }
                        else
                        {
                           res = best.at(fu_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best: " + STR(fu_id) + " bitstring: " + bitstring_to_string(res));
                           hard_break = true;
                        }
                     }
                     else if(GET_NODE(ga->op1)->get_kind() != call_expr_K && GET_NODE(ga->op1)->get_kind() != aggr_init_expr_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "this call point is not in the form (ssa_name = call_expr)");
                        THROW_UNREACHABLE("unexpected pattern: function " + fu_name + " is called by " + caller_name + " in operation " + STR(call_node));
                     }
                     else
                     {
                        THROW_UNREACHABLE(STR(GET_NODE(ga->op0)) + ": the assigned value is not an ssa_name: " + GET_NODE(ga->op0)->get_kind_text());
                     }
                  }
                  else if(call_node->get_kind() == tree_reindex_K)
                  {
                     THROW_ERROR("should not be a tree reindex");
                  }
                  else
                  {
                     res = best.at(fu_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best: " + STR(fu_id) + " bitstring: " + bitstring_to_string(res));
                     hard_break = true;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--examined call point " + STR(i));
                  if(hard_break)
                     break;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--examined caller \"" + caller_name + "\": id = " + STR(caller_id));
               if(hard_break)
                  break;
            }

            update_current(res, fu_id);

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Backward done");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After backward id: " + STR(fu_id) + " bitstring: " + STR(bitstring_to_string(best.at(fu_id))));

            mix();
            current.clear();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After mix id: " + STR(fu_id) + " bitstring: " + STR(bitstring_to_string(best.at(fu_id))));

#ifndef NDEBUG
            AppM->RegisterTransformation(GetName(), fu_node);
#endif
         }

         // --- forward ---
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Forward");

            current.insert(std::make_pair(fu_id, best.at(fu_id)));

            std::deque<bit_lattice> res = create_x_bitstring(1);

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing return statements in all BBs");

            const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
            for(const auto& B_it : sl->list_of_bloc)
            {
               blocRef B = B_it.second;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(B->number));
               bool hard_break = false;
               for(const auto& stmt : B->CGetStmtList())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + STR(stmt));
                  const auto stmt_node = GET_NODE(stmt);
                  if(stmt_node->get_kind() == gimple_return_K)
                  {
                     std::deque<bit_lattice> res_tmp;
                     const auto* gr = GetPointer<const gimple_return>(stmt_node);
                     if(not gr->op)
                     {
                        res_tmp = create_u_bitstring(BitLatticeManipulator::Size(fret_type_node));
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "uninitialized return value: " + bitstring_to_string(res_tmp));
                        hard_break = true;
                     }
                     else
                     {
#if HAVE_ASSERTS
                        const unsigned int ret_id = GET_INDEX_NODE(gr->op);
#endif
                        THROW_ASSERT(is_handled_by_bitvalue(ret_id), "returned value not handled by bitvalue");
                        THROW_ASSERT(tree_helper::is_int(TM, ret_id) == fu_signed, "function " + fu_name + " has return type = " + STR(tree_helper::GetFunctionReturnType(TM->get_tree_node_const(fu_id))) + " but in stmt " + STR(gr) +
                                                                                       " returns a variable of type = " + STR(tree_helper::CGetType(GET_NODE(gr->op))) + "\ndifferent signedness!");
                        const tree_nodeRef returned_tn = GET_NODE(gr->op);
                        const auto ret_kind = returned_tn->get_kind();
                        if(ret_kind == ssa_name_K)
                        {
                           const auto* ssa = GetPointer<const ssa_name>(returned_tn);
                           if(ssa->CGetUseStmts().size() == 1 and ssa->CGetDefStmts().empty())
                           {
                              res_tmp = create_u_bitstring(BitLatticeManipulator::Size(returned_tn));
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "return uninitialized ssa id: " + STR(ssa->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else
                           {
                              THROW_ASSERT(not ssa->bit_values.empty(), "unexpected assignment of return value to ssa " + STR(ssa) + " with id " + STR(ssa->index) + " and empty bit_values");
                              res_tmp = string_to_bitstring(ssa->bit_values);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ssa return value id: " + STR(ssa->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                        }
                        else if(ret_kind == integer_cst_K)
                        {
                           const auto* ic = GetPointer<const integer_cst>(returned_tn);
                           THROW_ASSERT(ic, "not an integer_cst");
                           res_tmp = create_bitstring_from_constant(ic->value, BitLatticeManipulator::Size(returned_tn), fu_signed);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "constant return value: " + bitstring_to_string(res_tmp));
                        }
                        else if(ret_kind == real_cst_K)
                        {
                           const auto* rc = GetPointer<const real_cst>(returned_tn);
                           THROW_ASSERT(rc, "not a real_cst");
                           const auto ret_size = BitLatticeManipulator::Size(GET_CONST_NODE(rc->type));
                           THROW_ASSERT(ret_size == 64 || ret_size == 32, "Unhandled real type size (" + STR(ret_size) + ")");
                           if(rc->valx.front() == '-' && rc->valr.front() != rc->valx.front())
                           {
                              res_tmp = string_to_bitstring(convert_fp_to_string("-" + rc->valr, ret_size));
                           }
                           else
                           {
                              res_tmp = string_to_bitstring(convert_fp_to_string(rc->valr, ret_size));
                           }
                           sign_reduce_bitstring(res_tmp, false);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "constant return value: " + bitstring_to_string(res_tmp));
                        }
                        else
                        {
                           res_tmp = create_u_bitstring(BitLatticeManipulator::Size(fret_type_node));
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "return is not an ssa nor a constant value: " + returned_tn->get_kind_text() + " bitstring: " + bitstring_to_string(res_tmp));
                           hard_break = true;
                        }
                     }
                     res = inf(res, res_tmp, fu_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "fu_id: " + STR(fu_id) + " bitstring: " + bitstring_to_string(res));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + STR(stmt));
                  if(hard_break)
                     break;
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(B->number));
               if(hard_break)
                  break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed return statements in all BBs");

            update_current(res, fu_id);

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Forward done");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After forward id: " + STR(fu_id) + " bitstring: " + STR(bitstring_to_string(best.at(fu_id))));

            mix();
            current.clear();
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After mix id: " + STR(fu_id) + " bitstring: " + STR(bitstring_to_string(best.at(fu_id))));
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Propagated bitvalue of the return value of function " + fu_name);
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Return value function " + fu_name + " not handled bitvalue");
      }

      // --- propagation through parameters ---
      THROW_ASSERT(fd and fd->body, "Node is not a function or it hasn't a body");
      int args_n = 0;
      for(const auto& pd : fd->list_of_args)
      {
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
#endif

         args_n++;
         const unsigned int pd_id = GET_INDEX_NODE(pd);
         if(best.find(pd_id) != best.cend())
         {
            THROW_ASSERT(is_handled_by_bitvalue(pd_id), "param \"" + STR(pd) + "\" id: " + STR(pd_id) + " not handled by bitvalue");
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Propagating bitvalue through parameter " + STR(GET_NODE(pd)) + " of function " + fu_name + " parm id: " + STR(pd_id));

            bool parm_signed = signed_var.find(pd_id) != signed_var.cend();

            // --- backward ---
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Backward");

               current.insert(std::make_pair(pd_id, best.at(pd_id)));

               std::deque<bit_lattice> res = create_x_bitstring(1);
#if HAVE_ASSERTS
               unsigned int prev_found = 0;
#endif
               const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
               for(const auto& B_it : sl->list_of_bloc)
               {
                  blocRef B = B_it.second;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(B->number));
                  for(const auto& stmt : B->CGetStmtList())
                  {
                     TreeNodeMap<size_t> used_ssa = tree_helper::ComputeSsaUses(stmt);
                     for(const auto& s : used_ssa)
                     {
                        const tree_nodeRef ssa_tn = GET_NODE(s.first);
                        const auto* ssa = GetPointer<const ssa_name>(ssa_tn);
                        if(ssa->var != nullptr and GET_NODE(ssa->var)->get_kind() == parm_decl_K)
                        {
                           const auto def = ssa->CGetDefStmts();
                           THROW_ASSERT(not def.empty(), "ssa_name " + STR(GET_NODE(ssa_tn)) + " with id " + STR(ssa->index) + " has no def_stmts");
                           if(def.size() == 1 and (GET_INDEX_NODE(ssa->var) == pd_id) and ((GET_NODE((*def.begin()))->get_kind() == gimple_nop_K) or ssa->volatile_flag))
                           {
                              // ssa is the first version of the parameter
                              THROW_ASSERT(prev_found == 0 or ssa->index == prev_found, "multiple ssa names are the first version of the same param\n"
                                                                                        "param id: " +
                                                                                            STR(pd_id) + " ssa id: " + STR(prev_found) + " ssa id: " + STR(ssa->index));
                              THROW_ASSERT(is_handled_by_bitvalue(ssa->index), "ssa " + STR(ssa) + " with id " + STR(ssa->index) + " not handled by bitvalue");
                              THROW_ASSERT(tree_helper::is_int(TM, ssa->index) == parm_signed, "param " + STR(pd) + " of function " + fu_name + " has type = " + STR(tree_helper::CGetType(GET_NODE(pd))) +
                                                                                                   " but its first version is an ssa of type = " + STR(tree_helper::CGetType(ssa_tn)) + "\ndifferent signedness!");

                              std::deque<bit_lattice> res_fanout = ssa->bit_values.empty() ? create_u_bitstring(BitLatticeManipulator::Size(tree_helper::CGetType(ssa_tn))) : string_to_bitstring(ssa->bit_values);
#if HAVE_ASSERTS
                              prev_found = ssa->index;
#endif
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res_fanout from ssa " + STR(ssa) + " id: " + STR(ssa->index) + " is first version of param " + STR(pd) + " id: " + STR(pd->index));
                              if(res_fanout.size() > 0)
                              {
                                 res = inf(res, res_fanout, pd_id);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "param id: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                              }
                              else
                              {
                                 res = best.at(pd_id);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                              }
                           }
                        }
                     }
                  }
                  for(const auto& stmt : B->CGetPhiList())
                  {
                     TreeNodeMap<size_t> used_ssa = tree_helper::ComputeSsaUses(stmt);
                     for(const auto& s : used_ssa)
                     {
                        const tree_nodeRef ssa_tn = GET_NODE(s.first);
                        const auto* ssa = GetPointer<const ssa_name>(ssa_tn);
                        if(ssa->var != nullptr and GET_NODE(ssa->var)->get_kind() == parm_decl_K)
                        {
                           const auto def = ssa->CGetDefStmts();
                           THROW_ASSERT(not def.empty(), "ssa_name " + STR(GET_NODE(ssa_tn)) + " with id " + STR(ssa->index) + " has no def_stmts");
                           if(def.size() == 1 and (GET_INDEX_NODE(ssa->var) == pd_id) and ((GET_NODE((*def.begin()))->get_kind() == gimple_nop_K) or ssa->volatile_flag))
                           {
                              // ssa is the first version of the parameter
                              THROW_ASSERT(prev_found == 0 or ssa->index == prev_found, "multiple ssa names are the first version of the same param\n"
                                                                                        "param id: " +
                                                                                            STR(pd_id) + " ssa id: " + STR(prev_found) + " ssa id: " + STR(ssa->index));
                              THROW_ASSERT(is_handled_by_bitvalue(ssa->index), "ssa " + STR(ssa) + " with id " + STR(ssa->index) + " not handled by bitvalue");
                              THROW_ASSERT(tree_helper::is_int(TM, ssa->index) == parm_signed, "param " + STR(pd) + " of function " + fu_name + " has type = " + STR(tree_helper::CGetType(GET_NODE(pd))) +
                                                                                                   " but its first version is an ssa of type = " + STR(tree_helper::CGetType(ssa_tn)) + "\ndifferent signedness!");
                              THROW_ASSERT(not ssa->bit_values.empty(), "ssa " + STR(ssa) + " id: " + STR(ssa->index) + " parm " + STR(pd) + " id: " + STR(pd->index));
#if HAVE_ASSERTS

                              prev_found = ssa->index;
#endif
                              std::deque<bit_lattice> res_fanout = string_to_bitstring(ssa->bit_values);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res_fanout from ssa " + STR(ssa) + " id: " + STR(ssa->index) + " is first version of param " + STR(pd) + " id: " + STR(pd->index));
                              if(res_fanout.size() > 0)
                              {
                                 res = inf(res, res_fanout, pd_id);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "param id: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                              }
                              else
                              {
                                 res = best.at(pd_id);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                              }
                           }
                        }
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(B->number));
               }

               update_current(res, pd_id);

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Backward done");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After backward id: " + STR(pd_id) + " bitstring: " + STR(bitstring_to_string(best.at(pd_id))));

               mix();
               current.clear();
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After mix id: " + STR(pd_id) + " bitstring: " + STR(bitstring_to_string(best.at(pd_id))));
            }

            /*
             * for root functions, don't perform forward propagation from actual
             * parameters to formal parameters, because this could lead to unsafe
             * optimizations if some external piece of code that was not
             * synthesized with bambu calls the top function from the bus
             */
            // --- forward ---
            if(not is_root)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Forward");

               current.insert(std::make_pair(pd_id, best.at(pd_id)));

               std::deque<bit_lattice> res = create_x_bitstring(1);

               const vertex fu_cgv = CGMan->GetVertex(fu_id);
               InEdgeIterator ie_it, ie_end;
               boost::tie(ie_it, ie_end) = boost::in_edges(fu_cgv, *cg);
               for(; ie_it != ie_end; ie_it++)
               {
                  const unsigned int caller_id = CGMan->get_function(boost::source(*ie_it, *cg));
                  const auto tmp_it = reached_body_fun_ids.find(caller_id);
                  if(tmp_it == reached_body_fun_ids.cend())
                     continue;
                  const std::string caller_name = AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->get_function_name();
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->examining caller \"" + caller_name + "\": id = " + STR(caller_id));
                  const tree_nodeRef tn = TM->get_tree_node_const(caller_id);
                  const FunctionEdgeInfoConstRef call_edge_info = cg->CGetFunctionEdgeInfo(*ie_it);
                  bool hard_break = false;
                  for(const auto i : call_edge_info->direct_call_points)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->examining direct call point " + STR(i));
                     std::deque<bit_lattice> res_tmp;
                     if(i == 0)
                     {
                        THROW_ASSERT(fu_name == "__builtin_memcpy", "function " + caller_name + " calls function " + fu_name + " with an artificial call: this should not happen");
                        THROW_UNREACHABLE("pattern is not handled yet: artifical call: "
                                          "function " +
                                          fu_name + " is called by " + caller_name);
                        /*
                        res_tmp = best.at(pd_id);
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "best: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                        */
                        hard_break = true;
                     }
                     else if(TM->get_tree_node_const(i)->get_kind() == gimple_assign_K)
                     {
                        const tree_nodeConstRef call_node = TM->get_tree_node_const(i);
                        const auto* ga = GetPointer<const gimple_assign>(call_node);
                        if(GET_NODE(ga->op1)->get_kind() != call_expr_K && GET_NODE(ga->op1)->get_kind() != aggr_init_expr_K)
                        {
                           // no way to retrieve the actual parameters
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "this call point is not in the form (ssa_name = call_expr)\n"
                                          "no way to retrieve the actual parameters of the call");
                           THROW_UNREACHABLE("unexpected pattern: function " + fu_name + " is called by " + caller_name + " in operation " + STR(call_node));
                        }
                        else
                        {
                           const auto* ce = GetPointer<const call_expr>(GET_NODE(ga->op1));
                           const auto actual_parms = ce->args;
                           THROW_ASSERT(ce->args.size() == fd->list_of_args.size(), "actual parameters: " + STR(ce->args.size()) + " formal parameters: " + STR(fd->list_of_args.size()));
                           const auto ap = std::next(ce->args.cbegin(), args_n - 1);
                           const auto ap_id = GET_INDEX_NODE(*ap);
                           const auto ap_node = GET_NODE(*ap);
                           const auto ap_kind = ap_node->get_kind();
                           THROW_ASSERT(is_handled_by_bitvalue(ap_id), "actual parameter not handled by bitvalue");
                           THROW_ASSERT(tree_helper::is_int(TM, ap_id) == parm_signed, "function " + caller_name + " calls function " + fu_name + "\nformal param " + STR(pd) + " type = " + STR(tree_helper::CGetType(GET_NODE(pd))) + "\nactual param " +
                                                                                           STR(ap_node) + " type = " + STR(tree_helper::CGetType(ap_node)) + "\ndifferent signedness!");
                           if(ap_kind == ssa_name_K)
                           {
                              const auto* ssa = GetPointer<const ssa_name>(ap_node);
                              THROW_ASSERT(ssa, "not ssa");
                              THROW_ASSERT(not ssa->bit_values.empty(), "unexpected assignment of return value to ssa " + STR(ssa) + " with id " + STR(ssa->index) + " and empty bit_values");
                              res_tmp = string_to_bitstring(ssa->bit_values);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter " + STR(ssa) + " id: " + STR(ssa->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else if(ap_kind == integer_cst_K)
                           {
                              const auto* ic = GetPointer<const integer_cst>(ap_node);
                              THROW_ASSERT(ic, "not an integer_cst");
                              res_tmp = create_bitstring_from_constant(ic->value, BitLatticeManipulator::Size(ap_node), parm_signed);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter " + STR(ic) + " is a constant value id: " + STR(ic->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else if(ap_kind == real_cst_K)
                           {
                              const auto* rc = GetPointer<const real_cst>(ap_node);
                              THROW_ASSERT(rc, "not a real_cst");
                              const auto ap_size = BitLatticeManipulator::Size(GET_CONST_NODE(rc->type));
                              THROW_ASSERT(ap_size == 64 || ap_size == 32, "Unhandled real type size (" + STR(ap_size) + ")");
                              if(rc->valx.front() == '-' && rc->valr.front() != rc->valx.front())
                              {
                                 res_tmp = string_to_bitstring(convert_fp_to_string("-" + rc->valr, ap_size));
                              }
                              else
                              {
                                 res_tmp = string_to_bitstring(convert_fp_to_string(rc->valr, ap_size));
                              }
                              sign_reduce_bitstring(res_tmp, false);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter " + STR(rc) + " is a constant value id: " + STR(rc->index) + " bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else if(ap_kind == var_decl_K)
                           {
                              res_tmp = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(pd)));
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter is uninitialized, bitstring: " + bitstring_to_string(res_tmp));
                           }
                           else
                           {
                              THROW_UNREACHABLE("unexpected actual parameter " + STR(ap_node) + " id : " + STR(ap_id) + " of kind: " + ap_node->get_kind_text());
                           }
                        }
                     }
                     else if(TM->get_tree_node_const(i)->get_kind() == gimple_call_K)
                     {
                        const tree_nodeConstRef call_node = TM->get_tree_node_const(i);
                        const auto* gc = GetPointer<const gimple_call>(call_node);
                        const auto actual_parms = gc->args;
                        THROW_ASSERT(gc->args.size() == fd->list_of_args.size(), "actual parameters: " + STR(gc->args.size()) + " formal parameters: " + STR(fd->list_of_args.size()));
                        const auto ap = std::next(gc->args.cbegin(), args_n - 1);
                        const auto ap_id = GET_INDEX_NODE(*ap);
                        const auto ap_node = GET_NODE(*ap);
                        const auto ap_kind = ap_node->get_kind();
                        THROW_ASSERT(is_handled_by_bitvalue(ap_id), "actual parameter not handled by bitvalue");
                        THROW_ASSERT(tree_helper::is_int(TM, ap_id) == parm_signed, "function " + caller_name + " calls function " + fu_name + "\nformal param " + STR(pd) + " type = " + STR(tree_helper::CGetType(GET_NODE(pd))) + "\nactual param " +
                                                                                        STR(ap_node) + " type = " + STR(tree_helper::CGetType(ap_node)) + "\ndifferent signedness!");
                        if(ap_kind == ssa_name_K)
                        {
                           const auto* ssa = GetPointer<const ssa_name>(ap_node);
                           THROW_ASSERT(ssa, "not ssa");
                           res_tmp = ssa->bit_values.empty() ? create_u_bitstring(BitLatticeManipulator::Size(tree_helper::CGetType(ap_node))) : string_to_bitstring(ssa->bit_values);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter " + STR(ssa) + " id: " + STR(ssa->index) + " bitstring: " + bitstring_to_string(res_tmp));
                        }
                        else if(ap_kind == integer_cst_K)
                        {
                           const auto* ic = GetPointer<const integer_cst>(ap_node);
                           THROW_ASSERT(ic, "not an integer_cst");
                           res_tmp = create_bitstring_from_constant(ic->value, BitLatticeManipulator::Size(ap_node), parm_signed);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter " + STR(ic) + " is a constant value id: " + STR(ic->index) + " bitstring: " + bitstring_to_string(res_tmp));
                        }
                        else if(ap_kind == real_cst_K)
                        {
                           const auto* rc = GetPointer<const real_cst>(ap_node);
                           THROW_ASSERT(rc, "not a real_cst");
                           const auto ap_size = BitLatticeManipulator::Size(GET_CONST_NODE(rc->type));
                           THROW_ASSERT(ap_size == 64 || ap_size == 32, "Unhandled real type size (" + STR(ap_size) + ")");
                           if(rc->valx.front() == '-' && rc->valr.front() != rc->valx.front())
                           {
                              res_tmp = string_to_bitstring(convert_fp_to_string("-" + rc->valr, ap_size));
                           }
                           else
                           {
                              res_tmp = string_to_bitstring(convert_fp_to_string(rc->valr, ap_size));
                           }
                           sign_reduce_bitstring(res_tmp, false);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter " + STR(rc) + " is a constant value id: " + STR(rc->index) + " bitstring: " + bitstring_to_string(res_tmp));
                        }
                        else if(ap_kind == var_decl_K)
                        {
                           res_tmp = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(pd)));
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "actual parameter is uninitialized, bitstring: " + bitstring_to_string(res_tmp));
                        }
                        else
                        {
                           THROW_UNREACHABLE("unexpected actual parameter " + STR(ap_node) + " id : " + STR(ap_id) + " of kind: " + ap_node->get_kind_text());
                        }
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                       "this call point is not in the form (ssa_name = call_expr)\n"
                                       "no way to retrieve the actual parameters of the call");
                        const tree_nodeConstRef call_node = TM->get_tree_node_const(i);
                        THROW_UNREACHABLE("unexpected pattern: function " + fu_name + " is called by " + caller_name + " in operation " + STR(call_node));
                     }
                     res = inf(res, res_tmp, pd_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "param id: " + STR(pd_id) + " bitstring: " + bitstring_to_string(res));
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--examined call point " + STR(i));
                     if(hard_break)
                        break;
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--examined caller \"" + caller_name + "\": id = " + STR(caller_id));
                  if(hard_break)
                     break;
               }

               update_current(res, pd_id);

               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Forward done");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After forward id: " + STR(pd_id) + " bitstring: " + STR(bitstring_to_string(best.at(pd_id))));

               mix();
               current.clear();
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---After mix id: " + STR(pd_id) + " bitstring: " + STR(bitstring_to_string(best.at(pd_id))));
            }

#ifndef NDEBUG
            AppM->RegisterTransformation(GetName(), pd);
#endif

            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Propagated bitvalue through parameter " + STR(GET_NODE(pd)) + " of function " + fu_name + " parm id: " + STR(pd_id));
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Parameter " + STR(GET_NODE(pd)) + " of function " + fu_name + " not handled by bitvalue");
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed function \"" + fu_name + "\": id = " + STR(fu_id));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--End BitValueIPA propagation");

   // ---- update bivalues on IR ----
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Updating IR");
   for(const auto& b : best)
   {
      const unsigned int tn_id = b.first;
      const std::string& new_bitvalue = bitstring_to_string(b.second);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---updating node id: " + STR(tn_id) + " bitstring: " + new_bitvalue);
      tree_nodeRef tn = TM->get_tree_node_const(tn_id);
      const auto kind = tn->get_kind();

      std::string null_string = "";
      std::string* old_bitvalue = &null_string;
      unsigned int size = 0;
      unsigned int restart_fun_id = 0;
      if(kind == function_decl_K)
      {
         auto* fd = GetPointer<function_decl>(tn);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is a function_decl: " + AppM->CGetFunctionBehavior(fd->index)->CGetBehavioralHelper()->get_function_name() + " id: " + STR(fd->index));
         THROW_ASSERT(fd->body, "has no body");
         old_bitvalue = &fd->bit_values;
         const tree_nodeConstRef fu_type = tree_helper::CGetType(tn);
         THROW_ASSERT(fu_type->get_kind() == function_type_K || fu_type->get_kind() == method_type_K, "node " + STR(tn_id) + " is " + fu_type->get_kind_text());
         tree_nodeRef fret_type_node;
         if(fu_type->get_kind() == function_type_K)
         {
            const auto* ft = GetPointer<const function_type>(fu_type);
            fret_type_node = GET_NODE(ft->retn);
         }
         else
         {
            const auto* mt = GetPointer<const method_type>(fu_type);
            fret_type_node = GET_NODE(mt->retn);
         }

         size = BitLatticeManipulator::Size(fret_type_node);
         restart_fun_id = fd->index;
      }
      else if(kind == parm_decl_K)
      {
         auto* pd = GetPointer<parm_decl>(tn);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is a parm_decl: " + STR(pd) + " id: " + STR(pd->index));
         old_bitvalue = &pd->bit_values;
         size = BitLatticeManipulator::Size(GET_NODE(pd->type));
         restart_fun_id = GET_INDEX_NODE(pd->scpe);
      }
      else
      {
         THROW_UNREACHABLE("unexpected condition: variable of kind " + tn->get_kind_text());
      }

      bool restart = false;
      if(old_bitvalue->empty())
      {
         if(new_bitvalue != bitstring_to_string(create_u_bitstring(size)))
            restart = true;
      }
      else if(*old_bitvalue != new_bitvalue)
      {
         restart = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---old bitstring: " + *old_bitvalue + " new bitstring: " + new_bitvalue + " restart = " + (restart ? "YES" : "NO"));
      *old_bitvalue = new_bitvalue;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---updated best id: " + STR(tn_id) + " bitstring: " + *old_bitvalue);

      if(restart)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "restart function " + AppM->CGetFunctionBehavior(restart_fun_id)->CGetBehavioralHelper()->get_function_name());
         fun_id_to_restart.insert(restart_fun_id);
         const vertex fu_cgv = CGMan->GetVertex(restart_fun_id);
         InEdgeIterator ie_it, ie_end;
         boost::tie(ie_it, ie_end) = boost::in_edges(fu_cgv, *cg);
         for(; ie_it != ie_end; ie_it++)
         {
            const unsigned int caller_id = CGMan->get_function(boost::source(*ie_it, *cg));
            const auto tmp_it = reached_body_fun_ids.find(caller_id);
            if(tmp_it == reached_body_fun_ids.cend())
               continue;
            const FunctionEdgeInfoConstRef call_edge_info = cg->CGetFunctionEdgeInfo(*ie_it);
            if(not call_edge_info->direct_call_points.empty())
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "restart caller " + AppM->CGetFunctionBehavior(caller_id)->CGetBehavioralHelper()->get_function_name());
               fun_id_to_restart.insert(caller_id);
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Updated IR");

   BitLatticeManipulator::clear();

   for(const auto i : CGMan->GetReachedBodyFunctions())
   {
      const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(i);
      last_bitvalue_ver[i] = FB->GetBitValueVersion();
      last_bb_ver[i] = FB->GetBBVersion();
   }
   return fun_id_to_restart.empty() ? DesignFlowStep_Status::UNCHANGED : DesignFlowStep_Status::SUCCESS;
}

void BitValueIPA::Initialize()
{
}
