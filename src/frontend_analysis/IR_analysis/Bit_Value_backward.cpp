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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

// include class header
#include "Bit_Value.hpp"

// include from tree/
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

// behavior includes
#include "application_manager.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

// include boost range adaptors
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp"
#include <boost/range/adaptors.hpp>

std::deque<bit_lattice> Bit_Value::get_current_or_best(const tree_nodeConstRef& tn) const
{
   const auto nid = GET_INDEX_CONST_NODE(tn);
   const auto node = GET_CONST_NODE(tn);
   if(node->get_kind() == ssa_name_K && current.find(nid) != current.end())
   {
      return current.at(nid);
   }
   THROW_ASSERT(best.count(nid), "");
   return best.at(nid);
}

void Bit_Value::backward()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing backward transfer");
   std::deque<tree_nodeConstRef> working_list;
   CustomUnorderedSet<unsigned int> working_list_idx;
   auto push_back = [&](const tree_nodeConstRef& stmt) {
      const auto stmt_kind = stmt->get_kind();
      if(!working_list_idx.count(stmt->index) && (stmt_kind == gimple_assign_K || stmt_kind == gimple_phi_K || stmt_kind == gimple_call_K || stmt_kind == gimple_return_K || stmt_kind == gimple_cond_K))
      {
         working_list.push_back(stmt);
         working_list_idx.insert(stmt->index);
      }
   };
   auto pop_front = [&]() -> tree_nodeConstRef {
      const tree_nodeConstRef stmt = working_list.front();
      working_list.pop_front();
      working_list_idx.erase(stmt->index);
      return stmt;
   };
   for(const auto& bb : boost::adaptors::reverse(bb_topological))
   {
      for(const auto& stmt : boost::adaptors::reverse(bb->CGetStmtList()))
      {
         const auto s = GET_CONST_NODE(stmt);
         push_back(s);
         THROW_ASSERT(GetPointer<const gimple_node>(s)->bb_index == bb->number, "");
      }
      for(const auto& stmt : boost::adaptors::reverse(bb->CGetPhiList()))
      {
         const auto s = GET_CONST_NODE(stmt);
         const auto gp = GetPointerS<const gimple_phi>(s);
         if(!gp->virtual_flag)
         {
            const auto output_uid = GET_INDEX_CONST_NODE(gp->res);
            if(is_handled_by_bitvalue(output_uid))
            {
               push_back(s);
               THROW_ASSERT(GetPointer<const gimple_node>(s)->bb_index == bb->number, "");
            }
         }
      }
   }
   const auto is_root_function = AppM->CGetCallGraphManager()->GetRootFunctions().count(function_id);
   while(!working_list.empty())
   {
      const auto stmt = pop_front();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + STR(stmt));
      const auto stmt_kind = stmt->get_kind();
      if(stmt_kind == gimple_assign_K)
      {
         const auto ga = GetPointerS<const gimple_assign>(stmt);
         const auto lhs_nid = GET_INDEX_CONST_NODE(ga->op0);
         if(!is_handled_by_bitvalue(lhs_nid))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(GET_CONST_NODE(ga->op0)) + " of type " + STR(tree_helper::CGetType(GET_CONST_NODE(ga->op0))) + " not considered id: " + STR(lhs_nid));
            continue;
         }
         std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
         tree_helper::get_required_values(TM, vars_read, TM->GetTreeNode(stmt->index), stmt->index);
         for(const auto& var_pair : vars_read)
         {
            const auto ssa_use_node_id = std::get<0>(var_pair);
            if(ssa_use_node_id == 0)
            {
               continue;
            }
            if(!is_handled_by_bitvalue(ssa_use_node_id))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---variable " + STR(TM->CGetTreeNode(ssa_use_node_id)) + " of type " + STR(tree_helper::CGetType(TM->CGetTreeNode(ssa_use_node_id))) + " not considered id: " + STR(ssa_use_node_id));
               continue;
            }
            const auto in_node = TM->CGetTreeNode(ssa_use_node_id);
            if(in_node->get_kind() == ssa_name_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Propagation for (" + STR(ssa_use_node_id) + ")" + STR(in_node));
               auto res = backward_transfer(ga, ssa_use_node_id);
               if(res.empty())
               {
                  THROW_ASSERT(best.find(ssa_use_node_id) != best.end(), "unexpected condition");
                  res = best.at(ssa_use_node_id);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---best: " + bitstring_to_string(res));
               }
               THROW_ASSERT(res.size(), "");
               auto& output_current = current[ssa_use_node_id];
               if(output_current.size())
               {
                  res = inf(res, output_current, ssa_use_node_id);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(res));
               if(output_current != res)
               {
                  output_current = res;
                  const auto ssa_var = GetPointerS<const ssa_name>(in_node);
                  const auto nextNode = ssa_var->CGetDefStmt();
                  push_back(GET_CONST_NODE(nextNode));
               }
            }
         }
      }
      else if(stmt_kind == gimple_phi_K)
      {
         const auto gp = GetPointerS<const gimple_phi>(stmt);
         if(!gp->virtual_flag)
         {
            const auto lhs_nid = GET_INDEX_CONST_NODE(gp->res);
            if(!is_handled_by_bitvalue(lhs_nid))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(GET_CONST_NODE(gp->res)) + " of type " + STR(tree_helper::CGetType(GET_CONST_NODE(gp->res))) + " not considered id: " + STR(lhs_nid));
               continue;
            }
            const auto lhs_bitstring = get_current_or_best(gp->res);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->");
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "LHS: " + bitstring_to_string(lhs_bitstring));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               const auto edge_node = GET_CONST_NODE(def_edge.first);
               if(edge_node->get_kind() == ssa_name_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Propagation for (" + STR(edge_node->index) + ")" + STR(edge_node));
                  auto res = lhs_bitstring;
                  THROW_ASSERT(res.size(), "");
                  auto& output_current = current[edge_node->index];
                  if(output_current.size())
                  {
                     res = inf(res, output_current, edge_node->index);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(res));
                  if(output_current != res)
                  {
                     output_current = res;
                     const auto ssa_var = GetPointerS<const ssa_name>(edge_node);
                     const auto nextNode = ssa_var->CGetDefStmt();
                     push_back(GET_CONST_NODE(nextNode));
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
         }
      }
      else if(stmt_kind == gimple_call_K)
      {
         const auto gc = GetPointerS<const gimple_call>(stmt);
         const auto call_it = direct_call_id_to_called_id.find(gc->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const auto called_id = call_it->second;
            const auto called_tn = TM->CGetTreeNode(called_id);
            const auto called_fd = GetPointerS<const function_decl>(called_tn);

            const auto& actual_parms = gc->args;
            const auto& formal_parms = called_fd->list_of_args;
            THROW_ASSERT(actual_parms.size() == formal_parms.size(), "");
            auto a_it = actual_parms.cbegin();
            const auto a_end = actual_parms.cend();
            auto f_it = formal_parms.cbegin();
            const auto f_end = formal_parms.cend();
            for(; a_it != a_end && f_it != f_end; a_it++, f_it++)
            {
               if(!is_handled_by_bitvalue(GET_INDEX_CONST_NODE(*a_it)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---variable " + STR(*a_it) + " of type " + STR(tree_helper::CGetType(*a_it)) + " not considered id: " + STR(GET_INDEX_CONST_NODE(*a_it)));
                  continue;
               }
               const auto arg_node = GET_CONST_NODE(*a_it);
               if(arg_node->get_kind() == ssa_name_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Propagation for (" + STR(arg_node->index) + ")" + STR(arg_node));
                  const auto p_decl_id = AppM->getSSAFromParm(called_id, GET_INDEX_CONST_NODE(*f_it));
                  const auto parmssa = TM->CGetTreeNode(p_decl_id);
                  const auto pd = GetPointerS<const ssa_name>(parmssa);
                  std::deque<bit_lattice> res;
                  if(pd->bit_values.empty())
                  {
                     res = create_u_bitstring(BitLatticeManipulator::Size(parmssa));
                  }
                  else
                  {
                     res = string_to_bitstring(pd->bit_values);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---param: " + bitstring_to_string(res));
                  THROW_ASSERT(res.size(), "");
                  auto& output_current = current[arg_node->index];
                  if(output_current.size())
                  {
                     res = inf(res, output_current, arg_node->index);
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(res));
                  if(output_current != res)
                  {
                     output_current = res;
                     const auto ssa_var = GetPointerS<const ssa_name>(arg_node);
                     const auto nextNode = ssa_var->CGetDefStmt();
                     push_back(GET_CONST_NODE(nextNode));
                  }
               }
            }
         }
      }
      else if(stmt_kind == gimple_return_K)
      {
         if(!is_root_function)
         {
            const auto gr = GetPointerS<const gimple_return>(stmt);
            if(gr->op && GET_CONST_NODE(gr->op)->get_kind() == ssa_name_K)
            {
               const auto op_nid = GET_INDEX_CONST_NODE(gr->op);
               if(!is_handled_by_bitvalue(op_nid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(gr->op) + " of type " + STR(tree_helper::CGetType(GET_CONST_NODE(gr->op))) + " not considered id: " + STR(op_nid));
                  continue;
               }
               std::deque<bit_lattice> res;
               // TODO: replace current with best, because IPA is updating best
               if(current.find(function_id) != current.end())
               {
                  res = current.at(function_id);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---ret: " + bitstring_to_string(res));
               }
               else
               {
                  THROW_ASSERT(best.find(op_nid) != best.end(), "unexpected condition");
                  res = best.at(op_nid);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---best: " + bitstring_to_string(res));
               }
               THROW_ASSERT(res.size(), "");
               auto& output_current = current[op_nid];
               if(output_current.size())
               {
                  res = inf(res, output_current, op_nid);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(res));
               if(output_current != res)
               {
                  output_current = res;
                  const auto ssa_var = GetPointerS<const ssa_name>(GET_CONST_NODE(gr->op));
                  const auto nextNode = ssa_var->CGetDefStmt();
                  push_back(GET_CONST_NODE(nextNode));
               }
            }
         }
      }
      else if(stmt_kind == gimple_cond_K)
      {
         const auto gc = GetPointer<const gimple_cond>(stmt);
         if(GET_CONST_NODE(gc->op0)->get_kind() == ssa_name_K)
         {
            const auto op_nid = GET_INDEX_CONST_NODE(gc->op0);
            // TODO: this is because IrLowering is not doing its job, better fix it and uncomment the assert
            // THROW_ASSERT(tree_helper::is_bool(TM, op_nid), "gimple_cond operand is not bool - " + STR(tree_helper::CGetType(GET_CONST_NODE(gc->op0))));
            auto res = create_u_bitstring(1);
            auto& output_current = current[op_nid];
            if(output_current.size())
            {
               res = inf(res, output_current, op_nid);
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(res));
            if(output_current != res)
            {
               output_current = res;
               const auto ssa_var = GetPointerS<const ssa_name>(GET_CONST_NODE(gc->op0));
               const auto nextNode = ssa_var->CGetDefStmt();
               push_back(GET_CONST_NODE(nextNode));
            }
         }
      }
      else
      {
         THROW_UNREACHABLE("Unhandled statement: " + STR(stmt) + "(" + tree_node::GetString(stmt_kind) + ")");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + STR(stmt));
   }

   if(!is_root_function)
   {
      const auto tn = TM->CGetTreeNode(function_id);
      const auto fd = GetPointerS<const function_decl>(tn);
      for(const auto& parm_decl_node : fd->list_of_args)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing argument " + STR(parm_decl_node));
         const auto parmssa_id = AppM->getSSAFromParm(function_id, GET_INDEX_CONST_NODE(parm_decl_node));
         const auto parmssa = TM->CGetTreeReindex(parmssa_id);
         if(!is_handled_by_bitvalue(parmssa_id))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--argument " + STR(parmssa) + " of type " + STR(tree_helper::CGetType(GET_CONST_NODE(parmssa))) + " not considered id: " + STR(parmssa_id));
            continue;
         }
         auto res = get_current_or_best(parmssa);
         THROW_ASSERT(res.size(), "");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---res: " + bitstring_to_string(res));
         auto& output_current = current[GET_INDEX_CONST_NODE(parm_decl_node)];
         if(output_current.size())
         {
            res = inf(res, output_current, GET_INDEX_CONST_NODE(parm_decl_node));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---inf: " + bitstring_to_string(res));
         output_current = res;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed argument " + STR(parm_decl_node));
      }
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Backward propagation to function arguments not performed on root functions");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed backward transfer");
}

std::deque<bit_lattice> Bit_Value::backward_transfer(const gimple_assign* ga, unsigned int res_nid) const
{
   std::deque<bit_lattice> res;
   if(GetPointer<const cst_node>(TM->CGetTreeNode(res_nid)))
   {
      return res;
   }
   const auto& lhs = ga->op0;
   const auto lhs_nid = GET_INDEX_CONST_NODE(lhs);
   const auto lhs_size = BitLatticeManipulator::Size(lhs);
   const auto lhs_kind = GET_CONST_NODE(lhs)->get_kind();

   switch(lhs_kind)
   {
      case array_ref_K:
      {
         auto operation = GetPointerS<const array_ref>(GET_CONST_NODE(ga->op0));
         do
         {
            if(GET_INDEX_NODE(operation->op1) == res_nid && current.count(GET_INDEX_CONST_NODE(operation->op1)))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS equals operand");
               return create_u_bitstring(pointer_resizing(GET_INDEX_CONST_NODE(operation->op0)));
            }
            operation = GetPointerS<const array_ref>(GET_CONST_NODE(operation->op0));
         } while(operation);
         return res;
      }
      case mem_ref_K:
      {
         const auto operation = GetPointerS<const mem_ref>(GET_CONST_NODE(ga->op0));
         if(GET_INDEX_CONST_NODE(operation->op0) == res_nid && current.count(GET_INDEX_CONST_NODE(operation->op0)))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---LHS equals operand");
            return create_u_bitstring(pointer_resizing(res_nid));
         }
         return res;
      }
      case ssa_name_K:
      {
         break;
      }
      case CASE_UNARY_EXPRESSION:
      case assert_expr_K:
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case eq_expr_K:
      case exact_div_expr_K:
      case fdesc_expr_K:
      case floor_div_expr_K:
      case floor_mod_expr_K:
      case ge_expr_K:
      case gt_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case init_expr_K:
      case le_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case lt_expr_K:
      case max_expr_K:
      case min_expr_K:
      case minus_expr_K:
      case modify_expr_K:
      case mult_expr_K:
      case mult_highpart_expr_K:
      case ne_expr_K:
      case ordered_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case set_le_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case uneq_expr_K:
      case ltgt_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case unordered_expr_K:
      case widen_sum_expr_K:
      case widen_mult_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      case extract_bit_expr_K:
      case sat_plus_expr_K:
      case sat_minus_expr_K:
      case CASE_TERNARY_EXPRESSION:
      case array_range_ref_K:
      case CASE_TYPE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_FAKE_NODES:
      case CASE_CPP_NODES:
      case CASE_DECL_NODES:
      case CASE_CST_NODES:
      case CASE_GIMPLE_NODES:
      case aggr_init_expr_K:
      case binfo_K:
      case block_K:
      case call_expr_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case lut_expr_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      default:
      {
         THROW_UNREACHABLE("Unhandled lhs expression: " + ga->ToString() + " (" + tree_node::GetString(lhs_kind) + ")");
         break;
      }
   }

   const auto lhs_bitstring = get_current_or_best(lhs);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---backward_transfer, lhs: " + bitstring_to_string(lhs_bitstring));
   const auto& rhs = ga->op1;
   const auto rhs_kind = GET_CONST_NODE(rhs)->get_kind();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- => " + tree_node::GetString(rhs_kind));
   switch(rhs_kind)
   {
      // Unary expressions
      case addr_expr_K:
      case bit_not_expr_K:
      case convert_expr_K:
      case negate_expr_K:
      case nop_expr_K:
      case truth_not_expr_K:
      case view_convert_expr_K:
      {
         const auto operation = GetPointerS<const unary_expr>(GET_CONST_NODE(rhs));

         const auto op_nid = GET_INDEX_NODE(operation->op);
         if(!is_handled_by_bitvalue(op_nid))
         {
            break;
         }
         THROW_ASSERT(best.count(op_nid), "");
         auto op_bitstring = best.at(op_nid);
         THROW_ASSERT(res_nid == op_nid, "unexpected condition");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   operand(" + STR(op_nid) + "): " + bitstring_to_string(op_bitstring));

         if(rhs_kind == addr_expr_K)
         {
            if(ga->temporary_address)
            {
               const auto op_kind = GET_CONST_NODE(operation->op)->get_kind();
               if(op_kind == mem_ref_K)
               {
                  const auto mr = GetPointerS<const mem_ref>(GET_CONST_NODE(operation->op));
                  if(GET_INDEX_CONST_NODE(mr->op0) == res_nid && current.count(GET_INDEX_CONST_NODE(mr->op0)))
                  {
                     return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(ga->op0)));
                  }
               }
               else if(op_kind == target_mem_ref461_K)
               {
                  const auto tmr = GetPointerS<const target_mem_ref461>(GET_CONST_NODE(operation->op));

                  if(GET_INDEX_CONST_NODE(tmr->base) == res_nid && current.count(GET_INDEX_CONST_NODE(tmr->base)))
                  {
                     return create_u_bitstring(pointer_resizing(lhs_nid));
                  }
                  else if(tmr->idx2 && GET_INDEX_CONST_NODE(tmr->idx2) == res_nid && current.count(GET_INDEX_CONST_NODE(tmr->idx2)))
                  {
                     return create_u_bitstring(pointer_resizing(lhs_nid));
                  }
               }
               else if(op_kind == array_ref_K)
               {
                  auto ar = GetPointerS<const array_ref>(GET_CONST_NODE(operation->op));
                  do
                  {
                     if(GET_INDEX_CONST_NODE(ar->op1) == res_nid && current.count(GET_INDEX_CONST_NODE(ar->op1)))
                     {
                        return create_u_bitstring(pointer_resizing(GET_INDEX_CONST_NODE(ar->op0)));
                     }
                     ar = GetPointerS<const array_ref>(GET_CONST_NODE(ar->op0));
                  } while(operation);
               }
            }
         }
         else if(rhs_kind == bit_not_expr_K)
         {
            const auto lhs_bitsize = lhs_bitstring.size();

            auto se_lhs_bitstring = lhs_bitstring;
            const auto initial_size = op_bitstring.size();
            if(initial_size < lhs_bitsize)
            {
               op_bitstring = sign_extend_bitstring(op_bitstring, tree_helper::is_int(TM, op_nid), lhs_bitsize);
            }
            if(initial_size > lhs_bitsize)
            {
               se_lhs_bitstring = sign_extend_bitstring(lhs_bitstring, tree_helper::is_int(TM, lhs_nid), initial_size);
            }

            auto it_lhs_bitstring = se_lhs_bitstring.rbegin();
            auto it_op_bitstring = op_bitstring.rbegin();
            for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op_bitstring != op_bitstring.rend(); ++it_lhs_bitstring, ++it_op_bitstring)
            {
               if(*it_lhs_bitstring == bit_lattice::X)
               {
                  res.push_front(bit_lattice::X);
               }
               else
               {
                  res.push_front(*it_op_bitstring);
               }
            }
         }
         else if(rhs_kind == convert_expr_K || rhs_kind == nop_expr_K || rhs_kind == view_convert_expr_K)
         {
            const bool lhs_signed = tree_helper::is_int(TM, lhs_nid);
            const bool op_signed = tree_helper::is_int(TM, op_nid);
            const auto op_size = BitLatticeManipulator::Size(operation->op);
            if(op_signed && !lhs_signed)
            {
               /*
                * ###################################################################
                * WARNING!! do not remove this condition!
                * the backward propagation of casts cannot be performed when the lhs
                * is unsigned and the rhs is signed.
                * the reason is that bitstrings attached to unsigned do not carry
                * around implicit information on sign bits. on the contrary,
                * bitstrings attached to signed carry this kind of implicit
                * information. if we propagate back from unsigned to signed there are
                * some corner cases when the sign bit information on the rhs would be
                * overwritten by dontcares in the lhs unsigned string, leading to
                * nasty propagations bugs, that occur rarely and only in very complex
                * tests, and are very hard to track down.
                * ###################################################################
                */
               break;
            }

            res = lhs_bitstring;
            if(res.size() < lhs_size)
            {
               res = sign_extend_bitstring(res, lhs_signed, lhs_size);
            }
            if(lhs_size > op_size)
            {
               if(op_size < 32 && op_size > 1)
               {
                  const auto sign_bit = res.front();
                  res.pop_front();
                  while(res.size() > op_size)
                  {
                     res.pop_front();
                  }
                  res.front() = bit_inf(sign_bit, res.front());
               }
               else
               {
                  while(res.size() > op_size)
                  {
                     res.pop_front();
                  }
               }
            }
         }
         else if(rhs_kind == negate_expr_K)
         {
            res = op_bitstring;
            const auto initial_size = op_bitstring.size();
            while(res.size() > lhs_bitstring.size())
            {
               res.pop_front();
            }
            if(res.size() != initial_size)
            {
               res.push_front(bit_lattice::X);
            }
         }
         else if(rhs_kind == truth_not_expr_K)
         {
            res = op_bitstring;
            while(res.size() > 1)
            {
               res.pop_front();
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled unary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) + ")");
         }
         break;
      }
      // Binary expressions
      case bit_and_expr_K:
      case bit_ior_expr_K:
      case bit_xor_expr_K:
      case extract_bit_expr_K:
      case lrotate_expr_K:
      case lshift_expr_K:
      case mem_ref_K:
      case minus_expr_K:
      case mult_expr_K:
      case plus_expr_K:
      case pointer_plus_expr_K:
      case rrotate_expr_K:
      case rshift_expr_K:
      case truth_and_expr_K:
      case truth_andif_expr_K:
      case truth_or_expr_K:
      case truth_orif_expr_K:
      case truth_xor_expr_K:
      case widen_mult_expr_K:
      {
         const auto operation = GetPointer<const binary_expr>(GET_CONST_NODE(rhs));

         auto op0_nid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.count(op0_nid), "");
         auto op0_bitstring = best.at(op0_nid);

         auto op1_nid = GET_INDEX_NODE(operation->op1);
         THROW_ASSERT(best.count(op1_nid), "");
         auto op1_bitstring = best.at(op1_nid);

         THROW_ASSERT(res_nid == op0_nid || res_nid == op1_nid, "unexpected condition");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   operand0(" + STR(op0_nid) + "): " + bitstring_to_string(op0_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   operand1(" + STR(op1_nid) + "): " + bitstring_to_string(op1_bitstring));

         if(rhs_kind == bit_and_expr_K || rhs_kind == bit_ior_expr_K || rhs_kind == bit_xor_expr_K)
         {
            const auto lhs_bitsize = lhs_bitstring.size();

            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op1_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }

            auto se_lhs_bitstring = lhs_bitstring;
            const auto initial_size = op0_bitstring.size();
            if(initial_size < lhs_bitsize)
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_bitsize);
            }
            if(initial_size > lhs_bitsize)
            {
               se_lhs_bitstring = sign_extend_bitstring(lhs_bitstring, tree_helper::is_int(TM, lhs_nid), initial_size);
            }
            if(op0_bitstring.size() > op1_bitstring.size())
            {
               op1_bitstring = sign_extend_bitstring(op1_bitstring, tree_helper::is_int(TM, op1_nid), op0_bitstring.size());
            }
            if(op1_bitstring.size() > op0_bitstring.size())
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), op1_bitstring.size());
            }

            auto it_lhs_bitstring = se_lhs_bitstring.rbegin();
            auto it_op0_bitstring = op0_bitstring.rbegin();
            auto it_op1_bitstring = op1_bitstring.rbegin();
            for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op0_bitstring != op0_bitstring.rend(); ++it_lhs_bitstring, ++it_op0_bitstring, ++it_op1_bitstring)
            {
               if(*it_lhs_bitstring == bit_lattice::X)
               {
                  res.push_front(bit_lattice::X);
               }
               else if(rhs_kind == bit_and_expr_K && *it_op0_bitstring != bit_lattice::ZERO && *it_op1_bitstring == bit_lattice::ZERO)
               {
                  res.push_front(bit_lattice::X);
               }
               else if(rhs_kind == bit_ior_expr_K && *it_op0_bitstring != bit_lattice::ONE && *it_op1_bitstring == bit_lattice::ONE)
               {
                  res.push_front(bit_lattice::X);
               }
               else
               {
                  res.push_front(*it_op0_bitstring);
               }
            }
         }
         else if(rhs_kind == lrotate_expr_K || rhs_kind == rrotate_expr_K)
         {
            if(op1_nid == res_nid)
            {
               const auto op_signed_p = tree_helper::is_int(TM, res_nid);
               unsigned int log2;
               for(log2 = 1; lhs_size > (1u << log2); ++log2)
               {
                  ;
               }
               res = op1_bitstring;
               for(auto index = 0u; res.size() > index + log2; ++index)
               {
                  if(op_signed_p && (res.size() == index + log2 + 1))
                  {
                     res[index] = bit_lattice::ZERO;
                  }
                  else
                  {
                     res[index] = bit_lattice::X;
                  }
               }
            }
         }
         else if(rhs_kind == lshift_expr_K)
         {
            if(GET_CONST_NODE(operation->op1)->get_kind() != integer_cst_K)
            {
               if(op1_nid == res_nid)
               {
                  const auto op_signed_p = tree_helper::is_int(TM, res_nid);
                  unsigned int log2;
                  for(log2 = 1; lhs_size > (1u << log2); ++log2)
                  {
                     ;
                  }
                  res = op1_bitstring;
                  for(auto index = 0u; res.size() > index + log2; ++index)
                  {
                     if(op_signed_p && (res.size() == index + log2 + 1))
                     {
                        res[index] = bit_lattice::ZERO;
                     }
                     else
                     {
                        res[index] = bit_lattice::X;
                     }
                  }
               }
               break;
            }

            const auto const1 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
            if(const1->value < 0)
            {
               res.push_back(bit_lattice::X);
               break;
            }

            res = lhs_bitstring;
            while(res.size() > (lhs_bitstring.size() - (static_cast<long long unsigned int>(const1->value))))
            {
               res.pop_back();
            }
            if(res.size() < lhs_bitstring.size())
            {
               res = sign_extend_bitstring(res, tree_helper::is_int(TM, res_nid), lhs_bitstring.size());
            }
         }
         else if(rhs_kind == mem_ref_K)
         {
            if(op0_nid == res_nid)
            {
               res = create_u_bitstring(pointer_resizing(res_nid));
            }
         }
         else if(rhs_kind == minus_expr_K || rhs_kind == mult_expr_K || rhs_kind == plus_expr_K || rhs_kind == pointer_plus_expr_K || rhs_kind == widen_mult_expr_K)
         {
            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op1_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }

            res = op0_bitstring;
            const auto initial_size = op0_bitstring.size();
            while(res.size() > lhs_bitstring.size())
            {
               res.pop_front();
            }
            if(res.size() != initial_size)
            {
               res.push_front(bit_lattice::X);
            }
         }
         else if(rhs_kind == rshift_expr_K)
         {
            if(GET_CONST_NODE(operation->op1)->get_kind() != integer_cst_K)
            {
               if(op1_nid == res_nid)
               {
                  const auto op_signed_p = tree_helper::is_int(TM, res_nid);
                  unsigned int log2;
                  for(log2 = 1; lhs_size > (1u << log2); ++log2)
                  {
                     ;
                  }
                  res = op1_bitstring;
                  for(auto index = 0u; res.size() > index + log2; ++index)
                  {
                     if(op_signed_p && (res.size() == index + log2 + 1))
                     {
                        res[index] = bit_lattice::ZERO;
                     }
                     else
                     {
                        res[index] = bit_lattice::X;
                     }
                  }
               }
               break;
            }

            const auto const1 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
            if(const1->value < 0)
            {
               res.push_back(bit_lattice::X);
               break;
            }

            res = lhs_bitstring;
            const auto shift_value = static_cast<unsigned long long>(const1->value);
            for(auto shift_value_it = 0u; shift_value_it < shift_value; shift_value_it++)
            {
               res.push_back(bit_lattice::X);
            }

            const auto shifted_type_size = BitLatticeManipulator::Size(operation->op0);
            while(res.size() > shifted_type_size)
            {
               res.pop_front();
            }
         }
         else if(rhs_kind == truth_and_expr_K || rhs_kind == truth_andif_expr_K || rhs_kind == truth_or_expr_K || rhs_kind == truth_orif_expr_K || rhs_kind == truth_xor_expr_K)
         {
            if(op0_nid != res_nid)
            {
               std::swap(op0_nid, op0_nid);
               std::swap(op0_bitstring, op1_bitstring);
            }
            res = op0_bitstring;
            while(res.size() > 1)
            {
               res.pop_front();
            }
         }
         else if(rhs_kind == extract_bit_expr_K)
         {
            if(op1_nid == res_nid)
            {
               THROW_ERROR("unexpected condition");
               break;
            }

            const auto const1 = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op1));
            THROW_ASSERT(const1->value >= 0, "unexpected condition");
            THROW_ASSERT(lhs_bitstring.size() == 1, "unexpected condition - " + bitstring_to_string(lhs_bitstring));

            res = lhs_bitstring;
            const auto shift_value = static_cast<unsigned long long>(const1->value);
            for(auto shift_value_it = 0u; shift_value_it < shift_value; shift_value_it++)
            {
               res.push_back(bit_lattice::X);
            }
            const auto shifted_type_size = BitLatticeManipulator::Size(operation->op0);
            while(res.size() < shifted_type_size)
            {
               res.push_front(bit_lattice::X);
            }
            while(res.size() > shifted_type_size)
            {
               res.pop_front();
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled binary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) + ")");
         }
         break;
      }
      // Ternary expressions
      case bit_ior_concat_expr_K:
      case cond_expr_K:
      case ternary_plus_expr_K:
      case ternary_pm_expr_K:
      case ternary_mp_expr_K:
      case ternary_mm_expr_K:
      {
         const auto operation = GetPointer<const ternary_expr>(GET_CONST_NODE(rhs));

         auto op0_nid = GET_INDEX_NODE(operation->op0);
         THROW_ASSERT(best.count(op0_nid), "");
         auto op0_bitstring = best.at(op0_nid);

         auto op1_nid = GET_INDEX_NODE(operation->op1);
         THROW_ASSERT(best.count(op1_nid), "");
         auto op1_bitstring = best.at(op1_nid);

         auto op2_nid = GET_INDEX_NODE(operation->op2);
         THROW_ASSERT(best.count(op2_nid), "");
         auto op2_bitstring = best.at(op2_nid);

         THROW_ASSERT(res_nid == op0_nid || res_nid == op1_nid || res_nid == op2_nid, "unexpected condition");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   operand0(" + STR(op0_nid) + "): " + bitstring_to_string(op0_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   operand1(" + STR(op1_nid) + "): " + bitstring_to_string(op1_bitstring));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---   operand2(" + STR(op2_nid) + "): " + bitstring_to_string(op2_bitstring));

         if(rhs_kind == bit_ior_concat_expr_K)
         {
            if(op2_nid == res_nid)
            {
               break;
            }
            const auto lhs_bitsize = lhs_bitstring.size();

            if(op0_nid == res_nid)
            {
               op1_nid = 0;
            }
            if(op1_nid == res_nid)
            {
               op0_nid = op1_nid;
               op0_bitstring = op1_bitstring;
            }

            const auto offset = GetPointerS<const integer_cst>(GET_CONST_NODE(operation->op2))->value;
            const auto initial_size = op0_bitstring.size();
            auto se_lhs_bitstring = lhs_bitstring;
            if(initial_size < lhs_bitsize)
            {
               op0_bitstring = sign_extend_bitstring(op0_bitstring, tree_helper::is_int(TM, op0_nid), lhs_bitsize);
            }
            if(initial_size > lhs_bitsize)
            {
               se_lhs_bitstring = sign_extend_bitstring(lhs_bitstring, tree_helper::is_int(TM, lhs_nid), initial_size);
            }

            auto it_lhs_bitstring = se_lhs_bitstring.rbegin();
            auto it_op0_bitstring = op0_bitstring.rbegin();
            long long int index = 0;
            if(op1_nid)
            {
               for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op0_bitstring != op0_bitstring.rend() && index < offset; ++it_lhs_bitstring, ++it_op0_bitstring, ++index)
               {
                  res.push_front(*it_lhs_bitstring);
               }
               if(tree_helper::is_int(TM, op1_nid))
               {
                  res.push_front(bit_lattice::X);
               }
            }
            else
            {
               for(; it_lhs_bitstring != se_lhs_bitstring.rend() && it_op0_bitstring != op0_bitstring.rend(); ++it_lhs_bitstring, ++it_op0_bitstring, ++index)
               {
                  if(index < offset)
                  {
                     res.push_front(bit_lattice::ZERO);
                  }
                  else
                  {
                     res.push_front(*it_lhs_bitstring);
                  }
               }
            }
         }
         else if(rhs_kind == cond_expr_K)
         {
            if(op0_nid == res_nid)
            {
               break;
            }

            if(op1_nid != res_nid)
            {
               std::swap(op1_nid, op2_nid);
               std::swap(op1_bitstring, op2_bitstring);
            }

            auto it_lhs_bitstring = lhs_bitstring.rbegin();
            auto it_op1_bitstring = op1_bitstring.rbegin();
            for(; it_lhs_bitstring != lhs_bitstring.rend() && it_op1_bitstring != op1_bitstring.rend(); ++it_lhs_bitstring, ++it_op1_bitstring)
            {
               if(*it_lhs_bitstring == bit_lattice::X)
               {
                  res.push_front(bit_lattice::X);
               }
               else
               {
                  res.push_front(*it_op1_bitstring);
               }
            }
            if(res.front() == bit_lattice::X && op1_bitstring.size() < lhs_bitstring.size())
            {
               const auto arg1_sign = op1_bitstring.front();
               res.pop_front();
               res.push_front(arg1_sign);
            }
         }
         else if(rhs_kind == ternary_plus_expr_K || rhs_kind == ternary_pm_expr_K || rhs_kind == ternary_mp_expr_K || rhs_kind == ternary_mm_expr_K)
         {
            if(op0_nid == res_nid)
            {
               res = op0_bitstring;
            }
            if(op1_nid == res_nid)
            {
               res = op1_bitstring;
            }
            if(op2_nid == res_nid)
            {
               res = op2_bitstring;
            }
            const auto initial_size = res.size();
            while(res.size() > lhs_bitstring.size())
            {
               res.pop_front();
            }
            if(res.size() != initial_size)
            {
               res.push_front(bit_lattice::X);
            }
         }
         else
         {
            THROW_UNREACHABLE("Unhadled ternary expression: " + ga->ToString() + "(" + tree_node::GetString(rhs_kind) + ")");
         }
         break;
      }
      case array_ref_K:
      {
         auto operation = GetPointerS<const array_ref>(GET_CONST_NODE(rhs));
         do
         {
            if(GET_INDEX_CONST_NODE(operation->op1) == res_nid)
            {
               return create_u_bitstring(pointer_resizing(GET_INDEX_CONST_NODE(operation->op0)));
            }
            operation = GetPointer<const array_ref>(GET_CONST_NODE(operation->op0));
         } while(operation);
         break;
      }
      case aggr_init_expr_K:
      case call_expr_K:
      {
         const auto call = GetPointerS<const call_expr>(GET_CONST_NODE(rhs));
         const auto call_it = direct_call_id_to_called_id.find(ga->index);
         if(call_it != direct_call_id_to_called_id.end())
         {
            const auto called_id = call_it->second;
            const auto tn = TM->CGetTreeNode(called_id);
            const auto fd = GetPointerS<const function_decl>(tn);

            const auto actual_parms = call->args;
            const auto formal_parms = fd->list_of_args;
            THROW_ASSERT(actual_parms.size() == formal_parms.size(), "");
            auto a_it = actual_parms.cbegin();
            auto a_end = actual_parms.cend();
            auto f_it = formal_parms.cbegin();
            auto f_end = formal_parms.cend();
            bool found = actual_parms.empty();
            for(; a_it != a_end and f_it != f_end; a_it++, f_it++)
            {
               if(GET_INDEX_CONST_NODE(*a_it) == res_nid)
               {
                  const auto p_decl_id = AppM->getSSAFromParm(called_id, GET_INDEX_CONST_NODE(*f_it));
                  const auto parmssa = TM->CGetTreeNode(p_decl_id);
                  const auto pd = GetPointerS<const ssa_name>(parmssa);
                  std::deque<bit_lattice> tmp;
                  if(pd->bit_values.empty())
                  {
                     tmp = create_u_bitstring(BitLatticeManipulator::Size(parmssa));
                  }
                  else
                  {
                     tmp = string_to_bitstring(pd->bit_values);
                  }

                  res = found ? inf(res, tmp, res_nid) : tmp;
                  found = true;
               }
            }
            THROW_ASSERT(found, STR(res_nid) + " is not an actual parameter of function " + STR(called_id));
         }
         break;
      }
      case ssa_name_K:
      {
         THROW_ASSERT(best.count(GET_INDEX_CONST_NODE(rhs)), "");
         res = best.at(GET_INDEX_CONST_NODE(rhs));
         break;
      }
      // Unary expressions
      case abs_expr_K:
      // Binary expressions
      case eq_expr_K:
      case exact_div_expr_K:
      case ge_expr_K:
      case gt_expr_K:
      case le_expr_K:
      case lt_expr_K:
      case ltgt_expr_K:
      case max_expr_K:
      case min_expr_K:
      case ne_expr_K:
      case ordered_expr_K:
      case sat_minus_expr_K:
      case sat_plus_expr_K:
      case trunc_div_expr_K:
      case trunc_mod_expr_K:
      case uneq_expr_K:
      case unge_expr_K:
      case ungt_expr_K:
      case unle_expr_K:
      case unlt_expr_K:
      case unordered_expr_K:
      case lut_expr_K:
      {
         // Do nothing
         break;
      }
      // Unary expressions
      case arrow_expr_K:
      case buffer_ref_K:
      case card_expr_K:
      case cleanup_point_expr_K:
      case conj_expr_K:
      case exit_expr_K:
      case fix_ceil_expr_K:
      case fix_floor_expr_K:
      case fix_round_expr_K:
      case fix_trunc_expr_K:
      case float_expr_K:
      case imagpart_expr_K:
      case indirect_ref_K:
      case misaligned_indirect_ref_K:
      case loop_expr_K:
      case non_lvalue_expr_K:
      case paren_expr_K:
      case realpart_expr_K:
      case reference_expr_K:
      case reinterpret_cast_expr_K:
      case sizeof_expr_K:
      case static_cast_expr_K:
      case throw_expr_K:
      case unsave_expr_K:
      case va_arg_expr_K:
      case reduc_max_expr_K:
      case reduc_min_expr_K:
      case reduc_plus_expr_K:
      case vec_unpack_hi_expr_K:
      case vec_unpack_lo_expr_K:
      case vec_unpack_float_hi_expr_K:
      case vec_unpack_float_lo_expr_K:
      // Binary expressions
      case assert_expr_K:
      case catch_expr_K:
      case ceil_div_expr_K:
      case ceil_mod_expr_K:
      case complex_expr_K:
      case compound_expr_K:
      case eh_filter_expr_K:
      case fdesc_expr_K:
      case floor_div_expr_K:
      case floor_mod_expr_K:
      case goto_subroutine_K:
      case in_expr_K:
      case init_expr_K:
      case modify_expr_K:
      case mult_highpart_expr_K:
      case postdecrement_expr_K:
      case postincrement_expr_K:
      case predecrement_expr_K:
      case preincrement_expr_K:
      case range_expr_K:
      case rdiv_expr_K:
      case round_div_expr_K:
      case round_mod_expr_K:
      case set_le_expr_K:
      case try_catch_expr_K:
      case try_finally_K:
      case widen_sum_expr_K:
      case with_size_expr_K:
      case vec_lshift_expr_K:
      case vec_rshift_expr_K:
      case widen_mult_hi_expr_K:
      case widen_mult_lo_expr_K:
      case vec_pack_trunc_expr_K:
      case vec_pack_sat_expr_K:
      case vec_pack_fix_trunc_expr_K:
      case vec_extracteven_expr_K:
      case vec_extractodd_expr_K:
      case vec_interleavehigh_expr_K:
      case vec_interleavelow_expr_K:
      // Ternary expressions
      case component_ref_K:
      case bit_field_ref_K:
      case vtable_ref_K:
      case with_cleanup_expr_K:
      case obj_type_ref_K:
      case save_expr_K:
      case vec_cond_expr_K:
      case vec_perm_expr_K:
      case dot_prod_expr_K:
      // Quaternary expressions
      case array_range_ref_K:
      case CASE_TYPE_NODES:
      case CASE_PRAGMA_NODES:
      case CASE_FAKE_NODES:
      case CASE_CPP_NODES:
      case CASE_DECL_NODES:
      case CASE_CST_NODES:
      case CASE_GIMPLE_NODES:
      case binfo_K:
      case block_K:
      case case_label_expr_K:
      case constructor_K:
      case error_mark_K:
      case identifier_node_K:
      case statement_list_K:
      case target_expr_K:
      case target_mem_ref_K:
      case target_mem_ref461_K:
      case tree_list_K:
      case tree_vec_K:
      default:
      {
         THROW_UNREACHABLE("Unhandled rhs expression: " + ga->ToString() + " (" + tree_node::GetString(rhs_kind) + ")");
         break;
      }
   }
   // TODO: this is because IrLowering is not doing its job, better fix it and remove this
   const auto res_size = BitLatticeManipulator::Size(TM->CGetTreeNode(res_nid));
   while(res.size() > res_size)
   {
      res.pop_front();
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "--- res: " + bitstring_to_string(res));
   return res;
}
