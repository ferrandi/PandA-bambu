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
#include "function_behavior.hpp"

// include boost range adaptors
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp"
#include <boost/range/adaptors.hpp>

std::deque<bit_lattice> Bit_Value::backward_compute_result_from_uses(const ssa_name& ssa, const statement_list& sl, unsigned int bb_loop_id) const
{
   const unsigned int output_uid = ssa.index;
   std::deque<bit_lattice> res = create_x_bitstring(1);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering variable:" + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(output_uid) + "(" + STR(output_uid) + ")");
   if(not ssa.CGetUseStmts().empty())
   {
      for(auto statement_node : ssa.CGetUseStmts())
      {
         const tree_nodeRef use_stmt = GET_NODE(statement_node.first);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Output " + STR(output_uid) + " is used in " + use_stmt->get_kind_text() + ": " + STR(GET_INDEX_NODE(statement_node.first)) + "(" + STR(use_stmt) + ")");
         const auto use_kind = use_stmt->get_kind();
         if(use_kind == gimple_assign_K)
         {
            const auto* ga_tmp = GetPointer<const gimple_assign>(use_stmt);
            std::deque<bit_lattice> res_fanout = backward_transfer(ga_tmp, output_uid);
            if(res_fanout.size() > 0)
            {
               res = inf(res, res_fanout, output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "inf:" + bitstring_to_string(res));
            }
            else
            {
               res = best.at(output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best");
               break;
            }
         }
         else if(use_kind == gimple_phi_K)
         {
            const auto* gp_tmp = GetPointer<const gimple_phi>(use_stmt);
            bool all_comes_from_the_same_loop = true;
            for(const auto& def_edge : gp_tmp->CGetDefEdgesList())
            {
               if(bb_loop_id != sl.list_of_bloc.at(def_edge.second)->loop_id)
                  all_comes_from_the_same_loop = false;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "all_comes_from_the_same_loop = " + STR(all_comes_from_the_same_loop));
            const auto dest_block = sl.list_of_bloc.at(gp_tmp->bb_index);
            unsigned int dest_loop_id = dest_block->loop_id;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bb_loop_id = " + STR(bb_loop_id));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "dest_loop_id = " + STR(dest_loop_id));
            if(bb_loop_id < dest_loop_id || all_comes_from_the_same_loop)
            {
               res = inf(res, best.at(GET_INDEX_NODE(gp_tmp->res)), output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "inf:" + bitstring_to_string(res));
            }
            else
            {
               res = best.at(output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best");
               break;
            }
         }
         else if(use_kind == gimple_return_K)
         {
#if HAVE_ASSERTS
            const auto* gr = GetPointer<const gimple_return>(use_stmt);
#endif
            THROW_ASSERT(gr->op, "ssa id " + STR(output_uid) + "used in empty return statement: " + STR(gr->index));
            std::deque<bit_lattice> res_fanout = std::deque<bit_lattice>();
            auto res_it = current.find(function_id);
            if(res_it != current.end())
               res_fanout = res_it->second;
            if(res_fanout.size() > 0)
            {
               res = inf(res, res_fanout, output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "inf:" + bitstring_to_string(res));
            }
            else
            {
               res = best.at(output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best");
               break;
            }
         }
         else if(use_kind == gimple_call_K)
         {
            std::deque<bit_lattice> res_fanout;
            const auto* gc_tmp = GetPointer<const gimple_call>(use_stmt);
            THROW_ASSERT(gc_tmp, "not a gimple_call");
            const auto call_it = direct_call_id_to_called_id.find(gc_tmp->index);
            if(call_it != direct_call_id_to_called_id.end())
            {
               const unsigned int called_id = call_it->second;
               const tree_nodeConstRef called_tn = TM->get_tree_node_const(called_id);
               const auto* called_fd = GetPointer<const function_decl>(called_tn);

               const auto actual_parms = gc_tmp->args;
               const auto formal_parms = called_fd->list_of_args;
               THROW_ASSERT(actual_parms.size() == formal_parms.size(), "");
               auto a_it = actual_parms.cbegin();
               auto a_end = actual_parms.cend();
               auto f_it = formal_parms.cbegin();
               auto f_end = formal_parms.cend();
               bool found = actual_parms.empty();
               for(; a_it != a_end and f_it != f_end; a_it++, f_it++)
               {
                  if(GET_INDEX_NODE(*a_it) == output_uid)
                  {
                     const auto* pd = GetPointer<const parm_decl>(GET_NODE(*f_it));
                     std::deque<bit_lattice> tmp;
                     if(pd->bit_values.empty())
                        tmp = create_u_bitstring(tree_helper::Size(GET_NODE(pd->type)));
                     else
                        tmp = string_to_bitstring(pd->bit_values);

                     res_fanout = found ? inf(res_fanout, tmp, output_uid) : tmp;
                     found = true;
                  }
               }
               THROW_ASSERT(found, STR(output_uid) + " is not an actual parameter of function " + STR(called_id));
            }

            if(res_fanout.size() > 0)
            {
               res = inf(res, res_fanout, output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "inf:" + bitstring_to_string(res));
            }
            else
            {
               res = best.at(output_uid);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best");
               break;
            }
         }
         else
         {
            res = best.at(output_uid);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best");
            break;
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "output uid: " + STR(output_uid) + " bitstring: " + bitstring_to_string(res));
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "no one use variable: " + STR(output_uid));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered variable:" + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(output_uid) + "(" + STR(output_uid) + ")");
   return res;
}

void Bit_Value::backward()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing backward transfer");
   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));

   bool current_updated = true;

   while(current_updated)
   {
      current_updated = false;
      // for each basic block B in CFG do > Consider all blocks successively
      for(const auto& B_it : sl->list_of_bloc)
      {
         blocRef B = B_it.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(B->number));
         for(const auto& stmt : boost::adaptors::reverse(B->CGetStmtList()))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + STR(stmt));
            const tree_nodeRef s = GET_NODE(stmt);
            const auto stmt_kind = s->get_kind();
            if(stmt_kind == gimple_assign_K)
            {
               auto* ga = GetPointer<gimple_assign>(s);
               unsigned int output_uid = GET_INDEX_NODE(ga->op0);
               auto* ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));

               if(ssa)
               {
                  if(not is_handled_by_bitvalue(output_uid))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(ga->op0))) + " not considered id: " + STR(output_uid));
                     continue;
                  }

                  THROW_ASSERT(best.find(output_uid) != best.end(), "unexpected condition");
                  if(current.find(output_uid) == current.end())
                  {
                     current[output_uid] = best.at(output_uid);
                  }

                  if(bitstring_constant(current[output_uid]))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable has been proven to be constant: " + STR(output_uid));
                     continue;
                  }
                  auto res = backward_compute_result_from_uses(*ssa, *sl, B->loop_id);
                  current_updated = update_current(res, output_uid) or current_updated;
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + STR(stmt));
         }

         for(const auto& stmt : boost::adaptors::reverse(B->CGetPhiList()))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing Phi " + STR(stmt));
            auto* gp = GetPointer<gimple_phi>(GET_NODE(stmt));
            bool is_virtual = gp->virtual_flag;

            if(!is_virtual)
            {
               unsigned int output_uid = GET_INDEX_NODE(gp->res);
               auto* ssa = GetPointer<ssa_name>(GET_NODE(gp->res));
               if(not is_handled_by_bitvalue(output_uid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(gp->res))) + " not considered id: " + STR(output_uid));
                  continue;
               }

               THROW_ASSERT(best.find(output_uid) != best.end(), "unexpected condition");
               if(current.find(output_uid) == current.end())
               {
                  current[output_uid] = best.at(output_uid);
               }

               if(bitstring_constant(current[output_uid]))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--variable has been proven to be constant: " + STR(output_uid));
                  continue;
               }
               auto res = backward_compute_result_from_uses(*ssa, *sl, B->loop_id);
               current_updated = update_current(res, output_uid) or current_updated;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed Phi " + STR(stmt));
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(B->number));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed backward transfer");
}

std::deque<bit_lattice> Bit_Value::backward_transfer(const gimple_assign* ga, unsigned int output_id) const
{
   enum kind left_kind = GET_NODE(ga->op0)->get_kind();
   if(left_kind == mem_ref_K)
   {
      auto* operation = GetPointer<mem_ref>(GET_NODE(ga->op0));

      if(GET_INDEX_NODE(operation->op0) == output_id && current.find(GET_INDEX_NODE(operation->op0)) != current.end())
      {
         return create_u_bitstring(pointer_resizing(output_id));
      }
      else
         return std::deque<bit_lattice>();
   }
   else if(left_kind == target_mem_ref461_K)
   {
      auto* operation = GetPointer<target_mem_ref461>(GET_NODE(ga->op0));

      if(GET_INDEX_NODE(operation->base) == output_id && current.find(GET_INDEX_NODE(operation->base)) != current.end())
      {
         return create_u_bitstring(pointer_resizing(output_id));
      }
      else
         return std::deque<bit_lattice>();
   }
   else if(left_kind == array_ref_K)
   {
      auto* operation = GetPointer<array_ref>(GET_NODE(ga->op0));
      do
      {
         if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
         {
            return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(operation->op0)));
         }
         operation = GetPointer<array_ref>(GET_NODE(operation->op0));
      } while(operation);
      return std::deque<bit_lattice>();
   }
   else if(current.find(GET_INDEX_NODE(ga->op0)) == current.end())
   {
      return std::deque<bit_lattice>();
   }

   const std::deque<bit_lattice>& output_bitstring = current.at(GET_INDEX_NODE(ga->op0));
   enum kind op_kind = GET_NODE(ga->op1)->get_kind();
   /// first check if the gimple assign still produces something of relevant and it is not a call_expr
   if(output_bitstring.size() == 1 && output_bitstring.front() == bit_lattice::X && op_kind != call_expr_K && op_kind != aggr_init_expr_K && op_kind != mem_ref_K)
      return output_bitstring;

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, GET_NODE(ga->op1)->get_kind_text());
   if(op_kind == ssa_name_K)
   {
      THROW_ASSERT(output_id == GET_INDEX_NODE(ga->op1), "unexpected condition:" + STR(output_id) + ":" + STR(GET_INDEX_NODE(ga->op1)));
      return output_bitstring;
   }
#if 1
   else if(op_kind == mult_expr_K || op_kind == widen_mult_expr_K || op_kind == plus_expr_K || op_kind == minus_expr_K || op_kind == pointer_plus_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op0);
      unsigned int arg2_uid = GET_INDEX_NODE(operation->op1);
      if(arg1_uid != output_id)
         std::swap(arg1_uid, arg2_uid);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition in backward transferring " + ga->ToString());

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      size_t initial_size = arg1_bitstring.size();
      while(arg1_bitstring.size() > output_bitstring.size())
         arg1_bitstring.pop_front();
      if(arg1_bitstring.size() != initial_size)
         arg1_bitstring.push_front(bit_lattice::X);

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
#endif
#if 1
   else if(op_kind == truth_and_expr_K || op_kind == truth_or_expr_K)
   {
      auto* operation = GetPointer<binary_expr>(GET_NODE(ga->op1));
      unsigned int arg1_uid = GET_INDEX_NODE(operation->op0);
      unsigned int arg2_uid = GET_INDEX_NODE(operation->op1);

      if(arg1_uid != output_id)
         std::swap(arg1_uid, arg2_uid);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      while(arg1_bitstring.size() > 1)
         arg1_bitstring.pop_front();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
   else if(op_kind == negate_expr_K)
   {
      auto* operation = GetPointer<negate_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      if(tree_helper::is_real(TM, arg1_uid))
      {
         // TODO: implement back propagation for real variables
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer Error: operation unhandled yet with real type operand -> " + GET_NODE(ga->op1)->get_kind_text());
         return std::deque<bit_lattice>();
      }

      size_t initial_size = arg1_bitstring.size();
      while(arg1_bitstring.size() > output_bitstring.size())
         arg1_bitstring.pop_front();
      if(arg1_bitstring.size() != initial_size)
         arg1_bitstring.push_front(bit_lattice::X);

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
   else if(op_kind == abs_expr_K)
   {
      auto* operation = GetPointer<abs_expr>(GET_NODE(ga->op1));
      if(tree_helper::is_real(TM, GET_INDEX_NODE(operation->op)))
      {
         //    const auto left_id = GET_INDEX_NODE(ga->op0);
         //    const auto& left_bitstring = current.at(left_id);

         // TODO: Back-propagate X on msb and other bits as in standard cast for real types
      }

      /// we cannot say anything on abs_expr
      return std::deque<bit_lattice>();
   }
   else if(op_kind == truth_not_expr_K)
   {
      auto* operation = GetPointer<truth_not_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      while(arg1_bitstring.size() > 1)
         arg1_bitstring.pop_front();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
#endif
#if 1
   else if(op_kind == bit_and_expr_K)
   {
      auto* operation = GetPointer<bit_and_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op0);
      unsigned int arg2_uid = GET_INDEX_NODE(operation->op1);
      size_t output_bitsize = output_bitstring.size();

      if(arg1_uid != output_id)
         std::swap(arg1_uid, arg2_uid);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();
      if(best.find(arg2_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      std::deque<bit_lattice> arg2_bitstring = best.at(arg2_uid);
      std::deque<bit_lattice> se_output_bitstring = output_bitstring;

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(arg1_uid) + " & " + STR(arg2_uid));

      size_t initial_size = arg1_bitstring.size();
      if(initial_size < output_bitsize)
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), output_bitsize);
      }
      if(initial_size > output_bitsize)
      {
         se_output_bitstring = sign_extend_bitstring(output_bitstring, tree_helper::is_int(TM, GET_INDEX_NODE(ga->op0)), initial_size);
      }
      if(arg1_bitstring.size() > arg2_bitstring.size())
      {
         arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
      }
      if(arg2_bitstring.size() > arg1_bitstring.size())
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " &");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(se_output_bitstring));

      std::deque<bit_lattice>::const_reverse_iterator it_output_bitstring = se_output_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg1_bitstring = arg1_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg2_bitstring = arg2_bitstring.rbegin();

      std::deque<bit_lattice> res_input1;
      for(; it_output_bitstring != se_output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend(); ++it_output_bitstring, ++it_arg1_bitstring, ++it_arg2_bitstring)
      {
         if(*it_output_bitstring == bit_lattice::X)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else if(*it_arg2_bitstring == bit_lattice::ZERO and *it_arg1_bitstring != bit_lattice::ZERO)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else
         {
            res_input1.push_front(*it_arg1_bitstring);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
   else if(op_kind == bit_ior_expr_K)
   {
      auto* operation = GetPointer<bit_ior_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op0);
      unsigned int arg2_uid = GET_INDEX_NODE(operation->op1);
      size_t output_bitsize = output_bitstring.size();

      if(arg1_uid != output_id)
         std::swap(arg1_uid, arg2_uid);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();
      if(best.find(arg2_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      std::deque<bit_lattice> arg2_bitstring = best.at(arg2_uid);
      std::deque<bit_lattice> se_output_bitstring = output_bitstring;

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(arg1_uid) + " | " + STR(arg2_uid));

      size_t initial_size = arg1_bitstring.size();
      if(initial_size < output_bitsize)
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), output_bitsize);
      }
      if(initial_size > output_bitsize)
      {
         se_output_bitstring = sign_extend_bitstring(output_bitstring, tree_helper::is_int(TM, GET_INDEX_NODE(ga->op0)), initial_size);
      }
      if(arg1_bitstring.size() > arg2_bitstring.size())
      {
         arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
      }
      if(arg2_bitstring.size() > arg1_bitstring.size())
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " |");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(se_output_bitstring));

      std::deque<bit_lattice>::const_reverse_iterator it_output_bitstring = se_output_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg1_bitstring = arg1_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg2_bitstring = arg2_bitstring.rbegin();

      std::deque<bit_lattice> res_input1;

      for(; it_output_bitstring != se_output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend(); ++it_output_bitstring, ++it_arg1_bitstring, ++it_arg2_bitstring)
      {
         if(*it_output_bitstring == bit_lattice::X)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else if(*it_arg1_bitstring != bit_lattice::ONE && *it_arg2_bitstring == bit_lattice::ONE)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else
         {
            res_input1.push_front(*it_arg1_bitstring);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
   else if(op_kind == bit_ior_concat_expr_K)
   {
      auto* operation = GetPointer<bit_ior_concat_expr>(GET_NODE(ga->op1));

      size_t output_bitsize = output_bitstring.size();

      unsigned int arg1_uid = 0;
      unsigned int arg2_uid = 0;
      if(GET_INDEX_NODE(operation->op0) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op0);

      if(GET_INDEX_NODE(operation->op1) == output_id)
         arg2_uid = arg1_uid = GET_INDEX_NODE(operation->op1);

      if(GET_INDEX_NODE(operation->op2) == output_id)
         return std::deque<bit_lattice>();

      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      long long int offset = GetPointer<integer_cst>(GET_NODE(operation->op2))->value;

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      std::deque<bit_lattice> se_output_bitstring = output_bitstring;

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " |concat " + STR(GET_INDEX_NODE(operation->op1)) + "(" + STR(offset) + ")");

      size_t initial_size = arg1_bitstring.size();
      if(initial_size < output_bitsize)
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), output_bitsize);
      }
      if(initial_size > output_bitsize)
      {
         se_output_bitstring = sign_extend_bitstring(output_bitstring, tree_helper::is_int(TM, GET_INDEX_NODE(ga->op0)), initial_size);
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(se_output_bitstring));

      std::deque<bit_lattice>::const_reverse_iterator it_output_bitstring = se_output_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg1_bitstring = arg1_bitstring.rbegin();

      std::deque<bit_lattice> res_input1;
      long long int index = 0;

      if(arg2_uid)
      {
         for(; it_output_bitstring != se_output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend() && index < offset; ++it_output_bitstring, ++it_arg1_bitstring, ++index)
         {
            res_input1.push_front(*it_output_bitstring);
         }
         if(tree_helper::is_int(TM, arg2_uid))
            res_input1.push_front(bit_lattice::X);
      }
      else
      {
         for(; it_output_bitstring != se_output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend(); ++it_output_bitstring, ++it_arg1_bitstring, ++index)
         {
            if(index < offset)
               res_input1.push_front(bit_lattice::ZERO);
            else
               res_input1.push_front(*it_output_bitstring);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
   else if(op_kind == bit_xor_expr_K)
   {
      auto* operation = GetPointer<bit_xor_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op0);
      unsigned int arg2_uid = GET_INDEX_NODE(operation->op1);
      size_t output_bitsize = output_bitstring.size();

      if(arg1_uid != output_id)
         std::swap(arg1_uid, arg2_uid);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();
      if(best.find(arg2_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      std::deque<bit_lattice> arg2_bitstring = best.at(arg2_uid);
      std::deque<bit_lattice> se_output_bitstring = output_bitstring;

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(arg1_uid) + " | " + STR(arg2_uid));
      size_t initial_size = arg1_bitstring.size();
      if(initial_size < output_bitsize)
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), output_bitsize);
      }
      if(initial_size > output_bitsize)
      {
         se_output_bitstring = sign_extend_bitstring(output_bitstring, tree_helper::is_int(TM, GET_INDEX_NODE(ga->op0)), initial_size);
      }
      if(arg1_bitstring.size() > arg2_bitstring.size())
      {
         arg2_bitstring = sign_extend_bitstring(arg2_bitstring, tree_helper::is_int(TM, arg2_uid), arg1_bitstring.size());
      }
      if(arg2_bitstring.size() > arg1_bitstring.size())
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), arg2_bitstring.size());
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " ^");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg2_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(se_output_bitstring));

      std::deque<bit_lattice>::const_reverse_iterator it_output_bitstring = se_output_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg1_bitstring = arg1_bitstring.rbegin();

      std::deque<bit_lattice> res_input1;

      for(; it_output_bitstring != se_output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend(); ++it_output_bitstring, ++it_arg1_bitstring)
      {
         if(*it_output_bitstring == bit_lattice::X)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else
         {
            res_input1.push_front(*it_arg1_bitstring);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
#endif
#if 1
   else if(op_kind == bit_not_expr_K)
   {
      auto* operation = GetPointer<bit_not_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op);
      size_t output_bitsize = output_bitstring.size();

      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      std::deque<bit_lattice> se_output_bitstring = output_bitstring;
      size_t initial_size = arg1_bitstring.size();
      if(initial_size < output_bitsize)
      {
         arg1_bitstring = sign_extend_bitstring(arg1_bitstring, tree_helper::is_int(TM, arg1_uid), output_bitsize);
      }
      if(initial_size > output_bitsize)
      {
         se_output_bitstring = sign_extend_bitstring(output_bitstring, tree_helper::is_int(TM, GET_INDEX_NODE(ga->op0)), initial_size);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = ~" + STR(arg1_uid));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "~" + bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(se_output_bitstring));

      std::deque<bit_lattice>::const_reverse_iterator it_output_bitstring = se_output_bitstring.rbegin();
      std::deque<bit_lattice>::const_reverse_iterator it_arg1_bitstring = arg1_bitstring.rbegin();

      std::deque<bit_lattice> res_input1;

      for(; it_output_bitstring != se_output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend(); ++it_output_bitstring, ++it_arg1_bitstring)
      {
         if(*it_output_bitstring == bit_lattice::X)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else
         {
            res_input1.push_front(*it_arg1_bitstring);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
#endif
#if 1
   else if(op_kind == rshift_expr_K)
   {
      auto* operation = GetPointer<rshift_expr>(GET_NODE(ga->op1));

      if(GET_NODE(operation->op1)->get_kind() != integer_cst_K)
      {
#if 1
         if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
         {
            bool op_signed_p = tree_helper::is_int(TM, output_id);
            unsigned int precision = BitLatticeManipulator::size(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op0)));
            unsigned int log2;
            for(log2 = 1; precision > (1u << log2); ++log2)
               ;
            std::deque<bit_lattice> res_input1 = current.at(GET_INDEX_NODE(operation->op1));
            for(unsigned int index = 0; res_input1.size() > index + log2; ++index)
            {
               if(op_signed_p && (res_input1.size() == index + log2 + 1))
                  res_input1[index] = bit_lattice::ZERO;
               else
                  res_input1[index] = bit_lattice::X;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " >> " + STR(GET_INDEX_NODE(operation->op1)));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
            return res_input1;
         }
#endif
         return std::deque<bit_lattice>();
      }
      THROW_ASSERT(output_id == GET_INDEX_NODE(operation->op0), "unexpected condition");

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, bitstring of first operand is-> " + bitstring_to_string(output_bitstring));

      auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, created bitstring from constant -> " + STR(const2->value));
      if(const2->value < 0)
      {
         std::deque<bit_lattice> res_input1;
         res_input1.push_back(bit_lattice::X);
         return res_input1;
      }

      std::deque<bit_lattice> res_input1 = output_bitstring;
      const auto shift_value = static_cast<unsigned long long>(const2->value);
      for(unsigned long long shift_value_it = 0; shift_value_it < shift_value; shift_value_it++)
         res_input1.push_back(bit_lattice::X);

      const auto shifted_type_size = tree_helper::Size(tree_helper::CGetType(GET_NODE(operation->op0)));
      while(res_input1.size() > shifted_type_size)
         res_input1.pop_front();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " >> " + STR(GET_INDEX_NODE(operation->op1)));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
#endif
#if 1
   else if(op_kind == lshift_expr_K)
   {
      auto* operation = GetPointer<lshift_expr>(GET_NODE(ga->op1));

      if(GET_NODE(operation->op1)->get_kind() != integer_cst_K)
      {
         if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
         {
            bool op_signed_p = tree_helper::is_int(TM, output_id);
            unsigned int precision = BitLatticeManipulator::size(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op0)));
            unsigned int log2;
            for(log2 = 1; precision > (1u << log2); ++log2)
               ;
            std::deque<bit_lattice> res_input1 = current.at(GET_INDEX_NODE(operation->op1));
            for(unsigned int index = 0; res_input1.size() > index + log2; ++index)
            {
               if(op_signed_p && (res_input1.size() == index + log2 + 1))
                  res_input1[index] = bit_lattice::ZERO;
               else
                  res_input1[index] = bit_lattice::X;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " << " + STR(GET_INDEX_NODE(operation->op1)));
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
            return res_input1;
         }
         return std::deque<bit_lattice>();
      }
      THROW_ASSERT(output_id == GET_INDEX_NODE(operation->op0), "unexpected condition: " + STR(TM->CGetTreeNode(output_id)) + " vs. " + STR(operation->op0));

      auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, created bitstring from constant -> " + STR(const2->value));

      if(const2->value < 0)
      {
         std::deque<bit_lattice> res_input1;
         res_input1.push_back(bit_lattice::X);
         return res_input1;
      }

      std::deque<bit_lattice> res_input1 = output_bitstring;

      while(res_input1.size() > (output_bitstring.size() - (static_cast<long long unsigned int>(const2->value))))
         res_input1.pop_back();

      if(res_input1.size() < output_bitstring.size())
         res_input1 = sign_extend_bitstring(res_input1, tree_helper::is_int(TM, output_id), output_bitstring.size());

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " << " + STR(GET_INDEX_NODE(operation->op1)));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));

      return res_input1;
   }
#endif
#if 1
   else if(op_kind == extract_bit_expr_K)
   {
      auto* operation = GetPointer<extract_bit_expr>(GET_NODE(ga->op1));
      if(GET_INDEX_NODE(operation->op1) == output_id)
      {
         THROW_ERROR("unexpected condition");
         return std::deque<bit_lattice>();
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, bitstring of first operand is-> " + bitstring_to_string(output_bitstring));

      auto* const2 = GetPointer<integer_cst>(GET_NODE(operation->op1));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, created bitstring from constant -> " + STR(const2->value));
      THROW_ASSERT(const2->value >= 0, "unexpected condition");
      THROW_ASSERT(output_bitstring.size() == 1, "unexpected condition");

      std::deque<bit_lattice> res_input1 = output_bitstring;
      const auto shift_value = static_cast<unsigned long long>(const2->value);
      for(unsigned long long shift_value_it = 0; shift_value_it < shift_value; shift_value_it++)
         res_input1.push_back(bit_lattice::X);
      const auto shifted_type_size = tree_helper::Size(tree_helper::CGetType(GET_NODE(operation->op0)));
      while(res_input1.size() < shifted_type_size)
         res_input1.push_front(bit_lattice::X);
      while(res_input1.size() > shifted_type_size)
         res_input1.pop_front();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " extract_bit_expr " + STR(GET_INDEX_NODE(operation->op1)));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));
      return res_input1;
   }
#endif
#if 1
   else if(op_kind == rrotate_expr_K)
   {
      auto* operation = GetPointer<rrotate_expr>(GET_NODE(ga->op1));

      if(GET_NODE(operation->op1)->get_kind() != integer_cst_K)
      {
         if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
         {
            bool op_signed_p = tree_helper::is_int(TM, output_id);
            unsigned int precision = BitLatticeManipulator::size(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op0)));
            unsigned int log2;
            for(log2 = 1; precision > (1u << log2); ++log2)
               ;
            std::deque<bit_lattice> res_input1 = current.at(GET_INDEX_NODE(operation->op1));
            for(unsigned int index = 0; res_input1.size() > index + log2; ++index)
            {
               if(op_signed_p && (res_input1.size() == index + log2 + 1))
                  res_input1[index] = bit_lattice::ZERO;
               else
                  res_input1[index] = bit_lattice::X;
            }
            return res_input1;
         }
         return std::deque<bit_lattice>();
      }
      return std::deque<bit_lattice>();
   }
   else if(op_kind == lrotate_expr_K)
   {
      auto* operation = GetPointer<lrotate_expr>(GET_NODE(ga->op1));

      if(GET_NODE(operation->op1)->get_kind() != integer_cst_K)
      {
         if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
         {
            bool op_signed_p = tree_helper::is_int(TM, output_id);
            unsigned int precision = BitLatticeManipulator::size(TM, tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op0)));
            unsigned int log2;
            for(log2 = 1; precision > (1u << log2); ++log2)
               ;
            std::deque<bit_lattice> res_input1 = current.at(GET_INDEX_NODE(operation->op1));
            for(unsigned int index = 0; res_input1.size() > index + log2; ++index)
            {
               if(op_signed_p && (res_input1.size() == index + log2 + 1))
                  res_input1[index] = bit_lattice::ZERO;
               else
                  res_input1[index] = bit_lattice::X;
            }
            return res_input1;
         }
         return std::deque<bit_lattice>();
      }
      return std::deque<bit_lattice>();
   }
#endif
#if 1
   else if(op_kind == nop_expr_K || op_kind == convert_expr_K || op_kind == view_convert_expr_K)
   {
      auto* operation = GetPointer<unary_expr>(GET_NODE(ga->op1));

      const auto left_id = GET_INDEX_NODE(ga->op0);
      const auto right_id = GET_INDEX_NODE(operation->op);

      const bool left_signed = tree_helper::is_int(TM, left_id);
      const bool right_signed = tree_helper::is_int(TM, right_id);

      const std::deque<bit_lattice>& left_bitstring = current.at(left_id);
#ifndef NDEBUG
      const std::deque<bit_lattice>& right_bitstring = best.at(right_id);

      const size_t right_bitsize = right_bitstring.size();
#endif

      const tree_nodeConstRef left_type = tree_helper::CGetType(GET_NODE(ga->op0));
      const tree_nodeConstRef right_type = tree_helper::CGetType(GET_NODE(operation->op));

      if(tree_helper::is_real(TM, left_type->index) and tree_helper::is_real(TM, right_type->index))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer Error: operation unhandled yet with real type operands -> " + GET_NODE(ga->op1)->get_kind_text());
         return std::deque<bit_lattice>();
      }

      const size_t left_type_size = tree_helper::Size(left_type);
      const size_t right_type_size = tree_helper::Size(right_type);

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(left_id) + (left_signed ? "S" : "U") + " = cast " + STR(right_id) + (right_signed ? "S" : "U"));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "operation sizes: " + STR(left_type_size) + (left_signed ? "S" : "U") + " = cast " + STR(right_type_size) + (right_signed ? "S" : "U"));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, (op_kind == nop_expr_K ? "cast " : "convert ") + bitstring_to_string(best.at(right_id)) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, (op_kind == nop_expr_K ? "     " : "        ") + bitstring_to_string(left_bitstring));
      if(right_signed and not left_signed)
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
         const auto r = best.at(right_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + std::string(op_kind != nop_expr_K ? "   " : "") + bitstring_to_string(r) + "(" + STR(right_bitsize) + "->" + STR(r.size()) + ")");
         return best.at(right_id);
      }

      std::deque<bit_lattice> res = left_bitstring;
      if(res.size() < left_type_size)
         res = sign_extend_bitstring(res, left_signed, left_type_size);
      if(left_type_size < right_type_size)
      {
         res.push_front(bit_lattice::X);
      }
      else if(left_type_size > right_type_size)
      {
         if(right_type_size < 32 and right_type_size > 1)
         {
            const auto sign_bit = res.front();
            res.pop_front();
            while(res.size() > right_type_size)
               res.pop_front();
            res.front() = bit_inf(sign_bit, res.front());
         }
         else
         {
            while(res.size() > right_type_size)
               res.pop_front();
         }
      }

      THROW_ASSERT(not tree_helper::is_real(TM, left_type->index) or res.size() == left_type_size, "Real type bit value should be exact");

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "res: " + std::string(op_kind != nop_expr_K ? "   " : "") + bitstring_to_string(res) + "(" + STR(right_bitsize) + "->" + STR(res.size()) + ")");
      return res;
   }
#endif
#if 1
   else if(op_kind == cond_expr_K)
   {
      auto* operation = GetPointer<cond_expr>(GET_NODE(ga->op1));

      unsigned int arg0_uid = GET_INDEX_NODE(operation->op0);
      if(arg0_uid == output_id)
         return std::deque<bit_lattice>();

      unsigned int arg1_uid = GET_INDEX_NODE(operation->op1);
      unsigned int arg2_uid = GET_INDEX_NODE(operation->op2);

      if(arg1_uid != output_id)
         std::swap(arg1_uid, arg2_uid);
      THROW_ASSERT(output_id == arg1_uid, "unexpected condition for statement " + ga->ToString());

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      const std::deque<bit_lattice>& arg1_bitstring = best.at(arg1_uid);

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(arg0_uid) + " - " + STR(arg1_uid) + " - " + STR(arg2_uid));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(output_bitstring));

      auto it_output_bitstring = output_bitstring.rbegin();
      auto it_arg1_bitstring = arg1_bitstring.rbegin();

      std::deque<bit_lattice> res_input1;

      for(; it_output_bitstring != output_bitstring.rend() && it_arg1_bitstring != arg1_bitstring.rend(); ++it_output_bitstring, ++it_arg1_bitstring)
      {
         if(*it_output_bitstring == bit_lattice::X)
         {
            res_input1.push_front(bit_lattice::X);
         }
         else
         {
            res_input1.push_front(*it_arg1_bitstring);
         }
      }
      if(res_input1.front() == bit_lattice::X && arg1_bitstring.size() < output_bitstring.size())
      {
         bit_lattice arg1_sign = arg1_bitstring.front();
         res_input1.pop_front();
         res_input1.push_front(arg1_sign);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(res_input1));

      return res_input1;
   }
#endif
#if 1
   else if(op_kind == ternary_plus_expr_K)
   {
      auto* operation = GetPointer<ternary_plus_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = 0;
      if(GET_INDEX_NODE(operation->op0) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op0);

      if(GET_INDEX_NODE(operation->op1) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op1);

      if(GET_INDEX_NODE(operation->op2) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op2);

      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      size_t initial_size = arg1_bitstring.size();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " + " + STR(GET_INDEX_NODE(operation->op1)) + " + " + STR(GET_INDEX_NODE(operation->op2)));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(output_bitstring));

      while(arg1_bitstring.size() > output_bitstring.size())
         arg1_bitstring.pop_front();
      if(arg1_bitstring.size() != initial_size)
         arg1_bitstring.push_front(bit_lattice::X);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
#endif
#if 1
   else if(op_kind == ternary_pm_expr_K)
   {
      auto* operation = GetPointer<ternary_pm_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = 0;
      if(GET_INDEX_NODE(operation->op0) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op0);

      if(GET_INDEX_NODE(operation->op1) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op1);

      if(GET_INDEX_NODE(operation->op2) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op2);

      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      size_t initial_size = arg1_bitstring.size();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " + " + STR(GET_INDEX_NODE(operation->op1)) + " - " + STR(GET_INDEX_NODE(operation->op2)));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(output_bitstring));

      while(arg1_bitstring.size() > output_bitstring.size())
         arg1_bitstring.pop_front();
      if(arg1_bitstring.size() != initial_size)
         arg1_bitstring.push_front(bit_lattice::X);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
#endif
#if 1
   else if(op_kind == ternary_mp_expr_K)
   {
      auto* operation = GetPointer<ternary_mp_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = 0;
      if(GET_INDEX_NODE(operation->op0) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op0);

      if(GET_INDEX_NODE(operation->op1) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op1);

      if(GET_INDEX_NODE(operation->op2) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op2);

      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      size_t initial_size = arg1_bitstring.size();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " - " + STR(GET_INDEX_NODE(operation->op1)) + " + " + STR(GET_INDEX_NODE(operation->op2)));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(output_bitstring));

      while(arg1_bitstring.size() > output_bitstring.size())
         arg1_bitstring.pop_front();
      if(arg1_bitstring.size() != initial_size)
         arg1_bitstring.push_front(bit_lattice::X);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
#endif
#if 1
   else if(op_kind == ternary_mm_expr_K)
   {
      auto* operation = GetPointer<ternary_mm_expr>(GET_NODE(ga->op1));

      unsigned int arg1_uid = 0;
      if(GET_INDEX_NODE(operation->op0) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op0);

      if(GET_INDEX_NODE(operation->op1) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op1);

      if(GET_INDEX_NODE(operation->op2) == output_id)
         arg1_uid = GET_INDEX_NODE(operation->op2);

      THROW_ASSERT(output_id == arg1_uid, "unexpected condition");

      if(best.find(arg1_uid) == best.end())
         return std::deque<bit_lattice>();

      std::deque<bit_lattice> arg1_bitstring = best.at(arg1_uid);
      size_t initial_size = arg1_bitstring.size();

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "backward_transfer, operation: " + STR(GET_INDEX_NODE(ga->op0)) + " = " + STR(GET_INDEX_NODE(operation->op0)) + " - " + STR(GET_INDEX_NODE(operation->op1)) + " - " + STR(GET_INDEX_NODE(operation->op2)));

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(arg1_bitstring) + " <=");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, bitstring_to_string(output_bitstring));

      while(arg1_bitstring.size() > output_bitstring.size())
         arg1_bitstring.pop_front();
      if(arg1_bitstring.size() != initial_size)
         arg1_bitstring.push_front(bit_lattice::X);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " input1: " + bitstring_to_string(arg1_bitstring));

      return arg1_bitstring;
   }
#endif
#if 1
   else if(op_kind == mem_ref_K)
   {
      auto* operation = GetPointer<mem_ref>(GET_NODE(ga->op1));

      if(GET_INDEX_NODE(operation->op0) == output_id && current.find(GET_INDEX_NODE(operation->op0)) != current.end())
      {
         return create_u_bitstring(pointer_resizing(output_id));
      }
      else
         return std::deque<bit_lattice>();
   }
   else if(op_kind == target_mem_ref461_K)
   {
      auto* operation = GetPointer<target_mem_ref461>(GET_NODE(ga->op1));

      if(GET_INDEX_NODE(operation->base) == output_id && current.find(GET_INDEX_NODE(operation->base)) != current.end())
      {
         return create_u_bitstring(pointer_resizing(output_id));
      }
      else if(operation->idx2 && GET_INDEX_NODE(operation->idx2) == output_id && current.find(GET_INDEX_NODE(operation->idx2)) != current.end())
      {
         return create_u_bitstring(pointer_resizing(output_id));
      }
      else
         return std::deque<bit_lattice>();
   }
   else if(op_kind == array_ref_K)
   {
      auto* operation = GetPointer<array_ref>(GET_NODE(ga->op1));
      do
      {
         if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
         {
            return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(operation->op0)));
         }
         operation = GetPointer<array_ref>(GET_NODE(operation->op0));
      } while(operation);
      return std::deque<bit_lattice>();
   }
   else if(op_kind == lut_expr_K)
   {
      return std::deque<bit_lattice>();
   }
   else if(op_kind == addr_expr_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Backward transfer of addr_expr");
      if(ga->temporary_address)
      {
         auto* operand = GetPointer<addr_expr>(GET_NODE(ga->op1));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Operand is " + GET_NODE(operand->op)->get_kind_text());
         op_kind = GET_NODE(operand->op)->get_kind();
         if(op_kind == mem_ref_K)
         {
            auto* operation = GetPointer<mem_ref>(GET_NODE(operand->op));

            if(GET_INDEX_NODE(operation->op0) == output_id && current.find(GET_INDEX_NODE(operation->op0)) != current.end())
            {
               return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(ga->op0)));
            }
            else
            {
               return std::deque<bit_lattice>();
            }
         }
         else if(op_kind == target_mem_ref461_K)
         {
            auto* operation = GetPointer<target_mem_ref461>(GET_NODE(operand->op));

            if(GET_INDEX_NODE(operation->base) == output_id && current.find(GET_INDEX_NODE(operation->base)) != current.end())
            {
               return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(ga->op0)));
            }
            else if(operation->idx2 && GET_INDEX_NODE(operation->idx2) == output_id && current.find(GET_INDEX_NODE(operation->idx2)) != current.end())
            {
               return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(ga->op0)));
            }
            else
            {
               return std::deque<bit_lattice>();
            }
         }
         else if(op_kind == array_ref_K)
         {
            auto* operation = GetPointer<array_ref>(GET_NODE(operand->op));
            do
            {
               if(GET_INDEX_NODE(operation->op1) == output_id && current.find(GET_INDEX_NODE(operation->op1)) != current.end())
               {
                  return create_u_bitstring(pointer_resizing(GET_INDEX_NODE(operation->op0)));
               }
               operation = GetPointer<array_ref>(GET_NODE(operation->op0));
            } while(operation);
            return std::deque<bit_lattice>();
         }
         else
            return std::deque<bit_lattice>();
      }
      else
         return std::deque<bit_lattice>();
   }
   else if(op_kind == lt_expr_K || op_kind == gt_expr_K || op_kind == le_expr_K || op_kind == ge_expr_K || op_kind == eq_expr_K || op_kind == ne_expr_K || op_kind == trunc_div_expr_K || op_kind == exact_div_expr_K || op_kind == trunc_mod_expr_K ||
           op_kind == min_expr_K || op_kind == max_expr_K)
   {
      return std::deque<bit_lattice>();
   }
#endif
#if 1
   else if(op_kind == call_expr_K || op_kind == aggr_init_expr_K)
   {
      std::deque<bit_lattice> res;
      const call_expr* call = GetPointer<call_expr>(GET_NODE(ga->op1));
      THROW_ASSERT(call, "not a call_expr");
      const auto call_it = direct_call_id_to_called_id.find(ga->index);
      if(call_it != direct_call_id_to_called_id.end())
      {
         const unsigned int called_id = call_it->second;
         const tree_nodeConstRef tn = TM->get_tree_node_const(called_id);
         const auto* fd = GetPointer<const function_decl>(tn);

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
            if(GET_INDEX_NODE(*a_it) == output_id)
            {
               const auto* pd = GetPointer<const parm_decl>(GET_NODE(*f_it));
               std::deque<bit_lattice> tmp;
               if(pd->bit_values.empty())
                  tmp = create_u_bitstring(tree_helper::Size(GET_NODE(pd->type)));
               else
                  tmp = string_to_bitstring(pd->bit_values);

               res = found ? inf(res, tmp, output_id) : tmp;
               found = true;
            }
         }
         THROW_ASSERT(found, STR(output_id) + " is not an actual parameter of function " + STR(called_id));
      }
      return res;
   }
#endif
   else
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "backward_transfer Error: operation unhandled yet -> " + GET_NODE(ga->op1)->get_kind_text());
   return std::deque<bit_lattice>();
}
