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
 *              Copyright (C) 2004-2019 Politecnico di Milano
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
 * @file Range_Analysis.cpp
 * @brief 
 *
 * @author Michele Fiorito <michele2.fiorito@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "Range_Analysis.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// stl
#include <map>
#include <sstream>
#include <vector>

/// Tree includes
#include "token_interface.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "ext_tree_node.hpp"
#include "tree_reindex.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "string_manipulation.hpp" // for GET_CLASS

#define RA_JUMPSET

#define DEBUG_RANGE_OP
#define DEBUG_BASICOP_EVAL
#define DEBUG_CGRAPH
#define DEBUG_RA
#define SCC_DEBUG

namespace RangeAnalysis
{
   using namespace boost::multiprecision;

   using eValue = std::pair<tree_nodeRef, tree_nodeRef>;
   using UAPInt = uint128_t;

   namespace 
   {
      // The number of bits needed to store the largest variable of the function (APInt).
      unsigned MAX_BIT_INT = 0;
      unsigned POINTER_BITSIZE = 32;

      // ========================================================================== //
      // Static global functions and definitions
      // ========================================================================== //

      APInt Min, Max;

      // Used to print pseudo-edges in the Constraint Graph dot
      std::string pestring;
      std::stringstream pseudoEdgesString(pestring);

      APInt getMaxValue(unsigned bitwidth)
      {
         return APInt((uint256_t(1) << (bitwidth + 1)) - 1);
      }

      APInt getMinValue(unsigned bitwidth)
      {
         return APInt(0);
      }

      APInt getSignedMaxValue(unsigned bitwidth)
      {
         return (APInt(1) << (bitwidth - 1)) - 1;
      }

      APInt getSignedMinValue(unsigned bitwidth)
      {
         return -(APInt(1) << (bitwidth - 1));
      }

      inline APInt ap_trunc(APInt x, unsigned bitwidth)
      {
         bitwidth = (1 << bitwidth) - 1;
         return x & bitwidth;
      }

      inline UAPInt ap_trunc(UAPInt x, unsigned bitwidth)
      {
         bitwidth = (1 << bitwidth) - 1;
         return x & bitwidth;
      }

      APInt lshr(const APInt& a, unsigned p)
      {
         APInt mask = (APInt(1) << (128 - p)) - 1;
         return (a >> p) & mask;
      }

      UAPInt lshr(const UAPInt& a, unsigned p)
      {
         if(p > 128)
         {
            return 0;
         }
         UAPInt mask = (UAPInt(1) << (128 - p)) - 1;
         return (a >> p) & mask;
      }

      unsigned countLeadingZeros(const APInt& a)
      {
         int i = 127;
         for(; i >= 0; --i)
         {
            if(bit_test(a, i))
            {
                  break;
            }
         }
         i++;
         return 128 - i;
      }

      unsigned countLeadingOnes(const APInt& a)
      {
         int i = 127;
         for(; i >= 0; --i)
         {
            if(!bit_test(a, i))
            {
                  break;
            }
         }
         i++;
         return 128 - i;
      }

      kind op_inv(kind op)
      {
         switch (op)
         {
         case ge_expr_K:
            return lt_expr_K;
         case gt_expr_K:
            return le_expr_K;
         case unge_expr_K:
            return unlt_expr_K;
         case ungt_expr_K:
            return unle_expr_K;
         case unle_expr_K:
            return ungt_expr_K;
         case unlt_expr_K:
            return unge_expr_K;
         case eq_expr_K:
            return ne_expr_K;
         case ne_expr_K:
            return eq_expr_K;
         case uneq_expr_K:
            return ne_expr_K;
         
         case assert_expr_K:
         case catch_expr_K:
         case ceil_div_expr_K:
         case ceil_mod_expr_K:
         case complex_expr_K:
         case compound_expr_K:
         case eh_filter_expr_K:
         case exact_div_expr_K:
         case fdesc_expr_K:
         case floor_div_expr_K:
         case floor_mod_expr_K:
         case goto_subroutine_K:
         case in_expr_K:
         case init_expr_K:
         case le_expr_K:
         case lrotate_expr_K:
         case lshift_expr_K:
         case lt_expr_K:
         case max_expr_K:
         case mem_ref_K:
         case min_expr_K:
         case minus_expr_K:
         case modify_expr_K:
         case mult_expr_K:
         case mult_highpart_expr_K:
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
         case ltgt_expr_K:
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
         default:
            break;
         }

         THROW_UNREACHABLE("Unhandled predicate (" + boost::lexical_cast<std::string>(op) + ")");
      }

      kind op_swap(kind op)
      {
         switch (op)
         {
         case ge_expr_K:
            return le_expr_K;
         case gt_expr_K:
            return lt_expr_K;
         case le_expr_K:
            return ge_expr_K;
         case lt_expr_K:
            return gt_expr_K;
         case unge_expr_K:
            return unle_expr_K;
         case ungt_expr_K:
            return unlt_expr_K;
         case unle_expr_K:
            return unge_expr_K;
         case unlt_expr_K:
            return ungt_expr_K;

         case bit_and_expr_K:
         case bit_xor_expr_K:
         case eq_expr_K:
         case ne_expr_K:
         case uneq_expr_K:
            return op;
         
         case assert_expr_K:
         case bit_ior_expr_K:
         case catch_expr_K:
         case ceil_div_expr_K:
         case ceil_mod_expr_K:
         case complex_expr_K:
         case compound_expr_K:
         case eh_filter_expr_K:
         case exact_div_expr_K:
         case fdesc_expr_K:
         case floor_div_expr_K:
         case floor_mod_expr_K:
         case goto_subroutine_K:
         case in_expr_K:
         case init_expr_K:
         case lrotate_expr_K:
         case lshift_expr_K:
         case max_expr_K:
         case mem_ref_K:
         case min_expr_K:
         case minus_expr_K:
         case modify_expr_K:
         case mult_expr_K:
         case mult_highpart_expr_K:
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
         case ltgt_expr_K:
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
         default:
            break;
         }

         THROW_UNREACHABLE("Unhandled predicate (" + boost::lexical_cast<std::string>(op) + ")");
      }

      bool isCompare(kind c_type)
      {
         return c_type == eq_expr_K || c_type == ne_expr_K || c_type == ltgt_expr_K || c_type == uneq_expr_K
            || c_type == gt_expr_K || c_type == lt_expr_K || c_type == ge_expr_K || c_type == le_expr_K 
            || c_type == unlt_expr_K || c_type == ungt_expr_K || c_type == unle_expr_K || c_type == unge_expr_K;
      }

      bool isCompare(struct binary_expr* condition)
      {
         return isCompare(condition->get_kind());
      }

      tree_nodeRef branchOpRecurse(tree_nodeRef op)
      {
         const auto Op = GET_NODE(op);
         if(auto* nop = GetPointer<nop_expr>(Op))
         {
            return branchOpRecurse(nop->op);
         }
         else if(auto* ssa = GetPointer<ssa_name>(Op))
         {
            auto* def = GetPointer<gimple_assign>(GET_NODE(ssa->CGetDefStmt()));

            return branchOpRecurse(def->op1);
         }
         return op;
      }

      // Print name of variable according to its type
      void printVarName(tree_nodeRef V, std::ostream& OS)
      {
         OS << V->ToString();

         // TODO: implement a better way to correctly print significant name

         //const Argument* A = nullptr;
         //const Instruction* I = nullptr;
         //
         //if((A = dyn_cast<Argument>(V)) != nullptr)
         //{
         //   const llvm::Function* currentFunction = A->getParent();
         //   llvm::ModuleSlotTracker MST(currentFunction->getParent());
         //   MST.incorporateFunction(*currentFunction);
         //   auto argID = MST.getLocalSlot(A);
         //   OS << A->getParent()->getName() << ".%" << argID;
         //}
         //else if((I = dyn_cast<Instruction>(V)) != nullptr)
         //{
         //   llvm::ModuleSlotTracker MST(I->getParent()->getParent()->getParent());
         //   MST.incorporateFunction(*I->getParent()->getParent());
         //   auto instID = MST.getLocalSlot(I);
         //   auto BBID = MST.getLocalSlot(I->getParent());
         //
         //   OS << I->getFunction()->getName() << ".BB" << BBID;
         //   if(instID < 0)
         //   {
         //      I->print(OS);
         //   }
         //   else
         //   {
         //      OS << ".%" << instID;
         //   }
         //}
         //else
         //{
         //   OS << V->getName();
         //}
      }

      bool isValidInstruction(tree_nodeRef stmt)
      {
         switch(GET_NODE(stmt)->get_kind())
         {
            case gimple_assign_K:   // TODO: refine check looking for supported operations only (add, sub, mul, ...)
            case gimple_phi_K:
               return true;

            case gimple_asm_K:
            case gimple_bind_K:
            case gimple_call_K:
            case gimple_cond_K:
            case gimple_for_K:
            case gimple_goto_K:
            case gimple_label_K:
            case gimple_multi_way_if_K:
            case gimple_nop_K:
            case gimple_pragma_K:
            case gimple_predict_K:
            case gimple_resx_K:
            case gimple_return_K:
            case gimple_switch_K:
            case gimple_while_K:
            default:
               return false;
         }
      }

      unsigned getGIMPLE_BW(const struct gimple_node* I)
      {
         switch (I->get_kind())
         {
         case gimple_assign_K:
            {
               const auto* ga = reinterpret_cast<const struct gimple_assign*>(I);
               auto* var = GetPointer<ssa_name>(GET_NODE(ga->op0));
               if(GET_NODE(ga->op0)->get_kind() == mem_ref_K) // Maybe it's a store operation
               {
                  var = GetPointer<ssa_name>(GET_NODE(ga->op1));
               }
               THROW_ASSERT(var, "Could not read gimple assign variable");
               struct type_node* var_type = GetPointer<type_node>(GET_NODE(var->type));
               auto* var_size = GetPointer<integer_cst>(GET_NODE(var_type->size));

               return var_size->value;
            }
            break;
         case gimple_cond_K:
            {
               const auto* gc = reinterpret_cast<const struct gimple_cond*>(I);
               auto* cond_var = GetPointer<ssa_name>(GET_NODE(gc->op0));
               THROW_ASSERT(cond_var, "Could not read branch variable");
               auto* cond_var_type = GetPointer<type_node>(GET_NODE(cond_var->type));
               auto* cond_var_size = GetPointer<integer_cst>(GET_NODE(cond_var_type->size));

               return cond_var_size->value;
            }
            break;

         case gimple_phi_K:
            {
               const auto* gp = reinterpret_cast<const struct gimple_phi*>(I);
               auto* var = GetPointer<ssa_name>(GET_NODE(gp->res));
               THROW_ASSERT(var, "Could not read phi variable");
               auto* var_type = GetPointer<type_node>(GET_NODE(var->type));
               auto* var_size = GetPointer<integer_cst>(GET_NODE(var_type->size));

               return var_size->value;
            }
            break;

         case gimple_return_K:
            {
               const auto* gr = reinterpret_cast<const struct gimple_return*>(I);
               auto* var = GetPointer<ssa_name>(GET_NODE(gr->op));
               THROW_ASSERT(var, "Could not read return variable");
               auto* var_type = GetPointer<type_node>(GET_NODE(var->type));
               auto* var_size = GetPointer<integer_cst>(GET_NODE(var_type->size));

               return var_size->value;
            }
            break;

         case gimple_bind_K:
         case gimple_call_K:
         case gimple_for_K:
         case gimple_goto_K:
         case gimple_label_K:
         case gimple_multi_way_if_K:
         case gimple_nop_K:
         case gimple_pragma_K:
         case gimple_predict_K:
         case gimple_resx_K:
         case gimple_switch_K:
         case gimple_while_K:
         default:
            break;
         }

         THROW_UNREACHABLE("Unhandled gimple statement (" + I->get_kind_text() + ")");
      }

      Range getGIMPLE_range(const struct gimple_node* I, application_managerRef /*AppM*/)
      {
         unsigned bw = getGIMPLE_BW(I);

         return Range(Regular, bw);
      }

      unsigned ObjectBW(tree_nodeRef var)
      {
         THROW_ASSERT(var, "Unexpected nullptr");

         THROW_UNREACHABLE("Unhandled var type (" + GET_NODE(var)->get_kind_text() + ")");

         return 0;
      }

      unsigned LoadStoreOperationBW(tree_nodeRef V, tree_nodeRef GV)
      {
         const auto _V = GET_NODE(V);
         if(GV)
         {
            return ObjectBW(GV);
         }
         if(auto* param = GetPointer<parm_decl>(_V))
         {
            auto* size = GetPointer<integer_cst>(GET_NODE(param->size));
            return size->value;
         }
         if(auto* ssa = GetPointer<ssa_name>(_V))
         {
            auto* type = GetPointer<type_node>(GET_NODE(ssa->type));
            auto* size = GetPointer<integer_cst>(GET_NODE(type->size));
            return size->value;
         }
         if(auto* cst = GetPointer<cst_node>(_V))
         {
            auto* type = GetPointer<type_node>(GET_NODE(cst->type));
            auto* size = GetPointer<integer_cst>(GET_NODE(type->size));
            return size->value;
         }
         if(auto* stmt = GetPointer<gimple_node>(_V))
         {
            return getGIMPLE_BW(stmt);
         }
         //if(auto IV = dyn_cast<InsertValueInst>(V))
         //{
         //   return IV->getInsertedValueOperand()->getType()->isPointerTy() ? POINTER_BITSIZE : IV->getInsertedValueOperand()->getType()->getPrimitiveSizeInBits();
         //}

         PRINT_MSG(V->get_kind_text() << std::endl << V->ToString());
         if(GV)
         {
            PRINT_MSG(GV->ToString());
         }
         THROW_UNREACHABLE("Unhandled type (" + _V->get_kind_text() + ")");
      }
   }

   // ========================================================================== //
   // Range
   // ========================================================================== //
   Range_base::Range_base(RangeType rType, unsigned rbw) : l(Min), u(Max), bw(rbw), type(rType)
   {
      THROW_ASSERT(rbw, "Invalid bitwidth for range");
   }

   void Range_base::normalizeRange(const APInt& lb, const APInt& ub, RangeType rType)
   {
      if(rType == Empty || rType == Unknown)
      {
         l = Min;
         u = Max;
      }
      else if(rType == Anti)
      {
         if(lb > ub)
         {
            type = Regular;
            l = Min;
            u = Max;
         }
         else
         {
            if((lb == Min) && (ub == Max))
            {
               type = Empty;
            }
            else if(lb == Min)
            {
               type = Regular;
               l = ub + 1;
               u = Max;
            }
            else if(ub == Max)
            {
               type = Regular;
               l = Min;
               u = lb - 1;
            }
            else
            {
               THROW_ASSERT(ub >= lb, "");
               auto maxS = getSignedMaxValue(bw);
               auto minS = getSignedMinValue(bw);
               bool lbgt = lb >maxS;
               bool ubgt = ub > maxS;
               bool lblt = lb < minS;
               bool ublt = ub < minS;
               if(lbgt && ubgt)
               {
                  l = ap_trunc(lb, bw);
                  u = ap_trunc(ub, bw);
               }
               else if(lblt && ublt)
               {
                  l = ap_trunc(ub, bw);
                  u = ap_trunc(lb, bw);
               }
               else if(!lblt && ubgt)
               {
                  auto ubnew = ap_trunc(ub, bw);
                  if(ubnew >= (lb - 1))
                  {
                     l = Min;
                     u = Max;
                  }
                  else
                  {
                     type = Regular;
                     l = ubnew + 1;
                     u = lb - 1;
                  }
               }
               else if(lblt && !ubgt)
               {
                  auto lbnew = ap_trunc(lb, bw);
                  if(lbnew <= (ub + 1))
                  {
                     l = Min;
                     u = Max;
                  }
                  else
                  {
                     type = Regular;
                     l = ub + 1;
                     u = lbnew - 1;
                  }
               }
               else if(!lblt && !ubgt)
               {
                  l = lb;
                  u = ub;
               }
               else
               {
                  THROW_UNREACHABLE("unexpected condition");
               }
            }
         }
      }
      else if((lb - 1) == ub)
      {
         type = Regular;
         l = Min;
         u = Max;
      }
      else if(lb > ub)
      {
         normalizeRange(ub + 1, lb - 1, Anti);
      }
      else
      {
         THROW_ASSERT(ub >= lb, "");
         auto maxS = getSignedMaxValue(bw);
         auto minS = getSignedMinValue(bw);

         bool lbgt = lb > maxS;
         bool ubgt = ub > maxS;
         bool lblt = lb < minS;
         bool ublt = ub < minS;
         if(ubgt && lblt)
         {
            l = Min;
            u = Max;
         }
         else if(lbgt && ubgt)
         {
            l = ap_trunc(lb, bw);
            u = ap_trunc(ub, bw);
         }
         else if(lblt && ublt)
         {
            l = ap_trunc(ub, bw);
            u = ap_trunc(lb, bw);
         }
         else if(!lblt && ubgt)
         {
            auto ubnew = ap_trunc(ub, bw);
            if(ubnew >= (lb - 1))
            {
               l = Min;
               u = Max;
            }
            else
            {
               type = Anti;
               l = ubnew + 1;
               u = lb - 1;
            }
         }
         else if(lblt && !ubgt)
         {
            auto lbnew = ap_trunc(lb, bw);
            if(lbnew <= (ub + 1))
            {
               l = Min;
               u = Max;
            }
            else
            {
               type = Anti;
               l = ub + 1;
               u = lbnew - 1;
            }
         }
         else if(!lblt && !ubgt)
         {
            l = lb;
            u = ub;
         }
         else
         {
            THROW_UNREACHABLE("unexpected condition");
         }
      }
      if(!(u >= l))
      {
         l = Min;
         u = Max;
      }
   }

   unsigned Range_base::neededBits(const APInt& a, const APInt& b, bool sign)
   {
      auto max_active_bits = [](const APInt& x, const APInt& y)
      {
         size_t i = 128;
         while(i > 0)
         {
            --i;
            if(bit_test(x, i) || bit_test(y, i))
            {
               break;
            }
         }

         return i;
      };

      if(sign)
      {
         auto a_u = a.sign() == -1 ? -a : a;
         auto b_u = b.sign() == -1 ? -b : b;

         return max_active_bits(a_u, b_u);
      }

      return max_active_bits(a, b);
   }

   Range_base::Range_base(RangeType rType, unsigned rbw, const APInt& lb, const APInt& ub) : l(lb), u(ub), bw(rbw), type(rType)
   {
      THROW_ASSERT(rbw, "Invalid bitwidth for range");
      normalizeRange(lb, ub, rType);
   }

   Range_base Range_base::getAnti(const Range_base& o)
   {
      if(o.type == Anti)
      {
         return Range_base(Regular, o.bw, o.l, o.u);
      }
      if(o.type == Regular)
      {
         return Range_base(Anti, o.bw, o.l, o.u);
      }
      if(o.type == Empty)
      {
         return Range_base(Regular, o.bw, Min, Max);
      }
      if(o.type == Unknown)
      {
         return o;
      }

      THROW_UNREACHABLE("unexpected condition");
   }

   unsigned int Range_base::getBitWidth() const
   {
      return bw;
   }

   const APInt Range_base::getLower() const
   {
      THROW_ASSERT(type != Anti, "Lower bound not valid for Anti range");
      return l;
   }

   const APInt Range_base::getUpper() const
   {
      THROW_ASSERT(type != Anti, "Upper bound not valid for Anti range");
      return u;
   }

   const APInt Range_base::getSignedMax() const
   {
      THROW_ASSERT(type != Unknown && type != Empty, "Max not valid for Unknown/Empty range");
      if(type == Regular)
      {
         auto maxS = getSignedMaxValue(bw);
         if(u > maxS)
         {
            return maxS;
         }

         return ap_trunc(u, bw);
      }

      return getSignedMaxValue(bw);
   }
   const APInt Range_base::getSignedMin() const
   {
      THROW_ASSERT(type != Unknown && type != Empty, "Min not valid for Unknown/Empty range");
      if(type != Anti)
      {
         auto minS = getSignedMinValue(bw);
         if(l < minS)
         {
            return minS;
         }

         return ap_trunc(l, bw);
      }

      return getSignedMinValue(bw);
   }
   const APInt Range_base::getUnsignedMax() const
   {
      THROW_ASSERT(type != Unknown && type != Empty, "UMax not valid for Unknown/Empty range");
      if(type == Regular)
      {
         if((l.sign() == -1) && (u.sign() == -1))
         {
            return ap_trunc(u, bw);
         }
         if(l < 0)
         {
            return getMaxValue(bw);
         }

         return ap_trunc(u, bw);
      }

      auto lb = ap_trunc(l, bw) - 1;
      auto ub = ap_trunc(u, bw) + 1;

      if((lb >= 0) || (ub < 0))
      {
         return getMaxValue(bw);
      }

      return lb;
   }
   const APInt Range_base::getUnsignedMin() const
   {
      THROW_ASSERT(type != Unknown && type != Empty, "UMin not valid for Unknown/Empty range");
      if(type == Regular)
      {
         if((l.sign() == -1) && (u.sign() == -1))
         {
            return ap_trunc(l, bw);
         }
         if(l < 0)
         {
            return 0;
         }

         return ap_trunc(l, bw);
      }

      auto lb = ap_trunc(l, bw) - 1;
      auto ub = ap_trunc(u, bw) + 1;

      if((lb >= 0) || (ub < 0))
      {
         return 0;
      }

      return ub;
   }

   bool Range::isFullSet() const
   {
      if(isAnti())
      {
         return false;
      }
      unsigned bw = getBitWidth();
      return (getSignedMaxValue(bw) <= getUpper() && getSignedMinValue(bw) >= getLower())
         || (getMaxValue(bw) <= getUpper() && getMinValue(bw) >= getLower());
   }

   bool Range::isMaxRange() const
   {
      if(isAnti())
      {
         return false;
      }
      return (this->getLower() == Min) && (this->getUpper() == Max);
   }
   bool Range::isConstant() const
   {
      if(isAnti())
      {
         return false;
      }
      return this->getLower() == this->getUpper();
   }

   /// Add and Mul are commutative. So, they are a little different
   /// than the other operations.
   /// Many Range reductions are done by exploiting ConstantRange code
   Range Range::add(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() && other.isConstant())
      {
         auto antiThis = getAnti(*this);
         auto l = antiThis.getLower();
         auto u = antiThis.getUpper();
         auto ol = other.getLower();
         if(ol >= (Max - u))
         {
            return Range(Regular, getBitWidth());
         }

         return Range(Anti, getBitWidth(), l + ol, u + ol);
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other.isConstant())
      {
         auto l = this->getLower();
         auto u = this->getUpper();
         auto ol = other.getLower();
         if(l == Min)
         {
            THROW_ASSERT(u != Max, "");
            return Range(Regular, getBitWidth(), l, u + ol);
         }
         if(u == Max)
         {
            THROW_ASSERT(l != Min, "");
            return Range(Regular, getBitWidth(), l + ol, u);
         }

         return Range(Regular, getBitWidth(), l + ol, u + ol);
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      Range thisR = (this_min == (this_max + 1)) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      Range thisUR = (thisU_min == thisU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), thisU_min, thisU_max + 1);
      Range otherUR = (otherU_min == otherU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), otherU_min, otherU_max + 1);
      //ConstantRange thisUR = (thisU_min == thisU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(thisU_min, thisU_max + 1);
      //ConstantRange otherUR = (otherU_min == otherU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(otherU_min, otherU_max + 1);

      auto AddU = Range(Regular, getBitWidth(), thisUR.getLower() + otherUR.getLower(), thisUR.getUpper() + otherUR.getUpper());
      auto AddS = Range(Regular, getBitWidth(), thisR.getLower() + otherR.getLower(), thisR.getUpper() + otherR.getUpper());
      
      return BestRange(AddU, AddS, getBitWidth());
   }

   Range Range::sub(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() && other.isConstant())
      {
         auto antiThis = getAnti(*this);
         auto l = antiThis.getLower();
         auto u = antiThis.getUpper();
         auto ol = other.getLower();
         if(l <= (Min + ol))
         {
            return Range(Regular, getBitWidth());
         }

         return Range(Anti, getBitWidth(), l - ol, u - ol);
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other.isConstant())
      {
         auto l = this->getLower();
         auto u = this->getUpper();
         auto ol = other.getLower();
         if(l == Min)
         {
            THROW_ASSERT(u != Max, "");
            auto bw = getBitWidth();
            auto minValue = getSignedMinValue(bw);
            auto upper = u - ol;
            if(minValue > upper)
            {
               return Range(Regular, bw);
            }

            return Range(Regular, bw, l, upper);
         }
         if(u == Max)
         {
            THROW_ASSERT(l != Min, "");
            auto bw = getBitWidth();
            auto maxValue = getSignedMaxValue(bw);
            auto lower = l - ol;
            if(maxValue < lower)
            {
               return Range(Regular, bw);
            }

            return Range(Regular, getBitWidth(), l - ol, u);
         }

         auto bw = getBitWidth();
         auto lower = ap_trunc(l - ol, bw);
         auto upper = ap_trunc(u - ol, bw);
         if(lower <= upper)
         {
            return Range(Regular, bw, lower, upper);
         }

         return Range(Anti, bw, upper + 1, lower - 1);
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      auto other_min = other.getSignedMin();
      auto other_max = other.getSignedMax();
      Range thisR = (this_min == (this_max + 1)) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      auto thisU_min = getUnsignedMin();
      auto thisU_max = getUnsignedMax();
      auto otherU_min = other.getUnsignedMin();
      auto otherU_max = other.getUnsignedMax();
      Range thisUR = (thisU_min == thisU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), thisU_min, thisU_max + 1);
      Range otherUR = (otherU_min == otherU_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), otherU_min, otherU_max + 1);
      //ConstantRange thisUR = (thisU_min == thisU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(thisU_min, thisU_max + 1);
      //ConstantRange otherUR = (otherU_min == otherU_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(otherU_min, otherU_max + 1);

      auto SubU = Range(Regular, getBitWidth(), thisUR.getLower() - otherUR.getUpper(), thisUR.getUpper() - otherUR.getLower());
      auto SubS = Range(Regular, getBitWidth(), thisR.getLower() - otherR.getUpper(), thisR.getUpper() - otherR.getLower());

      return BestRange(SubU, SubS, getBitWidth());
   }

   Range Range::mul(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      // Multiplication is signedness-independent. However different ranges can be
      // obtained depending on how the input ranges are treated. These different
      // ranges are all conservatively correct, but one might be better than the
      // other. We calculate two ranges; one treating the inputs as unsigned
      // and the other signed, then return the smallest of these ranges.

      // Unsigned range first.
      APInt this_min = getUnsignedMin();
      APInt this_max = getUnsignedMax();
      APInt Other_min = other.getUnsignedMin();
      APInt Other_max = other.getUnsignedMax();

      Range Result_zext = Range(Regular, getBitWidth() * 2, this_min * Other_min, this_max * Other_max + 1);
      Range UR = Result_zext.truncate(getBitWidth());

      // Now the signed range. Because we could be dealing with negative numbers
      // here, the lower bound is the smallest of the Cartesian product of the
      // lower and upper ranges; for example:
      //   [-1,4) * [-2,3) = min(-1*-2, -1*2, 3*-2, 3*2) = -6.
      // Similarly for the upper bound, swapping min for max.

      this_min = getSignedMin();
      this_max = getSignedMax();
      Other_min = other.getSignedMin();
      Other_max = other.getSignedMax();

      auto L = {this_min * Other_min, this_min * Other_max, this_max * Other_min, this_max * Other_max};
      auto Compare = [](const APInt& A, const APInt& B) { return A < B; };
      Range Result_sext(Regular, getBitWidth() * 2, std::min(L, Compare), std::max(L, Compare) + 1);
      Range SR = Result_sext.truncate(getBitWidth());
      return BestRange(UR, SR, getBitWidth());
   }

#define DIV_HELPER(x, y) (x == Max) ? ((y < 0) ? Min : ((y == 0) ? 0 : Max)) : ((y == Max) ? 0 : ((x == Min) ? ((y < 0) ? Max : ((y == 0) ? 0 : Min)) : ((y == Min) ? 0 : ((x) / (y)))))

   Range Range::udiv(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("this: " << *this << std::endl << "other: " << other);
      #endif
      UAPInt a(getUnsignedMin());
      UAPInt b(getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());

      // Deal with division by 0 exception
      if((c == 0) && (d == 0))
      {
         return Range(Regular, getBitWidth(), Min, Max);
      }
      if(c == 0)
      {
         c = 1;
      }

      APInt candidates[4];
      candidates[0] = DIV_HELPER(a, c);
      candidates[1] = DIV_HELPER(a, d);
      candidates[2] = DIV_HELPER(b, c);
      candidates[3] = DIV_HELPER(b, d);
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 4; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      return Range(Regular, getBitWidth(), *min, *max);
   }

   Range Range::sdiv(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      APInt c1, d1, c2, d2;
      bool is_zero_in = false;
      if(other.isAnti())
      {
         auto antiRange = getAnti(other);
         c1 = Min;
         d1 = antiRange.getLower() - 1;
         if(d1 == 0)
         {
            d1 = -1;
         }
         else if(d1 > 0)
         {
            return Range(Regular, getBitWidth()); /// could be improved
         }
         c2 = antiRange.getUpper() + 1;
         if(c2 == 0)
         {
            c2 = 1;
         }
         else if(c2 < 0)
         {
            return Range(Regular, getBitWidth()); /// could be improved
         }
         d2 = Max;
      }
      else
      {
         c1 = other.getLower();
         d1 = other.getUpper();
         // Deal with division by 0 exception
         if((c1 == 0) && (d1 == 0))
         {
            return Range(Regular, getBitWidth(), Min, Max);
         }
         is_zero_in = (c1 < 0) && (d1 > 0);
         if(is_zero_in)
         {
            d1 = -1;
            c2 = 1;
         }
         else
         {
            c2 = other.getLower();
            if(c2 == 0)
            {
               c1 = c2 = 1;
            }
         }
         d2 = other.getUpper();
         if(d2 == 0)
         {
            d1 = d2 = -1;
         }
      }
      auto n_iters = (is_zero_in || other.isAnti()) ? 8u : 4u;

      APInt candidates[8];
      candidates[0] = DIV_HELPER(a, c1);
      candidates[1] = DIV_HELPER(a, d1);
      candidates[2] = DIV_HELPER(b, c1);
      candidates[3] = DIV_HELPER(b, d1);
      if(n_iters == 8)
      {
         candidates[4] = DIV_HELPER(a, c2);
         candidates[5] = DIV_HELPER(a, d2);
         candidates[6] = DIV_HELPER(b, c2);
         candidates[7] = DIV_HELPER(b, d2);
      }
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < n_iters; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      return Range(Regular, getBitWidth(), *min, *max);
   }

   Range Range::urem(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt& a = this->getUnsignedMin();
      const APInt& b = this->getUnsignedMax();
      APInt c = other.getUnsignedMin();
      const APInt& d = other.getUnsignedMax();

      // Deal with mod 0 exception
      if((c == 0) && (d == 0))
      {
         return Range(Regular, getBitWidth());
      }
      if(c == 0)
      {
         c = 1;
      }
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("this-urem: " << *this << std::endl << "other-urem: " << other);
      #endif

      APInt candidates[8];

      candidates[0] = a < c ? a : 0;
      candidates[1] = a < d ? a : 0;
      candidates[2] = b < c ? b : 0;
      candidates[3] = b < d ? b : 0;
      candidates[4] = a < c ? a : c - 1;
      candidates[5] = a < d ? a : d - 1;
      candidates[6] = b < c ? b : c - 1;
      candidates[7] = b < d ? b : d - 1;

      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 8; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      auto res = Range(Regular, getBitWidth(), *min, *max);
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("res-urem: " << res << std::endl);
      #endif
      return res;
   }

   Range Range::srem(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      if(other == Range(Regular, other.getBitWidth(), 0, 0))
      {
         return Range(Empty, getBitWidth());
      }

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      APInt c = other.getLower();
      const APInt& d = other.getUpper();

      // Deal with mod 0 exception
      if((c == 0) && (d == 0))
      {
         return Range(Regular, getBitWidth(), Min, Max);
      }
      if(c == 0)
      {
         c = 1;
      }
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("this-rem: " << *this << std::endl << "other-rem: " << other);
      #endif

      APInt candidates[4];
      candidates[0] = Min;
      candidates[1] = Min;
      candidates[2] = Max;
      candidates[3] = Max;
      if((a != Min) && (c != Min))
      {
         candidates[0] = a & c; // lower lower
      }
      if((a != Min) && (d != Max))
      {
         candidates[1] = a % d; // lower upper
      }
      if((b != Max) && (c != Min))
      {
         candidates[2] = b % c; // upper lower
      }
      if((b != Max) && (d != Max))
      {
         candidates[3] = b % d; // upper upper
      }
      // Lower bound is the min value from the vector, while upper bound is the max value
      APInt* min = &candidates[0];
      APInt* max = &candidates[0];
      for(unsigned i = 1; i < 4; ++i)
      {
         if(candidates[i] > *max)
         {
            max = &candidates[i];
         }
         else if(candidates[i] < *min)
         {
            min = &candidates[i];
         }
      }
      auto res = Range(Regular, getBitWidth(), *min, *max);
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("res-rem: " << res << std::endl);
      #endif

      return res;
   }

   // Logic has been borrowed from ConstantRange
   Range Range::shl(const Range& other) const
   {
      if(isEmpty() || isUnknown() || isMaxRange())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown() || other.isMaxRange())
      {
         return other;
      }
      auto bw = getBitWidth();
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, bw);
      }
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("this-shl: " << *this << std::endl << "other-shl: " << other);
      #endif

      const APInt& a = this->getLower();
      const APInt& b = this->getUpper();
      const UAPInt c(other.getUnsignedMin());
      const UAPInt d(other.getUnsignedMax());
      UAPInt Prec = bw;

      if(c >= Prec)
      {
         return Range(Regular, bw);
      }
      if(d >= Prec)
      {
         return Range(Regular, bw);
      }
      if((a == Min) || (b == Max))
      {
         return Range(Regular, bw);
      }
      if((a == b) && (c == d)) // constant case
      {
         auto minmax = ap_trunc(a, bw) << (size_t)ap_trunc(c, bw).convert_to<long>();
         minmax = ap_trunc(minmax, MAX_BIT_INT);
         return Range(Regular, bw, minmax, minmax);
      }
      if((a.sign() == -1) && (b.sign() == -1))
      {
         UAPInt clOnes(countLeadingOnes(a) - (MAX_BIT_INT - getBitWidth()));
         if(d > clOnes)
         { // overflow
            return Range(Regular, bw);
         }

         return Range(Regular, getBitWidth(), a << (size_t)d.convert_to<long>(), b << (size_t)c.convert_to<long>());
      }
      if((a < 0) && (b >= 0))
      {
         UAPInt clOnes(countLeadingOnes(a) - (MAX_BIT_INT - getBitWidth()));
         UAPInt clZeros(countLeadingZeros(b) - (MAX_BIT_INT - getBitWidth()));
         if(d > clOnes || d > clZeros)
         { // overflow
            return Range(Regular, bw);
         }

         return Range(Regular, getBitWidth(), a << (size_t)d.convert_to<long>(), b << (size_t)d.convert_to<long>());
      }

      UAPInt clZeros(countLeadingZeros(b) - (MAX_BIT_INT - getBitWidth()));
      if(d > clZeros)
      { // overflow
         return Range(Regular, bw);
      }

      return Range(Regular, getBitWidth(), a << (size_t)c.convert_to<long>(), b << (size_t)d.convert_to<long>());
   }

   Range Range::lshr(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      UAPInt this_min(getUnsignedMin());
      UAPInt this_max(getUnsignedMax());
      UAPInt other_min(other.getUnsignedMin());
      UAPInt other_max(other.getUnsignedMax());
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      const Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //const ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);

      // TODO: check correctness
      auto lshrU = Range(Regular, getBitWidth(), 
         RangeAnalysis::lshr(thisR.getLower(), (unsigned)otherR.getUpper().convert_to<long>()), 
         RangeAnalysis::lshr(thisR.getUpper(), (unsigned)otherR.getLower().convert_to<long>()));
      if(lshrU.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }

      return lshrU;
   }

   static Range local_ashr(const Range& lthis, const Range& Other)
   {
      if(lthis.isEmpty() || Other.isEmpty())
      {
         return Range(Empty, lthis.getBitWidth());
      }

      // May straddle zero, so handle both positive and negative cases.
      // 'PosMax' is the upper bound of the result of the ashr
      // operation, when Upper of the LHS of ashr is a non-negative.
      // number. Since ashr of a non-negative number will result in a
      // smaller number, the Upper value of LHS is shifted right with
      // the minimum value of 'Other' instead of the maximum value.
      APInt PosMax = lthis.getSignedMax() >> ((size_t)Other.getUnsignedMin().convert_to<long>() + 1);

      // 'PosMin' is the lower bound of the result of the ashr
      // operation, when Lower of the LHS is a non-negative number.
      // Since ashr of a non-negative number will result in a smaller
      // number, the Lower value of LHS is shifted right with the
      // maximum value of 'Other'.
      APInt PosMin = lthis.getSignedMin() >> ((size_t)Other.getUnsignedMax().convert_to<long>());

      // 'NegMax' is the upper bound of the result of the ashr
      // operation, when Upper of the LHS of ashr is a negative number.
      // Since 'ashr' of a negative number will result in a bigger
      // number, the Upper value of LHS is shifted right with the
      // maximum value of 'Other'.
      APInt NegMax = lthis.getSignedMax() >> ((size_t)Other.getUnsignedMax().convert_to<long>() + 1);

      // 'NegMin' is the lower bound of the result of the ashr
      // operation, when Lower of the LHS of ashr is a negative number.
      // Since 'ashr' of a negative number will result in a bigger
      // number, the Lower value of LHS is shifted right with the
      // minimum value of 'Other'.
      APInt NegMin = lthis.getSignedMin() >> ((size_t)Other.getUnsignedMin().convert_to<long>());

      APInt max, min;
      if(lthis.getSignedMin() >= 0)
      {
         // Upper and Lower of LHS are non-negative.
         min = PosMin;
         max = PosMax;
      }
      else if(lthis.getSignedMax() < 0)
      {
         // Upper and Lower of LHS are negative.
         min = NegMin;
         max = NegMax;
      }
      else
      {
         // Upper is non-negative and Lower is negative.
         min = NegMin;
         max = PosMax;
      }
      if(min == max)
      {
         return Range(Regular, lthis.getBitWidth());
      }

      return Range(Regular, lthis.getBitWidth(), std::move(min), std::move(max));
   }

   Range Range::ashr(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }

      auto this_min = getSignedMin();
      auto this_max = getSignedMax();
      UAPInt other_min(other.getUnsignedMin());
      UAPInt other_max(other.getUnsignedMax());
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      const Range otherR = (other_min == other_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), other_min, other_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      //const ConstantRange otherR = (other_min == other_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(other_min, other_max + 1);
      auto AshrU = local_ashr(thisR, otherR);

      if(AshrU.isFullSet())
      {
         return Range(Regular, getBitWidth());
      }
      return AshrU;
   }

   /*
    * 	This and operation is coded following Hacker's Delight algorithm.
    * 	According to the author, it provides tight results.
    */
   Range Range::And(const Range& other) const
   {
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("And-a: " << *this << std::endl << "And-b: " << other);
      #endif
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      APInt a = this->isAnti() ? Min : this->getLower();
      APInt b = this->isAnti() ? Max : this->getUpper();
      APInt c = other.isAnti() ? Min : other.getLower();
      APInt d = other.isAnti() ? Max : other.getUpper();

      // negate everybody
      APInt negA = ~APInt(a);
      APInt negB = ~APInt(b);
      APInt negC = ~APInt(c);
      APInt negD = ~APInt(d);
      auto bw = getBitWidth();

      Range inv1 = Range(Regular, bw, negB, negA);
      Range inv2 = Range(Regular, bw, negD, negC);
      Range invres = inv1.Or(inv2);

      // negate the result of the 'or'
      APInt invLower = ~invres.getUpper();
      APInt invUpper = ~invres.getLower();
      auto res = Range(Regular, bw, invLower, invUpper);
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("And-res: " << res << std::endl);
      #endif
      return res;
   }

   namespace
   {
      APInt minOR(APInt a, const APInt& b, APInt c, const APInt& d)
      {
         UAPInt ub(b), ud(d), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((~a & c & m) != 0)
            {
               temp = UAPInt((a | m) & -m);
               if(temp <= ub)
               {
                  a = temp;
                  break;
               }
            }
            else if((a & ~c & m) != 0)
            {
               temp = UAPInt((c | m) & -m);
               if(temp <= ud)
               {
                  c = temp;
                  break;
               }
            }
            m = lshr(m, 1);
         }
         return a | c;
      }

      APInt maxOR(const APInt& a, APInt b, const APInt& c, APInt d)
      {
         UAPInt ua(a), uc(c), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((b & d & m) != 0)
            {
               temp = UAPInt((b - m) | (m - 1));
               if(temp >= ua)
               {
                  b = temp;
                  break;
               }
               temp = UAPInt((d - m) | (m - 1));
               if(temp >= uc)
               {
                  d = temp;
                  break;
               }
            }
            m = lshr(m, 1);
         }
         return b | d;
      }

      APInt minXOR(APInt a, const APInt& b, APInt c, const APInt& d)
      {
         UAPInt ub(b), ud(d), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((~a & c & m) != 0)
            {
               temp = UAPInt((a | m) & -m);
               if(temp <= ub)
               {
                  a = temp;
               }
            }
            else if((a & ~c & m) != 0)
            {
               temp = UAPInt((c | m) & -m);
               if(temp <= ud)
               {
                  c = temp;
               }
            }
            m = lshr(m, 1);
         }
         return a ^ c;
      }

      APInt maxXOR(const APInt& a, APInt b, const APInt& c, APInt d)
      {
         UAPInt ua(a), uc(c), temp;
         APInt m;
         m = 1 << (MAX_BIT_INT - 1);
         while(m != 0)
         {
            if((b & d & m) != 0)
            {
               temp = UAPInt((b - m) | (m - 1));
               if(temp >= ua)
               {
                  b = temp;
               }
               else
               {
                  temp = UAPInt((d - m) | (m - 1));
                  if(temp >= uc)
                  {
                     d = temp;
                  }
               }
            }
            m = lshr(m, 1);
         }
         return b ^ d;
      }

   } // namespace

   /**
    * Or operation coded following Hacker's Delight algorithm.
    */
   Range Range::Or(const Range& other) const
   {
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("Or-a: " << *this << std::endl << "Or-b: " << other);
      #endif
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }

      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      const APInt c = other.getLower();
      const APInt d = other.getUpper();

      //      if (a.eq(Min) || b.eq(Max) || c.eq(Min) || d.eq(Max))
      //         return Range(Regular,getBitWidth());

      unsigned char switchval = 0;
      switchval += (a >= 0 ? 1 : 0);
      switchval <<= 1;
      switchval += (b >= 0 ? 1 : 0);
      switchval <<= 1;
      switchval += (c >= 0 ? 1 : 0);
      switchval <<= 1;
      switchval += (d >= 0 ? 1 : 0);

      APInt l = Min, u = Max;

      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("switchval " << (unsigned)switchval << std::endl);
      #endif

      switch(switchval)
      {
         case 0:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 1:
            l = a;
            // u = llvm::APInt::getAllOnesValue(MAX_BIT_INT);
            u = -1;  // TODO: check correctness against commented instruction above
            break;
         case 3:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 4:
            l = c;
            // u = llvm::APInt::getAllOnesValue(MAX_BIT_INT);
            u = -1;
            break;
         case 5:
            l = (a < c ? a : c);
            u = maxOR(0, b, 0, d);
            break;
         case 7:
            // l = minOR(a, APInt::getAllOnesValue(MAX_BIT_INT), c, d);
            l = minOR(a, -1, c, d);
            u = maxOR(0, b, c, d);
            break;
         case 12:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
         case 13:
            // l = minOR(a, b, c, APInt::getAllOnesValue(MAX_BIT_INT));
            l = minOR(a, b, c, -1);
            u = maxOR(a, b, 0, d);
            break;
         case 15:
            l = minOR(a, b, c, d);
            u = maxOR(a, b, c, d);
            break;
      }
      if((l == Min) || (u == Max))
      {
         return Range(Regular, getBitWidth());
      }
      auto res = Range(Regular, getBitWidth(), l, u);
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("Or-res: " << res << std::endl);
      #endif
      return res;
   }

   Range Range::Xor(const Range& other) const
   {
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("Xor-a: " << *this << std::endl << "Xor-b: " << other);
      #endif
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(this->isAnti() || other.isAnti())
      {
         return Range(Regular, getBitWidth());
      }
      const APInt a = this->getLower();
      const APInt b = this->getUpper();
      const APInt c = other.getLower();
      const APInt d = other.getUpper();
      if((a >= 0) && (b >= 0) && (c >= 0) && (d >= 0))
      {
         APInt l = minXOR(a, b, c, d);
         APInt u = maxXOR(a, b, c, d);
         auto res = Range(Regular, getBitWidth(), l, u);
         #ifdef DEBUG_RANGE_OP
         PRINT_MSG("Xor-res: " << res << std::endl);
         #endif
         return res;
      }
      else if((c == -1) && (d == -1) && (a >= 0) && (b >= 0))
      {
         auto res = other.sub(*this);
         #ifdef DEBUG_RANGE_OP
         PRINT_MSG("Xor-res: " << res << std::endl);
         #endif
         return res;
      }
      return Range(Regular, getBitWidth());
   }

   Range Range::Eq(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(!isAnti() && !other.isAnti())
      {
         if((getLower() == Min) || (getUpper() == Max) || (other.getLower() == Min) || (other.getUpper() == Max))
         {
            return Range(Regular, bw, 0, 1);
         }
      }
      bool areTheyEqual = !this->intersectWith(other).isEmpty();
      auto AntiThis = getAnti(*this);
      APInt a = isAnti() ? AntiThis.getLower() : this->getLower();
      APInt b = isAnti() ? AntiThis.getUpper() : this->getUpper();
      bool areTheyDifferent = !((a == b) && *this == other);

      if(areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, 0, 1);
      }
      if(areTheyEqual && !areTheyDifferent)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(!areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, 0, 0);
      }

      THROW_UNREACHABLE("condition unexpected");
   }
   Range Range::Ne(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(!isAnti() && !other.isAnti())
      {
         if((getLower() == Min) || (getUpper() == Max) || (other.getLower() == Min) || (other.getUpper() == Max))
         {
            return Range(Regular, bw, 0, 1);
         }
      }
      bool areTheyEqual = !this->intersectWith(other).isEmpty();
      auto AntiThis = getAnti(*this);
      APInt a = isAnti() ? AntiThis.getLower() : this->getLower();
      APInt b = isAnti() ? AntiThis.getUpper() : this->getUpper();
      bool areTheyDifferent = !((a == b) && *this == other);
      if(areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, 0, 1);
      }
      if(areTheyEqual && !areTheyDifferent)
      {
         return Range(Regular, bw, 0, 0);
      }
      if(!areTheyEqual && areTheyDifferent)
      {
         return Range(Regular, bw, 1, 1);
      }

      THROW_UNREACHABLE("condition unexpected");
   }
   Range Range::Ugt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(a > d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c >= b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Uge(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(a >= d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c > b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Ult(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(b < c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d <= a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Ule(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      UAPInt a(this->getUnsignedMin());
      UAPInt b(this->getUnsignedMax());
      UAPInt c(other.getUnsignedMin());
      UAPInt d(other.getUnsignedMax());
      if(b <= c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d < a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Sgt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(a > d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c >= b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Sge(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(a >= d)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(c > b)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Slt(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(b < c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d <= a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }
   Range Range::Sle(const Range& other, unsigned bw) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      if(isAnti() || other.isAnti())
      {
         return Range(Regular, bw, 0, 1);
      }

      APInt a = this->getSignedMin();
      APInt b = this->getSignedMax();
      APInt c = other.getSignedMin();
      APInt d = other.getSignedMax();
      if(b <= c)
      {
         return Range(Regular, bw, 1, 1);
      }
      if(d < a)
      {
         return Range(Regular, bw, 0, 0);
      }

      return Range(Regular, bw, 0, 1);
   }

   // Truncate
   // - if the source range is entirely inside max bit range, it is the result
   // - else, the result is the max bit range
   Range Range::truncate(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }

      APInt maxupper = getSignedMaxValue(bitwidth);
      APInt maxlower = getSignedMinValue(bitwidth);

      // Check if source range is contained by max bit range
      if(!this->isAnti() && this->getLower() >= maxlower && this->getUpper() <= maxupper)
      {
         return Range(Regular, bitwidth, this->getLower(), this->getUpper());
      }

      return Range(Regular, bitwidth, maxlower, maxupper);
   }

   Range Range::sextOrTrunc(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      auto from_bw = getBitWidth();
      auto this_min = from_bw == 1 ? getUnsignedMin() : getSignedMin();
      auto this_max = from_bw == 1 ? getUnsignedMax() : getSignedMax();
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      auto sextRes = Range(Regular, bitwidth, ap_trunc(thisR.getLower(), bitwidth), ap_trunc(thisR.getUpper(), bitwidth));
      if(sextRes.isFullSet())
      {
         return Range(Regular, bitwidth);
      }

      return Range(Regular, bitwidth, sextRes.getSignedMin(), sextRes.getSignedMax());
   }

   Range Range::zextOrTrunc(unsigned bitwidth) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      auto this_min = getUnsignedMin();
      auto this_max = getUnsignedMax();
      const Range thisR = (this_min == this_max + 1) ? Range(Regular, getBitWidth()) : Range(Regular, getBitWidth(), this_min, this_max + 1);
      //const ConstantRange thisR = (this_min == this_max + 1) ? ConstantRange(getBitWidth(), /*isFullSet=*/true) : ConstantRange(this_min, this_max + 1);
      auto zextRes = Range(Regular, bitwidth, ap_trunc(thisR.getLower(), bitwidth), ap_trunc(thisR.getUpper(), bitwidth));
      if(zextRes.isFullSet())
      {
         return Range(Regular, bitwidth);
      }

      return Range(Regular, bitwidth, zextRes.getUnsignedMin(), zextRes.getUnsignedMax());
   }

   Range Range::intersectWith(const Range& other) const
   {
      if(isEmpty() || isUnknown())
      {
         return *this;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return other;
      }
      #ifdef DEBUG_RANGE_OP
      PRINT_MSG("intersectWith-a: " << *this << std::endl << "intersectWith-b: " << other);
      #endif

      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower() > other.getLower() ? getLower() : other.getLower();
         APInt u = getUpper() < other.getUpper() ? getUpper() : other.getUpper();
         if(u < l)
         {
            return Range(Empty, getBitWidth());
         }

         return Range(Regular, getBitWidth(), l, u);
      }
      if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil <= other.getLower())
         {
            if(other.getUpper() <= antiu)
            {
               return Range(Empty, getBitWidth());
            }
            APInt l = other.getLower() > antiu ? other.getLower() : antiu + 1;
            APInt u = other.getUpper();
            return Range(Regular, getBitWidth(), l, u);
         }
         if(antiu >= other.getUpper())
         {
            THROW_ASSERT(!other.getLower() >= antil, "");
            APInt l = other.getLower();
            APInt u = other.getUpper() < antil ? other.getUpper() : antil - 1;
            return Range(Regular, getBitWidth(), l, u);
         }
         if(other.getLower() == Min && other.getUpper() == Max)
         {
            return *this;
         }
         if(antil > other.getUpper() || antiu < other.getLower())
         {
            return other;
         }

         // we approximate to the range of other
         return other;
      }
      if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         if(antil <= this->getLower())
         {
            if(this->getUpper() <= antiu)
            {
               return Range(Empty, getBitWidth());
            }
            APInt l = this->getLower() > antiu ? this->getLower() : antiu + 1;
            APInt u = this->getUpper();
            return Range(Regular, getBitWidth(), l, u);
         }
         if(antiu >= this->getUpper())
         {
            THROW_ASSERT(!this->getLower() >= antil, "");
            APInt l = this->getLower();
            APInt u = this->getUpper() < antil ? this->getUpper() : antil - 1;
            return Range(Regular, getBitWidth(), l, u);
         }
         if(this->getLower() == Min && this->getUpper() == Max)
         {
            return other;
         }
         if(antil > this->getUpper() || antiu < this->getLower())
         {
            return *this;
         }

         // we approximate to the range of this
         return *this;
      }

      auto antiRange_a = getAnti(*this);
      auto antiRange_b = getAnti(other);
      auto antil_a = antiRange_a.getLower();
      auto antiu_a = antiRange_a.getUpper();
      auto antil_b = antiRange_b.getLower();
      auto antiu_b = antiRange_b.getUpper();
      if(antil_a > antil_b)
      {
         std::swap(antil_a, antil_b);
         std::swap(antiu_a, antiu_b);
      }
      if(antil_b > (antiu_a + 1))
      {
         return Range(Anti, getBitWidth(), antil_a, antiu_a);
      }
      APInt l = antil_a;
      APInt u = antiu_a > antiu_b ? antiu_a : antiu_b;
      if(l == Min && u == Max)
      {
         return Range(Empty, getBitWidth());
      }

      return Range(Anti, getBitWidth(), l, u);
   }

   Range Range::unionWith(const Range& other) const
   {
      if(this->isEmpty() || this->isUnknown())
      {
         return other;
      }
      if(other.isEmpty() || other.isUnknown())
      {
         return *this;
      }
      if(!this->isAnti() && !other.isAnti())
      {
         APInt l = getLower() < other.getLower() ? getLower() : other.getLower();
         APInt u = getUpper() > other.getUpper() ? getUpper() : other.getUpper();
         return Range(Regular, getBitWidth(), l, u);
      }
      if(this->isAnti() && !other.isAnti())
      {
         auto antiRange = getAnti(*this);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         THROW_ASSERT(antil != Min, "");
         THROW_ASSERT(antiu != Max, "");
         if(antil > other.getUpper() || antiu < other.getLower())
         {
            return *this;
         }
         if(antil > other.getLower() && antiu < other.getUpper())
         {
            return Range(Regular, getBitWidth());
         }
         if(antil >= other.getLower() && antiu > other.getUpper())
         {
            return Range(Anti, getBitWidth(), other.getUpper() + 1, antiu);
         }
         if(antil < other.getLower() && antiu <= other.getUpper())
         {
            return Range(Anti, getBitWidth(), antil, other.getLower() - 1);
         }

         return Range(Regular, getBitWidth()); // approximate to the full set
      }
      if(!this->isAnti() && other.isAnti())
      {
         auto antiRange = getAnti(other);
         auto antil = antiRange.getLower();
         auto antiu = antiRange.getUpper();
         THROW_ASSERT(antil != Min, "");
         THROW_ASSERT(antiu != Max, "");
         if(antil > this->getUpper() || antiu < this->getLower())
         {
            return other;
         }
         if(antil > this->getLower() && antiu < this->getUpper())
         {
            return Range(Regular, getBitWidth());
         }
         if(antil >= this->getLower() && antiu > this->getUpper())
         {
            return Range(Anti, getBitWidth(), this->getUpper() + 1, antiu);
         }
         if(antil < this->getLower() && antiu <= this->getUpper())
         {
            return Range(Anti, getBitWidth(), antil, this->getLower() - 1);
         }

         return Range(Regular, getBitWidth()); // approximate to the full set
      }

      auto antiRange_a = getAnti(*this);
      auto antiRange_b = getAnti(other);
      auto antil_a = antiRange_a.getLower();
      auto antiu_a = antiRange_a.getUpper();
      THROW_ASSERT(antil_a != Min, "");
      THROW_ASSERT(antiu_a != Max, "");
      auto antil_b = antiRange_b.getLower();
      auto antiu_b = antiRange_b.getUpper();
      THROW_ASSERT(antil_b != Min, "");
      THROW_ASSERT(antiu_b != Max, "");
      if(antil_a > antiu_b || antiu_a < antil_b)
      {
         return Range(Regular, getBitWidth());
      }
      if(antil_a > antil_b && antiu_a < antiu_b)
      {
         return *this;
      }
      if(antil_b > antil_a && antiu_b < antiu_a)
      {
         return *this;
      }
      if(antil_a >= antil_b && antiu_b <= antiu_a)
      {
         return Range(Anti, getBitWidth(), antil_a, antiu_b);
      }
      if(antil_b >= antil_a && antiu_a <= antiu_b)
      {
         return Range(Anti, getBitWidth(), antil_b, antiu_a);
      }

      THROW_UNREACHABLE("unsupported condition");
   }

   Range Range::BestRange(const Range& UR, const Range& SR, unsigned bw) const
   {
      if(UR.isFullSet() && SR.isFullSet())
      {
         return Range(Regular, bw);
      }
      if(UR.isFullSet())
      {
         return SR.truncate(bw);
      }
      if(SR.isFullSet())
      {
         return UR.truncate(bw);
      }
      auto nbitU = neededBits(UR.getUnsignedMin(), UR.getUnsignedMax(), false);
      auto nbitS = neededBits(SR.getSignedMin(), SR.getSignedMax(), true);
      if(nbitU < nbitS)
      {
         return UR.truncate(bw);
      }

      return SR.truncate(bw);
   }

   bool Range::operator==(const Range& other) const
   {
      return getBitWidth() == other.getBitWidth() && Range::isSameType(*this, other) && Range::isSameRange(*this, other);
   }

   bool Range::operator!=(const Range& other) const
   {
      return getBitWidth() != other.getBitWidth() || !Range::isSameType(*this, other) || !Range::isSameRange(*this, other);
   }

   void Range::print(std::ostream& OS) const
   {
      if(this->isUnknown())
      {
         OS << "Unknown";
         return;
      }
      if(this->isEmpty())
      {
         OS << "Empty";
         return;
      }
      if(this->isAnti())
      {
         auto antiObj = getAnti(*this);
         if(antiObj.getLower() == Min)
         {
            OS << ")-inf,";
         }
         else
         {
            OS << ")" << antiObj.getLower().str() << ",";
         }
         OS << getBitWidth() << ",";
         if(antiObj.getUpper() == Max)
         {
            OS << "+inf(";
         }
         else
         {
            OS << antiObj.getUpper().str() << "(";
         }
      }
      else
      {
         if(getLower() == Min)
         {
            OS << "[-inf,";
         }
         else
         {
            OS << "[" << getLower().str() << ",";
         }
         OS << getBitWidth() << ",";
         if(getUpper() == Max)
         {
            OS << "+inf]";
         }
         else
         {
            OS << getUpper().str() << "]";
         }
      }
   }

   std::ostream& operator<<(std::ostream& OS, const Range& R)
   {
      R.print(OS);
      return OS;
   }

   Range Range::makeSatisfyingCmpRegion(kind pred, const Range& Other)
   {
      unsigned bw = Other.getBitWidth();

      switch (pred)
      {
      case ge_expr_K:
         return Range(Regular, bw, Other.getUpper() - 1, getSignedMaxValue(bw));
      case gt_expr_K:
         return Range(Regular, bw, Other.getUpper(), getSignedMaxValue(bw));
      case le_expr_K:
         return Range(Regular, bw, getSignedMinValue(bw), Other.getLower() + 1);
      case lt_expr_K:
         return Range(Regular, bw, getSignedMinValue(bw), Other.getLower());
      case unge_expr_K:
         return Range(Regular, bw, UAPInt(Other.getUpper() - 1), getMaxValue(bw));
      case ungt_expr_K:
         return Range(Regular, bw, UAPInt(Other.getUpper()), getMaxValue(bw));
      case unle_expr_K:
         return Range(Regular, bw, getMinValue(bw), UAPInt(Other.getLower()));
      case unlt_expr_K:
         return Range(Regular, bw, getMinValue(bw), UAPInt(Other.getLower() - 1));
      case eq_expr_K:
         return Other;
      case ne_expr_K:
         return getAnti(Other);
      
      case uneq_expr_K:
      default:
         break;
      }

      THROW_UNREACHABLE("Unhandled compare operation (" + boost::lexical_cast<std::string>(pred) + ")");
   }

   // ========================================================================== //
   // VarNode
   // ========================================================================== //
   class VarNode
   {
    private:
      /// The program variable
      const tree_nodeRef V;
      /// if not null refers to a global variable object or to an AllocaInst
      const tree_nodeRef GV;
      /// A Range associated to the variable, that is,
      /// its interval inferred by the analysis.
      Range interval;
      /// Used by the crop meet operator
      char abstractState;

    public:
      explicit VarNode(const tree_nodeRef _V, const tree_nodeRef _GV);
      ~VarNode();
      VarNode(const VarNode&) = delete;
      VarNode(VarNode&&) = delete;
      VarNode& operator=(const VarNode&) = delete;
      VarNode& operator=(VarNode&&) = delete;

      /// Initializes the value of the node.
      void init(bool outside);
      /// Returns the range of the variable represented by this node.
      const Range getRange() const
      {
         return interval;
      }
      /// Returns the variable represented by this node.
      eValue getValue() const
      {
         return std::make_pair(V, GV);
      }
      unsigned int getBitWidth() const
      {
         return interval.getBitWidth();
      }
      /// Changes the status of the variable represented by this node.
      void setRange(const Range newInterval)
      {
         interval = newInterval;
      }

      /// Pretty print.
      void print(std::ostream& OS) const;
      char getAbstractState()
      {
         return abstractState;
      }
      // The possible states are '0', '+', '-' and '?'.
      void storeAbstractState();
   };

   /// The ctor.
   VarNode::VarNode(const tree_nodeRef _V, const tree_nodeRef _GV) : V(_V), GV(_GV), interval(Unknown, LoadStoreOperationBW(_V, _GV), Min, Max), abstractState(0)
   {
      if(_V->get_kind() == cast_expr_K)
      {
         THROW_UNREACHABLE("Cast expression not yet implemented");
      }

      // TODO: do this operations exists in GIMPLE representation?
      //if(dyn_cast<FPToSIInst>(_V))
      //{
      //   auto bw = _V->getType()->getPrimitiveSizeInBits();
      //   interval = Range(Regular, bw, llvm::APInt::getSignedMinValue(bw).sext(MAX_BIT_INT), llvm::APInt::getSignedMaxValue(bw).sext(MAX_BIT_INT));
      //}
      //else if(dyn_cast<FPToUIInst>(_V))
      //{
      //   auto bw = _V->getType()->getPrimitiveSizeInBits();
      //   interval = Range(Regular, bw, llvm::APInt::getMinValue(bw).sext(MAX_BIT_INT), llvm::APInt::getMaxValue(bw).sext(MAX_BIT_INT));
      //}
   }

   /// The dtor.
   VarNode::~VarNode() = default;

   /// Initializes the value of the node.
   void VarNode::init(bool outside)
   {
      auto bw = LoadStoreOperationBW(V, GV);
      THROW_ASSERT(bw, "Bitwidth not valid");
      if(const auto* CI = GetPointer<integer_cst>(GET_NODE(V)))
      {
         APInt tmp = CI->value;
         this->setRange(Range(Regular, bw, tmp, tmp));
      }
      else
      {
         if(!outside)
         {
            // Initialize with a basic, unknown, interval.
            this->setRange(Range(Unknown, bw));
         }
         else
         {
            this->setRange(Range(Regular, bw));
         }
      }
   }

   /// Pretty print.
   void VarNode::print(std::ostream& OS) const
   {
      if(const auto* C = GetPointer<integer_cst>(GET_NODE(V)))
      {
         OS << C->value;
      }
      else if(GV)
      {
         printVarName(GV, OS);
         OS << "=";
         printVarName(V, OS);
      }
      else
      {
         printVarName(V, OS);
      }
      OS << " ";
      this->getRange().print(OS);
   }

   void VarNode::storeAbstractState()
   {
      THROW_ASSERT(!this->interval.isUnknown(), "storeAbstractState doesn't handle empty set");

      if(this->interval.getLower() == Min)
      {
         if(this->interval.getUpper() == Max)
         {
            this->abstractState = '?';
         }
         else
         {
            this->abstractState = '-';
         }
      }
      else if(this->interval.getUpper() == Max)
      {
         this->abstractState = '+';
      }
      else
      {
         this->abstractState = '0';
      }
   }

   std::ostream& operator<<(std::ostream& OS, const VarNode* VN)
   {
      VN->print(OS);
      return OS;
   }

   // ========================================================================== //
   // BasicInterval
   // ========================================================================== //
   enum IntervalId
   {
      BasicIntervalId,
      SymbIntervalId
   };

   /// This class represents a basic interval of values. This class could inherit
   /// from LLVM's Range class, since it *is a Range*. However,
   /// LLVM's Range class doesn't have a virtual constructor.
   class BasicInterval
   {
    private:
      Range range;

    public:
      BasicInterval();
      explicit BasicInterval(Range range);
      virtual ~BasicInterval(); // This is a base class.
      BasicInterval(const BasicInterval&) = delete;
      BasicInterval(BasicInterval&&) = delete;
      BasicInterval& operator=(const BasicInterval&) = delete;
      BasicInterval& operator=(BasicInterval&&) = delete;

      // Methods for RTTI
      virtual IntervalId getValueId() const
      {
         return BasicIntervalId;
      }
      static bool classof(BasicInterval const* /*unused*/)
      {
         return true;
      }

      /// Returns the range of this interval.
      const Range& getRange() const
      {
         return this->range;
      }
      /// Sets the range of this interval to another range.
      void setRange(const Range& newRange)
      {
         this->range = newRange;
      }

      /// Pretty print.
      virtual void print(std::ostream& OS) const;
   };

   BasicInterval::BasicInterval(Range range) : range(std::move(range))
   {
   }

   BasicInterval::BasicInterval() : range(Range(Regular, MAX_BIT_INT))
   {
   }

   // This is a base class, its dtor must be virtual.
   BasicInterval::~BasicInterval() = default;

   /// Pretty print.
   void BasicInterval::print(std::ostream& OS) const
   {
      this->getRange().print(OS);
   }

   std::ostream& operator<<(std::ostream& OS, const BasicInterval* BI)
   {
      BI->print(OS);
      return OS;
   }

   // ========================================================================== //
   // SymbInterval
   // ========================================================================== //

   /// This is an interval that contains a symbolic limit, which is
   /// given by the bounds of a program name, e.g.: [-inf, ub(b) + 1].
   class SymbInterval : public BasicInterval
   {
    private:
      /// The bound. It is a node which limits the interval of this range.
      const tree_nodeRef bound;
      /// The predicate of the operation in which this interval takes part.
      /// It is useful to know how we can constrain this interval
      /// after we fix the intersects.
      kind pred;

    public:
      SymbInterval(const Range& range, const tree_nodeRef bound, kind pred);
      ~SymbInterval() override;
      SymbInterval(const SymbInterval&) = delete;
      SymbInterval(SymbInterval&&) = delete;
      SymbInterval& operator=(const SymbInterval&) = delete;
      SymbInterval& operator=(SymbInterval&&) = delete;

      // Methods for RTTI
      IntervalId getValueId() const override
      {
         return SymbIntervalId;
      }
      static bool classof(SymbInterval const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicInterval const* BI)
      {
         return BI->getValueId() == SymbIntervalId;
      }

      /// Returns the opcode of the operation that create this interval.
      kind getOperation() const
      {
         return this->pred;
      }
      /// Returns the node which is the bound of this interval.
      const eValue getBound() const
      {
         return std::make_pair(this->bound, nullptr);
      }
      /// Replace symbolic intervals with hard-wired constants.
      Range fixIntersects(VarNode* bound, VarNode* sink);

      /// Prints the content of the interval.
      void print(std::ostream& OS) const override;
   };

   SymbInterval::SymbInterval(const Range& range, const tree_nodeRef bound, kind pred) : BasicInterval(range), bound(bound), pred(pred)
   {
   }

   SymbInterval::~SymbInterval() = default;

   Range SymbInterval::fixIntersects(VarNode* bound, VarNode* sink)
   {
      // Get the lower and the upper bound of the
      // node which bounds this intersection.
      const auto boundRange = bound->getRange();
      const auto sinkRange = sink->getRange();
      THROW_ASSERT(!boundRange.isEmpty(), "");
      THROW_ASSERT(!sinkRange.isEmpty(), "");

      auto IsAnti = bound->getRange().isAnti() || sinkRange.isAnti();
      APInt l = IsAnti ? (boundRange.isUnknown() ? Min : boundRange.getUnsignedMin()) : boundRange.getLower();
      APInt u = IsAnti ? (boundRange.isUnknown() ? Max : boundRange.getUnsignedMax()) : boundRange.getUpper();

      // Get the lower and upper bound of the interval of this operation
      APInt lower = IsAnti ? (sinkRange.isUnknown() ? Min : sinkRange.getUnsignedMin()) : sinkRange.getLower();
      APInt upper = IsAnti ? (sinkRange.isUnknown() ? Max : sinkRange.getUnsignedMax()) : sinkRange.getUpper();

      auto bw = getRange().getBitWidth();
      switch(this->getOperation())
      {
         case eq_expr_K: // equal
            return Range(Regular, bw, l, u);
         case le_expr_K: // signed less or equal
            if(lower > u)
            {
               return Range(Empty, bw);
            }
            else
            {
               return Range(Regular, bw, lower, u);
            }
         case lt_expr_K: // signed less than
            if(u != Max)
            {
               if(lower > (u - 1))
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, lower, u - 1);
            }
            else
            {
               if(lower > u)
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, lower, u);
            }
         case ge_expr_K: // signed greater or equal
            if(l > upper)
            {
               return Range(Empty, bw);
            }
            else
            {
               return Range(Regular, bw, l, upper);
            }
         case gt_expr_K: // signed greater than
            if(l != Min)
            {
               if((l + 1) > upper)
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, l + 1, upper);
            }
            else
            {
               if(l > upper)
               {
                  return Range(Empty, bw);
               }

               return Range(Regular, bw, l, upper);
            }
         default:
            return Range(Regular, bw);
      }
      THROW_UNREACHABLE("unexpected condition");
   }

   /// Pretty print.
   void SymbInterval::print(std::ostream& OS) const
   {
      auto bnd = getBound();
      switch(this->getOperation())
      {
         case eq_expr_K: // equal
            OS << "[lb(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << "), ub(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << ")]";
            break;
         case le_expr_K: // sign less or equal
            OS << "[-inf, ub(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << ")]";
            break;
         case lt_expr_K: // sign less than
            OS << "[-inf, ub(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << ") - 1]";
            break;
         case ge_expr_K: // sign greater or equal
            OS << "[lb(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << "), +inf]";
            break;
         case gt_expr_K: // sign greater than
            OS << "[lb(";
            if(bnd.second)
            {
               printVarName(bnd.second, OS);
            }
            else
            {
               printVarName(bnd.first, OS);
            }
            OS << " - 1), +inf]";
            break;
         default:
            OS << "Unknown Instruction.\n";
      }
   }

   // ========================================================================== //
   // BasicOp
   // ========================================================================== //

   /// This class represents a generic operation in our analysis.
   class BasicOp
   {
    private:
      /// The range of the operation. Each operation has a range associated to it.
      /// This range is obtained by inspecting the branches in the source program
      /// and extracting its condition and intervals.
      std::shared_ptr<BasicInterval> intersect;
      // The target of the operation, that is, the node which
      // will store the result of the operation.
      VarNode* sink;
      // The instruction that originated this op node
      const tree_nodeRef inst;

    protected:
      /// We do not want people creating objects of this class,
      /// but we want to inherit from it.
      BasicOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const tree_nodeRef inst);

    public:
      enum class OperationId
      {
         UnaryOpId,
         SigmaOpId,
         BinaryOpId,
         TernaryOpId,
         PhiOpId,
         ControlDepId,
         LoadOpId,
         StoreOpId
      };

      /// The dtor. It's virtual because this is a base class.
      virtual ~BasicOp();
      // We do not want people creating objects of this class.
      BasicOp(const BasicOp&) = delete;
      BasicOp(BasicOp&&) = delete;
      BasicOp& operator=(const BasicOp&) = delete;
      BasicOp& operator=(BasicOp&&) = delete;

      // Methods for RTTI
      virtual OperationId getValueId() const = 0;
      static bool classof(BasicOp const* /*unused*/)
      {
         return true;
      }

      /// Given the input of the operation and the operation that will be
      /// performed, evaluates the result of the operation.
      virtual Range eval() = 0;
      /// Return the instruction that originated this op node
      const tree_nodeRef getInstruction() const
      {
         return inst;
      }
      /// Replace symbolic intervals with hard-wired constants.
      void fixIntersects(VarNode* V);
      /// Returns the range of the operation.
      std::shared_ptr<BasicInterval> getIntersect() const
      {
         return intersect;
      }
      /// Changes the interval of the operation.
      void setIntersect(const Range& newIntersect)
      {
         this->intersect->setRange(newIntersect);
      }
      /// Returns the target of the operation, that is,
      /// where the result will be stored.
      const VarNode* getSink() const
      {
         return sink;
      }
      /// Returns the target of the operation, that is,
      /// where the result will be stored.
      VarNode* getSink()
      {
         return sink;
      }

      /// Prints the content of the operation.
      virtual void print(std::ostream& OS) const = 0;
   };

   /// We can not want people creating objects of this class,
   /// but we want to inherit of it.
   BasicOp::BasicOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const tree_nodeRef inst) : intersect(std::move(intersect)), sink(sink), inst(inst)
   {
   }

   /// We can not want people creating objects of this class,
   /// but we want to inherit of it.
   BasicOp::~BasicOp() = default;

   /// Replace symbolic intervals with hard-wired constants.
   void BasicOp::fixIntersects(VarNode* V)
   {
      if(auto* SI = reinterpret_cast<SymbInterval*>(getIntersect().get()))
      {
         this->setIntersect(SI->fixIntersects(V, getSink()));
      }
   }

   // ========================================================================== //
   // PhiOp
   // ========================================================================== //

   /// A constraint like sink = phi(src1, src2, ..., srcN)
   class PhiOp : public BasicOp
   {
    private:
      // Vector of sources
      std::vector<const VarNode*> sources;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      PhiOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const tree_nodeRef inst);
      ~PhiOp() override = default;
      PhiOp(const PhiOp&) = delete;
      PhiOp(PhiOp&&) = delete;
      PhiOp& operator=(const PhiOp&) = delete;
      PhiOp& operator=(PhiOp&&) = delete;

      /// Add source to the vector of sources
      void addSource(const VarNode* newsrc)
      {
         sources.push_back(newsrc);
      }
      /// Return source identified by index
      const VarNode* getSource(unsigned index) const
      {
         return sources[index];
      }
      /// return the number of sources
      size_t getNumSources() const
      {
         return sources.size();
      }

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::PhiOpId;
      }
      static bool classof(PhiOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::PhiOpId;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(std::ostream& OS) const override;
   };

   // The ctor.
   PhiOp::PhiOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, const tree_nodeRef inst) : BasicOp(std::move(intersect), sink, inst)
   {
   }

   /// Computes the interval of the sink based on the interval of the sources.
   /// The result of evaluating a phi-function is the union of the ranges of
   /// every variable used in the phi.
   Range PhiOp::eval()
   {
      THROW_ASSERT(sources.size() > 0, "");
      Range result = this->getSource(0)->getRange();
      #ifdef DEBUG_BASICOP_EVAL
      PRINT_MSG(getSink());
      #endif
      // Iterate over the sources of the phiop
      for(const VarNode* varNode : sources)
      {
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG(" ->" << varNode->getRange());
         #endif
         result = result.unionWith(varNode->getRange());
      }
      #ifdef DEBUG_BASICOP_EVAL
      PRINT_MSG("=" << result);
      #endif
      bool test = this->getIntersect()->getRange().isMaxRange();
      if(!test)
      {
         Range aux = this->getIntersect()->getRange();
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG(" aux=" << aux);
         #endif
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
      }
      #ifdef DEBUG_BASICOP_EVAL
      PRINT_MSG(" res=" << result);
      #endif
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void PhiOp::print(std::ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";
      OS << "phi";
      OS << "\"]\n";
      for(const VarNode* varNode : sources)
      {
         const auto V = varNode->getValue();
         if(auto* C = GetPointer<integer_cst>(V.first))
         {
            OS << " " << C->value << " -> " << quot << this << quot << "\n";
         }
         else
         {
            OS << " " << quot;
            if(V.second)
            {
               printVarName(V.second, OS);
               OS << "." << V.first;
            }
            else
            {
               printVarName(V.first, OS);
            }
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }
      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // UnaryOp
   // ========================================================================== //
   /// A constraint like sink = operation(source) \intersec [l, u]
   /// Examples: unary instructions such as truncation, sign extensions,
   /// zero extensions.
   class UnaryOp : public BasicOp
   {
    private:
      // The source node of the operation.
      VarNode* source;
      // The opcode of the operation.
      kind opcode;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      UnaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source, kind opcode);
      ~UnaryOp() override;
      UnaryOp(const UnaryOp&) = delete;
      UnaryOp(UnaryOp&&) = delete;
      UnaryOp& operator=(const UnaryOp&) = delete;
      UnaryOp& operator=(UnaryOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::UnaryOpId;
      }
      static bool classof(UnaryOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::UnaryOpId || BO->getValueId() == OperationId::SigmaOpId;
      }

      /// Return the opcode of the operation.
      kind getOpcode() const
      {
         return opcode;
      }
      /// Returns the source of the operation.
      VarNode* getSource() const
      {
         return source;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(std::ostream& OS) const override;
   };

   UnaryOp::UnaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source, kind opcode) : BasicOp(std::move(intersect), sink, inst), source(source), opcode(opcode)
   {
   }

   // The dtor.
   UnaryOp::~UnaryOp() = default;

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range UnaryOp::eval()
   {
      unsigned bw = getSink()->getBitWidth();
      Range oprnd = source->getRange();
      Range result(Unknown, bw, Min, Max);

      if(oprnd.isRegular() || oprnd.isAnti())
      {
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG(getSink()->getValue().first->ToString() << std::endl << oprnd);
         #endif
         // TODO: update with useful operations
         switch(this->getOpcode())
         {
            //case Instruction::Trunc:
            //   result = oprnd.truncate(bw);
            //   break;
            //case Instruction::ZExt:
            //   result = oprnd.zextOrTrunc(bw);
            //   break;
            //case Instruction::SExt:
            //   result = oprnd.sextOrTrunc(bw);
            //   break;
            //case Instruction::FPToSI:
            //   result = oprnd.sextOrTrunc(bw);
            //   break;
            //case Instruction::FPToUI:
            //   result = oprnd.zextOrTrunc(bw);
            //   break;
            default:
               // Loads and Stores are handled here.
               result = oprnd;
               break;
         }
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG("=" << result);
         #endif
      }
      else if(oprnd.isEmpty())
      {
         result = Range(Empty, bw);
      }

      auto test = this->getIntersect()->getRange().isMaxRange();
      if(!test)
      {
         Range aux = this->getIntersect()->getRange();
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG("intersection: " << result);
         #endif
      }
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void UnaryOp::print(std::ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")";

      // Instruction bitwidth
      unsigned bw = getSink()->getBitWidth();

      // TODO: update with valid operations
      switch(this->opcode)
      {
         //case Instruction::Trunc:
         //   OS << "trunc i" << bw;
         //   break;
         //case Instruction::ZExt:
         //   OS << "zext i" << bw;
         //   break;
         //case Instruction::SExt:
         //   OS << "sext i" << bw;
         //   break;
         //case Instruction::FPToSI:
         //   OS << "fptosi i" << bw;
         //   break;
         //case Instruction::FPToUI:
         //   OS << "fptoui i" << bw;
         //   break;
         default:
            // Phi functions, Loads and Stores are handled here.
            this->getIntersect()->print(OS);
            break;
      }

      OS << "\"]\n";

      const auto V = this->getSource()->getValue();
      if(const auto* C = GetPointer<integer_cst>(V.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V.second)
         {
            printVarName(V.second, OS);
            OS << "." << V.first;
         }
         else
         {
            printVarName(V.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // SigmaOp
   // ========================================================================== //
   /// Specific type of UnaryOp used to represent sigma functions
   class SigmaOp : public UnaryOp
   {
    private:
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

      // The symbolic source node of the operation.
      VarNode* SymbolicSource;

      bool unresolved;

    public:
      SigmaOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source, VarNode* SymbolicSource, kind opcode);
      ~SigmaOp() override = default;
      SigmaOp(const SigmaOp&) = delete;
      SigmaOp(SigmaOp&&) = delete;
      SigmaOp& operator=(const SigmaOp&) = delete;
      SigmaOp& operator=(SigmaOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::SigmaOpId;
      }
      static bool classof(SigmaOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(UnaryOp const* UO)
      {
         return UO->getValueId() == OperationId::SigmaOpId;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::SigmaOpId;
      }

      bool isUnresolved() const
      {
         return unresolved;
      }
      void markResolved()
      {
         unresolved = false;
      }
      void markUnresolved()
      {
         unresolved = true;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(std::ostream& OS) const override;
   };

   SigmaOp::SigmaOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source, VarNode* _SymbolicSource, kind opcode)
       : UnaryOp(std::move(intersect), sink, inst, source, opcode), SymbolicSource(_SymbolicSource), unresolved(false)
   {
   }

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   Range SigmaOp::eval()
   {
      Range result = this->getSource()->getRange();
      #ifdef DEBUG_BASICOP_EVAL
      PRINT_MSG("SigmaOp: " << getSink() << " src=" << result);
      #endif
      Range aux = this->getIntersect()->getRange();
      #ifdef DEBUG_BASICOP_EVAL
      PRINT_MSG(" aux=" << aux);
      #endif
      if(!aux.isUnknown())
      {
         Range intersect = result.intersectWith(aux);
         if(!intersect.isEmpty())
         {
            result = intersect;
         }
      }
      #ifdef DEBUG_BASICOP_EVAL
      PRINT_MSG(" = " << result);
      #endif
      return result;
   }

   /// Prints the content of the operation. I didn't it an operator overload
   /// because I had problems to access the members of the class outside it.
   void SigmaOp::print(std::ostream& OS) const
   {
      const char* quot = R"(")";
      OS << " " << quot << this << quot << R"( [label=")"
         << "SigmnaOp:";
      this->getIntersect()->print(OS);
      OS << "\"]\n";
      const auto V = this->getSource()->getValue();
      if(const auto* C = GetPointer<integer_cst>(V.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V.second)
         {
            printVarName(V.second, OS);
            OS << "." << V.first;
         }
         else
         {
            printVarName(V.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      if(SymbolicSource)
      {
         const auto V = SymbolicSource->getValue();
         if(const auto* C = GetPointer<integer_cst>(V.first))
         {
            OS << " " << C->value << " -> " << quot << this << quot << "\n";
         }
         else
         {
            OS << " " << quot;
            if(V.second)
            {
               printVarName(V.second, OS);
               OS << "." << V.first;
            }
            else
            {
               printVarName(V.first, OS);
            }
            OS << quot << " -> " << quot << this << quot << "\n";
         }
      }

      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // BinaryOp
   // ========================================================================== //
   /// A constraint like sink = source1 operation source2 intersect [l, u].
   class BinaryOp : public BasicOp
   {
    private:
      // The first operand.
      VarNode* source1;
      // The second operand.
      VarNode* source2;
      // The opcode of the operation.
      kind opcode;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      BinaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source1, VarNode* source2, kind opcode);
      ~BinaryOp() override = default;
      BinaryOp(const BinaryOp&) = delete;
      BinaryOp(BinaryOp&&) = delete;
      BinaryOp& operator=(const BinaryOp&) = delete;
      BinaryOp& operator=(BinaryOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::BinaryOpId;
      }
      static bool classof(BinaryOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::BinaryOpId;
      }

      /// Return the opcode of the operation.
      kind getOpcode() const
      {
         return opcode;
      }
      /// Returns the first operand of this operation.
      VarNode* getSource1() const
      {
         return source1;
      }
      /// Returns the second operand of this operation.
      VarNode* getSource2() const
      {
         return source2;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(std::ostream& OS) const override;
   };

   // The ctor.
   BinaryOp::BinaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source1, VarNode* source2, kind opcode) : BasicOp(std::move(intersect), sink, inst), source1(source1), source2(source2), opcode(opcode)
   {
   }

   /// Computes the interval of the sink based on the interval of the sources,
   /// the operation and the interval associated to the operation.
   /// Basically, this function performs the operation indicated in its opcode
   /// taking as its operands the source1 and the source2.
   Range BinaryOp::eval()
   {
      Range op1 = this->getSource1()->getRange();
      Range op2 = this->getSource2()->getRange();
      // Instruction bitwidth
      unsigned bw = getSink()->getBitWidth();
      Range result(Unknown, bw);

      // only evaluate if all operands are Regular
      if((op1.isRegular() || op1.isAnti()) && (op2.isRegular() || op2.isAnti()))
      {
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG(getSink()->getValue().first->ToString() << std::endl << op1 << "," << op2);
         #endif
         THROW_ASSERT(op1.getBitWidth() == bw, "");
         THROW_ASSERT(op2.getBitWidth() == bw, "");
         switch(this->getOpcode())
         {
            case plus_expr_K:
               result = op1.add(op2);
               break;
            case minus_expr_K:
               result = op1.sub(op2);
               break;
            case mult_expr_K:
               result = op1.mul(op2);
               break;
            //case Instruction::UDiv:
            //   result = op1.udiv(op2);
            //   break;
            //case Instruction::URem:
            //   result = op1.urem(op2);
            //   break;
            case trunc_div_expr_K:
               result = op1.sdiv(op2);
               break;
            case trunc_mod_expr_K:
               result = op1.srem(op2);
               break;
            case lshift_expr_K:
               result = op1.shl(op2);
               break;
            // TODO: check correctness: no logical shift right operation in GIMPLE?
            //case Instruction::LShr:
            //   result = op1.lshr(op2);
            //   break;
            case rshift_expr_K:
               result = op1.ashr(op2);
               break;
            case truth_and_expr_K:
               result = op1.And(op2);
               break;
            case truth_or_expr_K:
               result = op1.Or(op2);
               break;
            case truth_xor_expr_K:
               result = op1.Xor(op2);
               break;
            case eq_expr_K:
               result = op1.Eq(op2, bw);
               break;
            case ne_expr_K:
               result = op1.Ne(op2, bw);
               break;
            case unge_expr_K:
               result = op1.Uge(op2, bw);
               break;
            case ungt_expr_K:
               result = op1.Ugt(op2, bw);
               break;
            case unlt_expr_K:
               result = op1.Ult(op2, bw);
               break;
            case unle_expr_K:
               result = op1.Ule(op2, bw);
               break;
            case gt_expr_K:
               result = op1.Sgt(op2, bw);
               break;
            case ge_expr_K:
               result = op1.Sge(op2, bw);
               break;
            case lt_expr_K:
               result = op1.Slt(op2, bw);
               break;
            case le_expr_K:
               result = op1.Sle(op2, bw);
               break;
            
            case floor_div_expr_K:
            case floor_mod_expr_K:
            case ceil_div_expr_K:
            case ceil_mod_expr_K:
            case round_div_expr_K:
            case round_mod_expr_K:
            default:
               THROW_UNREACHABLE("Unhandled binary operation (" + tree_node::GetString(this->getOpcode()) + ")");
               break;
         }
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG("=" << result);
         #endif
         bool test = this->getIntersect()->getRange().isMaxRange();
         if(!test)
         {
            Range aux = this->getIntersect()->getRange();
            #ifdef DEBUG_BASICOP_EVAL
            PRINT_MSG("  aux=" << aux);
            #endif
            Range intersect = result.intersectWith(aux);
            if(!intersect.isEmpty())
            {
               result = intersect;
            }
         }
      }
      else
      {
         if(op1.isEmpty() || op2.isEmpty())
         {
            result = Range(Empty, bw);
         }
      }
      return result;
   }

   /// Pretty print.
   void BinaryOp::print(std::ostream& OS) const
   {
      const char* quot = R"(")";
      std::string opcodeName = tree_node::GetString(opcode);
      OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";
      const auto V1 = this->getSource1()->getValue();
      if(auto* C = GetPointer<integer_cst>(V1.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V1.second)
         {
            printVarName(V1.second, OS);
            OS << "." << V1.first;
         }
         else
         {
            printVarName(V1.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto V2 = this->getSource2()->getValue();
      if(auto* C = GetPointer<integer_cst>(V2.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V2.second)
         {
            printVarName(V2.second, OS);
            OS << "." << V2.first;
         }
         else
         {
            printVarName(V2.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // TernaryOp
   // ========================================================================== //
   class TernaryOp : public BasicOp
   {
    private:
      // The first operand.
      VarNode* source1;
      // The second operand.
      VarNode* source2;
      // The third operand.
      VarNode* source3;
      // The opcode of the operation.
      kind opcode;
      /// Computes the interval of the sink based on the interval of the sources,
      /// the operation and the interval associated to the operation.
      Range eval() override;

    public:
      TernaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source1, VarNode* source2, VarNode* source3, kind opcode);
      ~TernaryOp() override = default;
      TernaryOp(const TernaryOp&) = delete;
      TernaryOp(TernaryOp&&) = delete;
      TernaryOp& operator=(const TernaryOp&) = delete;
      TernaryOp& operator=(TernaryOp&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::TernaryOpId;
      }
      static bool classof(TernaryOp const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::TernaryOpId;
      }

      /// Return the opcode of the operation.
      kind getOpcode() const
      {
         return opcode;
      }
      /// Returns the first operand of this operation.
      VarNode* getSource1() const
      {
         return source1;
      }
      /// Returns the second operand of this operation.
      VarNode* getSource2() const
      {
         return source2;
      }
      /// Returns the third operand of this operation.
      VarNode* getSource3() const
      {
         return source3;
      }

      /// Prints the content of the operation. I didn't it an operator overload
      /// because I had problems to access the members of the class outside it.
      void print(std::ostream& OS) const override;
   };

   // The ctor.
   TernaryOp::TernaryOp(std::shared_ptr<BasicInterval> intersect, VarNode* sink, tree_nodeRef inst, VarNode* source1, VarNode* source2, VarNode* source3, kind opcode)
       : BasicOp(std::move(intersect), sink, inst), source1(source1), source2(source2), source3(source3), opcode(opcode)
   {
   }

   Range TernaryOp::eval()
   {
      Range op1 = this->getSource1()->getRange();
      Range op2 = this->getSource2()->getRange();
      Range op3 = this->getSource3()->getRange();
      // Instruction bitwidth
      unsigned bw = getSink()->getBitWidth();
      assert(bw == op2.getBitWidth());
      assert(bw == op3.getBitWidth());
      Range result(Unknown, bw, Min, Max);

      // only evaluate if all operands are Regular
      if((op1.isRegular() || op1.isAnti()) && (op2.isRegular() || op2.isAnti()) && (op3.isRegular() || op3.isAnti()))
      {
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG(getSink()->getValue().first->ToString() << std::endl << op1 << "?" << op2 << ":" << op3);
         #endif
         switch(this->getOpcode())
         {
            case cond_expr_K:
            {
               // Source1 is the selector
               if(op1 == Range(Regular, op1.getBitWidth(), 1, 1))
               {
                  result = op2;
               }
               else if(op1 == Range(Regular, op1.getBitWidth(), 0, 0))
               {
                  result = op3;
               }
               else
               {
                  auto* I = GetPointer<ternary_expr>(GET_NODE(getInstruction()));
                  auto* opV0 = GetPointer<binary_expr>(GET_NODE(I->op0));
                  THROW_ASSERT(opV0, "Condition of ternary operator not valid (" + GET_NODE(I->op0)->get_kind_text() + ")");

                  if(isCompare(opV0))
                  {
                     const auto CondOp0 = opV0->op0;
                     const auto CondOp1 = opV0->op1;
                     if(GET_NODE(CondOp0)->get_kind() == integer_cst_K || GET_NODE(CondOp1)->get_kind() == integer_cst_K)
                     {
                        auto variable = GET_NODE(CondOp0)->get_kind() == integer_cst_K ? CondOp1 : CondOp0;
                        auto constant = GET_NODE(CondOp0)->get_kind() == integer_cst_K ? GetPointer<integer_cst>(GET_NODE(CondOp0)) : GetPointer<integer_cst>(GET_NODE(CondOp1));
                        auto opV1 = I->op1;
                        auto opV2 = I->op2;
                        if(variable == opV1 || variable == opV2)
                        {
                           auto bw = op2.getBitWidth();
                           THROW_ASSERT(bw == op3.getBitWidth(), "Operands have different bitwidth");
                           Range CR(Regular, bw, constant->value, constant->value + 1);
                           kind pred = opV0->get_kind();
                           kind swappred = op_swap(pred);

                           Range tmpT = (variable == CondOp0) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);
                           THROW_ASSERT(!tmpT.isFullSet(), "");

                           if(variable == opV2)
                           {
                              Range FValues = Range(Range_base::getAnti(tmpT));
                              op3 = op3.intersectWith(FValues);
                           }
                           else
                           {
                              const Range& TValues = tmpT;
                              op2 = op2.intersectWith(TValues);
                           }
                        }
                     }
                  }
                  result = op2.unionWith(op3);
               }
               break;
            }
            default:
               break;
         }
         #ifdef DEBUG_BASICOP_EVAL
         PRINT_MSG("=" << result);
         #endif
         bool test = this->getIntersect()->getRange().isMaxRange();
         if(!test)
         {
            Range aux = this->getIntersect()->getRange();
            Range intersect = result.intersectWith(aux);
            if(!intersect.isEmpty())
            {
               result = intersect;
            }
         }
      }
      else
      {
         if(op1.isEmpty() || op2.isEmpty() || op3.isEmpty())
         {
            result = Range(Empty, bw);
         }
      }
      return result;
   }

   /// Pretty print.
   void TernaryOp::print(std::ostream& OS) const
   {
      const char* quot = R"(")";
      std::string opcodeName = tree_node::GetString(this->getOpcode());
      OS << " " << quot << this << quot << R"( [label=")" << opcodeName << "\"]\n";

      const auto V1 = this->getSource1()->getValue();
      if(const auto* C = GetPointer<integer_cst>(V1.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V1.second)
         {
            printVarName(V1.second, OS);
            OS << "." << V1.first;
         }
         else
         {
            printVarName(V1.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto V2 = this->getSource2()->getValue();
      if(const auto* C = GetPointer<integer_cst>(V2.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V2.second)
         {
            printVarName(V2.second, OS);
            OS << "." << V2.first;
         }
         else
         {
            printVarName(V2.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }

      const auto V3 = this->getSource3()->getValue();
      if(const auto* C = GetPointer<integer_cst>(V3.first))
      {
         OS << " " << C->value << " -> " << quot << this << quot << "\n";
      }
      else
      {
         OS << " " << quot;
         if(V3.second)
         {
            printVarName(V3.second, OS);
            OS << "." << V3.first;
         }
         else
         {
            printVarName(V3.first, OS);
         }
         OS << quot << " -> " << quot << this << quot << "\n";
      }
      const auto VS = this->getSink()->getValue();
      OS << " " << quot << this << quot << " -> " << quot;
      if(VS.second)
      {
         printVarName(VS.second, OS);
         OS << "." << VS.first;
      }
      else
      {
         printVarName(VS.first, OS);
      }
      OS << quot << "\n";
   }

   // ========================================================================== //
   // ControlDep
   // ========================================================================== //
   /// Specific type of BasicOp used in Nuutila's strongly connected
   /// components algorithm.
   class ControlDep : public BasicOp
   {
    private:
      VarNode* source;
      Range eval() override;

    public:
      ControlDep(VarNode* sink, VarNode* source);
      ~ControlDep() override;
      ControlDep(const ControlDep&) = delete;
      ControlDep(ControlDep&&) = delete;
      ControlDep& operator=(const ControlDep&) = delete;
      ControlDep& operator=(ControlDep&&) = delete;

      // Methods for RTTI
      OperationId getValueId() const override
      {
         return OperationId::ControlDepId;
      }
      static bool classof(ControlDep const* /*unused*/)
      {
         return true;
      }
      static bool classof(BasicOp const* BO)
      {
         return BO->getValueId() == OperationId::ControlDepId;
      }

      /// Returns the source of the operation.
      VarNode* getSource() const
      {
         return source;
      }

      void print(std::ostream& OS) const override;
   };

   ControlDep::ControlDep(VarNode* sink, VarNode* source) : BasicOp(std::make_shared<BasicInterval>(), sink, nullptr), source(source)
   {
   }

   ControlDep::~ControlDep() = default;

   Range ControlDep::eval()
   {
      return Range(Regular, MAX_BIT_INT);
   }

   void ControlDep::print(std::ostream& /*OS*/) const
   {
   }

   // ========================================================================== //
   // Nuutila
   // ========================================================================== //
   // The VarNodes type.
   using VarNodes = std::map<eValue, VarNode*>;

   // A map from variables to the operations where these variables are used.
   using UseMap = std::map<eValue, std::set<BasicOp*>>;

   // A map from variables to the operations where these
   // variables are present as bounds
   using SymbMap = std::map<eValue, std::set<BasicOp*>>;

   class Nuutila
   {
    public:
      VarNodes* variables;
      int index;
      std::map<eValue, int> dfs;
      std::map<eValue, eValue> root;
      std::set<eValue> inComponent;
      std::map<eValue, std::set<VarNode*>*> components;
      std::deque<eValue> worklist;
      #ifdef SCC_DEBUG
      bool checkWorklist();
      bool checkComponents();
      bool checkTopologicalSort(UseMap* useMap);
      bool hasEdge(std::set<VarNode*>* componentFrom, std::set<VarNode*>* componentTo, UseMap* useMap);
      #endif
    public:
      Nuutila(VarNodes* varNodes, UseMap* useMap, SymbMap* symbMap);
      ~Nuutila();
      Nuutila(const Nuutila&) = delete;
      Nuutila(Nuutila&&) = delete;
      Nuutila& operator=(const Nuutila&) = delete;
      Nuutila& operator=(Nuutila&&) = delete;

      void addControlDependenceEdges(SymbMap* symbMap, UseMap* useMap, VarNodes* vars);
      void delControlDependenceEdges(UseMap* useMap);
      void visit(eValue V, std::stack<eValue>& stack, UseMap* useMap);
      using iterator = std::deque<eValue>::reverse_iterator;
      iterator begin()
      {
         return worklist.rbegin();
      }
      iterator end()
      {
         return worklist.rend();
      }
   };

   /*
    *	Finds the strongly connected components in the constraint graph formed
    * by Variables and UseMap. The class receives the map of futures to insert
    * the control dependence edges in the constraint graph. These edges are removed
    * after the class is done computing the SCCs.
    */
   Nuutila::Nuutila(VarNodes* varNodes, UseMap* useMap, SymbMap* symbMap)
   {
      // Copy structures
      this->variables = varNodes;
      this->index = 0;

      // Iterate over all varnodes of the constraint graph
      for(auto vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
      {
         // Initialize DFS control variable for each Value in the graph
         auto V = vit->first;
         dfs[V] = -1;
      }
      addControlDependenceEdges(symbMap, useMap, varNodes);
      // Iterate again over all varnodes of the constraint graph
      for(auto vit = varNodes->begin(), vend = varNodes->end(); vit != vend; ++vit)
      {
         auto V = vit->first;
         // If the Value has not been visited yet, call visit for him
         if(dfs[V] < 0)
         {
            std::stack<eValue> pilha;
            visit(V, pilha, useMap);
         }
      }
      delControlDependenceEdges(useMap);

      #ifdef SCC_DEBUG
      THROW_ASSERT(checkWorklist(), "an inconsistency in SCC worklist have been found");
      THROW_ASSERT(checkComponents(), "a component has been used more than once");
      THROW_ASSERT(checkTopologicalSort(useMap), "topological sort is incorrect");
      #endif
   }

   Nuutila::~Nuutila()
   {
      for(auto mit = components.begin(), mend = components.end(); mit != mend; ++mit)
      {
         delete mit->second;
      }
   }

   /*
    *	Adds the edges that ensure that we solve a future before fixing its
    *  interval. I have created a new class: ControlDep edges, to represent
    *  the control dependencies. In this way, in order to delete these edges,
    *  one just need to go over the map of uses removing every instance of the
    *  ControlDep class.
    */
   void Nuutila::addControlDependenceEdges(SymbMap* symbMap, UseMap* useMap, VarNodes* vars)
   {
      for(auto sit = symbMap->begin(), send = symbMap->end(); sit != send; ++sit)
      {
         for(auto opit = sit->second.begin(), opend = sit->second.end(); opit != opend; ++opit)
         {
            auto source_value = vars->find(sit->first);
            VarNode* source = source_value->second;
            BasicOp* cdedge = new ControlDep((*opit)->getSink(), source);
            (*useMap)[sit->first].insert(cdedge);
         }
      }
   }

   /*
    *	Removes the control dependence edges from the constraint graph.
    */
   void Nuutila::delControlDependenceEdges(UseMap* useMap)
   {
      for(auto it = useMap->begin(), end = useMap->end(); it != end; ++it)
      {
         std::deque<ControlDep*> ops;
         for(auto sit : it->second)
         {
            ControlDep* op = nullptr;
            if((op = reinterpret_cast<ControlDep*>(sit)) != nullptr)
            {
               ops.push_back(op);
            }
         }

         for(ControlDep* op : ops)
         {
            // Add pseudo edge to the string
            const auto V = op->getSource()->getValue();
            if(const auto* C = GetPointer<integer_cst>(V.first))
            {
               pseudoEdgesString << " " << C->value << " -> ";
            }
            else
            {
               pseudoEdgesString << " " << '"';
               if(V.second)
               {
                  printVarName(V.second, pseudoEdgesString);
               }
               else
               {
                  printVarName(V.first, pseudoEdgesString);
               }
               pseudoEdgesString << '"' << " -> ";
            }
            const auto VS = op->getSink()->getValue();
            pseudoEdgesString << '"';
            if(VS.second)
            {
               printVarName(VS.second, pseudoEdgesString);
            }
            else
            {
               printVarName(VS.first, pseudoEdgesString);
            }
            pseudoEdgesString << '"';
            pseudoEdgesString << " [style=dashed]\n";
            // Remove pseudo edge from the map
            it->second.erase(op);
         }
      }
   }

   /*
    *	Finds SCCs using Nuutila's algorithm. This algorithm is divided in
    *  two parts. The first calls the recursive visit procedure on every node
    *  in the constraint graph. The second phase revisits these nodes,
    *  grouping them in components.
    */
   void Nuutila::visit(eValue V, std::stack<eValue>& stack, UseMap* useMap)
   {
      dfs[V] = index;
      ++index;
      root[V] = V;

      // Visit every node defined in an instruction that uses V
      for(auto sit = (*useMap)[V].begin(), send = (*useMap)[V].end(); sit != send; ++sit)
      {
         auto op = *sit;
         auto name = op->getSink()->getValue();
         if(dfs[name] < 0)
         {
            visit(name, stack, useMap);
         }
         if((!static_cast<bool>(inComponent.count(name))) && (dfs[root[V]] >= dfs[root[name]]))
         {
            root[V] = root[name];
         }
      }

      // The second phase of the algorithm assigns components to stacked nodes
      if(root[V] == V)
      {
         // Neither the worklist nor the map of components is part of Nuutila's
         // original algorithm. We are using these data structures to get a
         // topological ordering of the SCCs without having to go over the root
         // list once more.
         worklist.push_back(V);
         auto SCC = new std::set<VarNode*>;
         SCC->insert((*variables)[V]);
         inComponent.insert(V);
         while(!stack.empty() && (dfs[stack.top()] > dfs[V]))
         {
            auto node = stack.top();
            stack.pop();
            inComponent.insert(node);
            SCC->insert((*variables)[node]);
         }
         components[V] = SCC;
      }
      else
      {
         stack.push(V);
      }
   }

#ifdef SCC_DEBUG
   bool Nuutila::checkWorklist()
   {
      bool consistent = true;
      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto v = *nit;
         for(auto nit2 = this->begin(), nend2 = this->end(); nit2 != nend2; ++nit2)
         {
            auto v2 = *nit2;
            if(v == v2 && nit != nit2)
            {
               PRINT_MSG("[Nuutila::checkWorklist] Duplicated entry in worklist" << std::endl << v.first->ToString());
               consistent = false;
            }
         }
      }
      return consistent;
   }

   bool Nuutila::checkComponents()
   {
      bool isConsistent = true;
      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto component = this->components[*nit];
         for(auto nit2 = this->begin(), nend2 = this->end(); nit2 != nend2; ++nit2)
         {
            auto component2 = this->components[*nit2];
            if(component == component2 && nit != nit2)
            {
               PRINT_MSG("[Nuutila::checkComponent] Component [" << component << ", " << component->size() << "]");
               isConsistent = false;
            }
         }
      }
      return isConsistent;
   }

   /**
    * Check if a component has an edge to another component
    */
   bool Nuutila::hasEdge(std::set<VarNode*>* componentFrom, std::set<VarNode*>* componentTo, UseMap* useMap)
   {
      for(auto vit = componentFrom->begin(), vend = componentFrom->end(); vit != vend; ++vit)
      {
         const auto source = (*vit)->getValue();
         for(auto sit = (*useMap)[source].begin(), send = (*useMap)[source].end(); sit != send; ++sit)
         {
            BasicOp* op = *sit;
            if(componentTo->count(op->getSink()) != 0)
               return true;
         }
      }
      return false;
   }

   bool Nuutila::checkTopologicalSort(UseMap* useMap)
   {
      bool isConsistent = true;
      std::map<std::set<VarNode*>*, bool> visited;
      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto component = this->components[*nit];
         visited[component] = false;
      }

      for(auto nit = this->begin(), nend = this->end(); nit != nend; ++nit)
      {
         auto component = this->components[*nit];
         if(!visited[component])
         {
            visited[component] = true;
            // check if this component points to another component that has already
            // been visited
            for(auto nit2 = this->begin(), nend2 = this->end(); nit2 != nend2; ++nit2)
            {
               auto component2 = this->components[*nit2];
               if(nit != nit2 && visited[component2] && hasEdge(component, component2, useMap))
                  isConsistent = false;
            }
         }
         else
         {
            PRINT_MSG("[Nuutila::checkTopologicalSort] Component visited more than once time");
         }
      }
      return isConsistent;
   }

#endif

   // ========================================================================== //
   // Meet
   // ========================================================================== //
   class Meet
   {
   private:
      static APInt getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val);
      static APInt getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val);

   public:
      static bool widen(BasicOp* op, const std::vector<APInt>* constantvector);
      static bool narrow(BasicOp* op, const std::vector<APInt>* constantvector);
      static bool crop(BasicOp* op, const std::vector<APInt>* constantvector);
      static bool growth(BasicOp* op, const std::vector<APInt>* constantvector);
      static bool fixed(BasicOp* op);
   };

   /*
    * Get the first constant from vector greater than val
    */
   APInt Meet::getFirstGreaterFromVector(const std::vector<APInt>& constantvector, const APInt& val)
   {
      for(const auto& vapint : constantvector)
      {
         if(vapint >= val)
         {
            return vapint;
         }
      }
      return Max;
   }

   /*
    * Get the first constant from vector less than val
    */
   APInt Meet::getFirstLessFromVector(const std::vector<APInt>& constantvector, const APInt& val)
   {
      for(auto vit = constantvector.rbegin(), vend = constantvector.rend(); vit != vend; ++vit)
      {
         const auto& vapint = *vit;
         if(vapint <= val)
         {
            return vapint;
         }
      }
      return Min;
   }

   bool Meet::fixed(BasicOp* op)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      op->getSink()->setRange(newInterval);
      #ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         auto instID = GET_NODE(op->getInstruction())->index;
         PRINT_MSG("FIXED::%" << instID << ": " << oldInterval << " -> " << newInterval);
      }
      else
      {
         PRINT_MSG("FIXED::%artificial phi : " << oldInterval << " -> " << newInterval);
      }
      #endif
      return oldInterval != newInterval;
   }

   /// This is the meet operator of the growth analysis. The growth analysis
   /// will change the bounds of each variable, if necessary. Initially, each
   /// variable is bound to either the undefined interval, e.g. [., .], or to
   /// a constant interval, e.g., [3, 15]. After this analysis runs, there will
   /// be no undefined interval. Each variable will be either bound to a
   /// constant interval, or to [-, c], or to [c, +], or to [-, +].
   bool Meet::widen(BasicOp* op, const std::vector<APInt>* constantvector)
   {
      THROW_ASSERT(constantvector, "Invalid pointer to constant vector");
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      unsigned bw = oldInterval.getBitWidth();
      if(oldInterval.isUnknown() || oldInterval.isEmpty() || oldInterval.isAnti() || newInterval.isEmpty() || newInterval.isAnti())
      {
         if(oldInterval.isAnti() && newInterval.isAnti() && newInterval != oldInterval)
         {
            auto oldAnti = Range::getAnti(oldInterval);
            auto newAnti = Range::getAnti(newInterval);
            const APInt oldLower = oldAnti.getLower();
            const APInt oldUpper = oldAnti.getUpper();
            const APInt newLower = newAnti.getLower();
            const APInt newUpper = newAnti.getUpper();
            APInt nlconstant = getFirstGreaterFromVector(*constantvector, newLower);
            APInt nuconstant = getFirstLessFromVector(*constantvector, newUpper);

            if((newLower > oldLower) && (newUpper < oldUpper))
            {
               op->getSink()->setRange(Range(Anti, bw, nlconstant, nuconstant));
            }
            else
            {
               if(newLower > oldLower)
               {
                  op->getSink()->setRange(Range(Anti, bw, nlconstant, oldUpper));
               }
               else if(newUpper < oldUpper)
               {
                  op->getSink()->setRange(Range(Anti, bw, oldLower, nuconstant));
               }
            }
         }
         else
         {
            op->getSink()->setRange(newInterval);
         }
      }
      else
      {
         const APInt& oldLower = oldInterval.getLower();
         const APInt& oldUpper = oldInterval.getUpper();
         const APInt& newLower = newInterval.getLower();
         const APInt& newUpper = newInterval.getUpper();

         // Jump-set
         APInt nlconstant = getFirstLessFromVector(*constantvector, newLower);
         APInt nuconstant = getFirstGreaterFromVector(*constantvector, newUpper);
         if((newLower < oldLower) && (newUpper > oldUpper))
         {
            op->getSink()->setRange(Range(Regular, bw, nlconstant, nuconstant));
         }
         else
         {
            if(newLower < oldLower)
            {
               op->getSink()->setRange(Range(Regular, bw, nlconstant, oldUpper));
            }
            else if(newUpper > oldUpper)
            {
               op->getSink()->setRange(Range(Regular, bw, oldLower, nuconstant));
            }
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();

      #ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         auto instID = GET_NODE(op->getInstruction())->index;
         PRINT_MSG("WIDEN::%" << instID << ": " << oldInterval << " -> " << newInterval << " -> " << sinkInterval);
      }
      else
      {
         PRINT_MSG("WIDEN::%artificial phi : " << oldInterval << " -> " << newInterval << " -> " << sinkInterval);
      }
      #endif

      return oldInterval != sinkInterval;
   }

   bool Meet::growth(BasicOp* op, const std::vector<APInt>* /*constantvector*/)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();
      if(oldInterval.isUnknown() || oldInterval.isEmpty() || oldInterval.isAnti() || newInterval.isEmpty() || newInterval.isAnti())
      {
         op->getSink()->setRange(newInterval);
      }
      else
      {
         unsigned bw = oldInterval.getBitWidth();
         const APInt& oldLower = oldInterval.getLower();
         const APInt& oldUpper = oldInterval.getUpper();
         const APInt& newLower = newInterval.getLower();
         const APInt& newUpper = newInterval.getUpper();
         if(newLower < oldLower)
         {
            if(newUpper > oldUpper)
            {
               op->getSink()->setRange(Range(Regular, bw));
            }
            else
            {
               op->getSink()->setRange(Range(Regular, bw, Min, oldUpper));
            }
         }
         else if(newUpper > oldUpper)
         {
            op->getSink()->setRange(Range(Regular, bw, oldLower, Max));
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
      #ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         auto instID = GET_NODE(op->getInstruction())->index;
         PRINT_MSG("GROWTH::%" << instID << ": " << oldInterval << " -> " << sinkInterval);
      }
      else
      {
         PRINT_MSG("GROWTH::%artificial phi : " << oldInterval << " -> " << sinkInterval);
      }
      #endif
      return oldInterval != sinkInterval;
   }

   /// This is the meet operator of the cropping analysis. Whereas the growth
   /// analysis expands the bounds of each variable, regardless of intersections
   /// in the constraint graph, the cropping analysis shrinks these bounds back
   /// to ranges that respect the intersections.
   bool Meet::narrow(BasicOp* op, const std::vector<APInt>* constantvector)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();
      unsigned bw = oldInterval.getBitWidth();

      if(oldInterval.isAnti() || newInterval.isAnti() || oldInterval.isEmpty() || newInterval.isEmpty())
      {
         if(oldInterval.isAnti() && newInterval.isAnti() && newInterval != oldInterval)
         {
            auto oldAnti = Range::getAnti(oldInterval);
            auto newAnti = Range::getAnti(newInterval);
            const APInt& oLower = oldAnti.getLower();
            const APInt& oUpper = oldAnti.getUpper();
            const APInt& nLower = newAnti.getLower();
            const APInt& nUpper = newAnti.getUpper();
            APInt nlconstant = getFirstGreaterFromVector(*constantvector, nLower);
            APInt nuconstant = getFirstLessFromVector(*constantvector, nUpper);
            THROW_ASSERT(oLower != Min, "");
            const APInt& smin = std::max(oLower, nlconstant);
            if(oLower != smin)
            {
               op->getSink()->setRange(Range(Anti, bw, smin, oUpper));
            }
            THROW_ASSERT(oUpper != Max, "");
            const APInt& smax = std::min(oUpper, nuconstant);
            if(oUpper != smax)
            {
               auto sinkRange = op->getSink()->getRange();
               if(sinkRange.isAnti())
               {
                  auto sinkAnti = Range::getAnti(sinkRange);
                  op->getSink()->setRange(Range(Anti, bw, sinkAnti.getLower(), smax));
               }
               else
               {
                  op->getSink()->setRange(Range(Anti, bw, sinkRange.getLower(), smax));
               }
            }
         }
         else
         {
            op->getSink()->setRange(newInterval);
         }
      }
      else
      {
         const APInt oLower = oldInterval.getLower();
         const APInt oUpper = oldInterval.getUpper();
         const APInt nLower = newInterval.getLower();
         const APInt nUpper = newInterval.getUpper();
         if((oLower == Min) && (nLower == Min))
         {
            op->getSink()->setRange(Range(Regular, bw, nLower, oUpper));
         }
         else
         {
            const APInt& smin = std::min(oLower, nLower);
            if(oLower != smin)
            {
               op->getSink()->setRange(Range(Regular, bw, smin, oUpper));
            }
         }
         if(!op->getSink()->getRange().isAnti())
         {
            if((oUpper == Max) && (nUpper == Max))
            {
               op->getSink()->setRange(Range(Regular, bw, op->getSink()->getRange().getLower(), nUpper));
            }
            else
            {
               const APInt& smax = std::max(oUpper, nUpper);
               if(oUpper != smax)
               {
                  op->getSink()->setRange(Range(Regular, bw, op->getSink()->getRange().getLower(), smax));
               }
            }
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
      #ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         auto instID = GET_NODE(op->getInstruction())->index;
         PRINT_MSG("NARROW::%" << instID << ": " << oldInterval << " -> " << sinkInterval);
      }
      else
      {
         PRINT_MSG("NARROW::%artificial phi : " << oldInterval << " -> " << sinkInterval);
      }
      #endif
      return oldInterval != sinkInterval;
   }

   bool Meet::crop(BasicOp* op, const std::vector<APInt>* /*constantvector*/)
   {
      const auto oldInterval = op->getSink()->getRange();
      Range newInterval = op->eval();

      if(oldInterval.isAnti() || newInterval.isAnti() || oldInterval.isEmpty() || newInterval.isEmpty())
      {
         op->getSink()->setRange(newInterval);
      }
      else
      {
         unsigned bw = oldInterval.getBitWidth();
         char abstractState = op->getSink()->getAbstractState();
         if((abstractState == '-' || abstractState == '?') && (newInterval.getLower() > oldInterval.getLower()))
         {
            op->getSink()->setRange(Range(Regular, bw, newInterval.getLower(), oldInterval.getUpper()));
         }

         if((abstractState == '+' || abstractState == '?') && (newInterval.getUpper() < oldInterval.getUpper()))
         {
            op->getSink()->setRange(Range(Regular, bw, oldInterval.getLower(), newInterval.getUpper()));
         }
      }
      const auto& sinkInterval = op->getSink()->getRange();
      #ifdef LOG_TRANSACTIONS
      if(op->getInstruction())
      {
         auto instID = GET_NODE(op->getInstruction())->index);
         PRINT_MSG("CROP::%" << instID << ": " << oldInterval << " -> " << sinkInterval);
      }
      else
      {
         PRINT_MSG("CROP::%artificial phi : " << oldInterval << " -> " << sinkInterval);
      }
      #endif
      return oldInterval != sinkInterval;
   }

   /// This class is used to store the intersections that we get in the branches.
   /// I decided to write it because I think it is better to have an object
   /// to store these information than create a lot of maps
   /// in the ConstraintGraph class.
   class ValueBranchMap
   {
    private:
      const tree_nodeRef V;
      const blocRef BBTrue;
      const blocRef BBFalse;
      std::shared_ptr<BasicInterval> ItvT;
      std::shared_ptr<BasicInterval> ItvF;

    public:
      ValueBranchMap(const tree_nodeRef V, const blocRef BBTrue, const blocRef BBFalse, std::shared_ptr<BasicInterval> ItvT, std::shared_ptr<BasicInterval> ItvF) : V(V), BBTrue(BBTrue), BBFalse(BBFalse), ItvT(ItvT), ItvF(ItvF)
      {
      }
      ~ValueBranchMap() = default;
      ValueBranchMap(const ValueBranchMap&) = default;
      ValueBranchMap(ValueBranchMap&&) = default;
      ValueBranchMap& operator=(const ValueBranchMap&) = delete;
      ValueBranchMap& operator=(ValueBranchMap&&) = delete;

      /// Get the "false side" of the branch
      const blocRef getBBFalse() const
      {
         return BBFalse;
      }
      /// Get the "true side" of the branch
      const blocRef getBBTrue() const
      {
         return BBTrue;
      }
      /// Get the interval associated to the true side of the branch
      std::shared_ptr<BasicInterval> getItvT() const
      {
         return ItvT;
      }
      /// Get the interval associated to the false side of the branch
      std::shared_ptr<BasicInterval> getItvF() const
      {
         return ItvF;
      }
      /// Get the value associated to the branch.
      const tree_nodeRef getV() const
      {
         return V;
      }
      /// Change the interval associated to the true side of the branch
      void setItvT(std::shared_ptr<BasicInterval> Itv)
      {
         this->ItvT = Itv;
      }
      /// Change the interval associated to the false side of the branch
      void setItvF(std::shared_ptr<BasicInterval> Itv)
      {
         this->ItvF = Itv;
      }
   };

   /// This is pretty much the same thing as ValueBranchMap
   /// but implemented specifically for switch instructions
   class ValueSwitchMap
   {
    private:
      const tree_nodeRef V;
      std::vector<std::pair<std::shared_ptr<BasicInterval>, const blocRef>> BBsuccs;

    public:
      ValueSwitchMap(const tree_nodeRef V, std::vector<std::pair<std::shared_ptr<BasicInterval>, const blocRef>>& BBsuccs) : V(V), BBsuccs(BBsuccs)
      {
      }
      ~ValueSwitchMap() = default;
      ValueSwitchMap(const ValueSwitchMap&) = default;
      ValueSwitchMap(ValueSwitchMap&&) = default;
      ValueSwitchMap& operator=(const ValueSwitchMap&) = delete;
      ValueSwitchMap& operator=(ValueSwitchMap&&) = delete;

      /// Get the corresponding basic block
      const blocRef getBB(unsigned idx) const
      {
         return BBsuccs[idx].second;
      }
      /// Get the interval associated to the switch case idx
      std::shared_ptr<BasicInterval> getItv(unsigned idx) const
      {
         return BBsuccs[idx].first;
      }
      // Get how many cases this switch has
      unsigned getNumOfCases() const
      {
         return BBsuccs.size();
      }
      /// Get the value associated to the switch.
      const tree_nodeRef getV() const
      {
         return V;
      }
      /// Change the interval associated to the true side of the branch
      void setItv(unsigned idx, std::shared_ptr<BasicInterval> Itv)
      {
         this->BBsuccs[idx].first = Itv;
      }
   };

   // ========================================================================== //
   // ConstraintGraph
   // ========================================================================== //
   // The Operations type.
   using GenOprs = std::set<BasicOp*>;

   // A map from varnodes to the operation in which this variable is defined
   using DefMap = std::map<eValue, BasicOp*>;

   using ValuesBranchMap = std::map<const tree_nodeRef, ValueBranchMap>;

   using ValuesSwitchMap = std::map<tree_nodeRef, ValueSwitchMap>;

   class ConstraintGraph
   {
   protected:
      // The variables of the source program and the nodes which represent them.
      VarNodes vars;
      // The operations of the source program and the nodes which represent them.
      GenOprs oprs;

      // Perform the widening and narrowing operations
      void update(const UseMap& compUseMap, std::set<eValue>& actv, bool (*meet)(BasicOp* op, const std::vector<APInt>* constantvector))
      {
         while(!actv.empty())
         {
            const auto V = *actv.begin();
            actv.erase(V);
            #ifdef DEBUG_CGRAPH
            PRINT_MSG("-> update: " << V.first->ToString());
            #endif

            // The use list.
            const auto& L = compUseMap.find(V)->second;

            for(BasicOp* op : L)
            {
               #ifdef DEBUG_CGRAPH
               PRINT_MSG("  > " << op->getSink());
               #endif
               if(meet(op, &constantvector))
               {
                  // I want to use it as a set, but I also want
                  // keep an order or insertions and removals.
                  auto val = op->getSink()->getValue();
                  actv.insert(val);
               }
            }
         }
      }
      
      void update(unsigned nIterations, const UseMap& compUseMap, std::set<eValue>& actv)
      {
         std::list<eValue> queue(actv.begin(), actv.end());
         actv.clear();
         while(!queue.empty())
         {
            const auto V = queue.front();
            queue.pop_front();
            // The use list.
            const auto& L = compUseMap.find(V)->second;
            for(auto op : L)
            {
               if(nIterations == 0)
               {
                  return;
               }
               --nIterations;
               if(Meet::fixed(op))
               {
                  auto next = op->getSink()->getValue();
                  if(std::find(queue.begin(), queue.end(), next) == queue.end())
                  {
                     queue.push_back(next);
                  }
               }
            }
         }
      }

      virtual void preUpdate(const UseMap& compUseMap, std::set<eValue>& entryPoints) = 0;
      virtual void posUpdate(const UseMap& compUseMap, std::set<eValue>& activeVars, const std::set<VarNode*>* component) = 0;

   private:
      int debug_level;
      
      // A map from variables to the operations that define them
      DefMap defMap;
      // A map from variables to the operations where these variables are used.
      UseMap useMap;
      // A map from variables to the operations where these
      // variables are present as bounds
      SymbMap symbMap;
      // This data structure is used to store intervals, basic blocks and intervals
      // obtained in the branches.
      ValuesBranchMap valuesBranchMap;
      ValuesSwitchMap valuesSwitchMap;

      // Vector containing the constants from a SCC
      // It is cleared at the beginning of every SCC resolution
      std::vector<APInt> constantvector;

      void buildValueBranchMap(const gimple_cond* br, blocRef branchBB, std::map<unsigned int, blocRef>& BBs)
      {
         THROW_ASSERT(GET_NODE(br->op0)->get_kind() == ssa_name_K, "Non SSA variable found in branch");
         const auto Cond = GET_NODE(branchOpRecurse(br->op0));

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
            " <--RA constraint graph: branch condition is " << Cond->get_kind_text());

         if(auto* bin_op = GetPointer<binary_expr>(Cond))
         {
            if(!isCompare(bin_op))
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: not a compare codition, skipping...");
               return;
            }

            // Create VarNodes for comparison operands explicitly
            addVarNode(bin_op->op0, nullptr);
            addVarNode(bin_op->op1, nullptr);

            // Gets the successors of the current basic block.
            const auto TBlock = BBs.at(branchBB->true_edge);
            const auto FBlock = BBs.at(branchBB->false_edge);

            // We have a Variable-Constant comparison.
            const auto Op0 = GET_NODE(bin_op->op0);
            const auto Op1 = GET_NODE(bin_op->op1);
            const struct integer_cst* constant = nullptr;
            tree_nodeRef variable = nullptr;

            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
               " <--RA constraint graph: Op0 is " << Op0->get_kind_text() << " and Op1 is " << Op1->get_kind_text());

            // If both operands are constants, nothing to do here
            if(Op0->get_kind() == integer_cst_K && Op1->get_kind() == integer_cst_K)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: both operands are constants, skipping...");
               return;
            }

            // Then there are two cases: variable being compared to a constant,
            // or variable being compared to another variable

            // Op0 is constant, Op1 is variable
            if((constant = GetPointer<integer_cst>(Op0)) != nullptr)
            {
               variable = bin_op->op1;
               // Op0 is variable, Op1 is constant
            }
            else if((constant = GetPointer<integer_cst>(Op1)) != nullptr)
            {
               variable = bin_op->op0;
            }
            // Both are variables
            // which means constant == 0 and variable == 0

            if(constant != nullptr)
            {
               kind pred = bin_op->get_kind();
               kind swappred = op_swap(pred);
               unsigned bw = LoadStoreOperationBW(variable, nullptr);
               Range CR(Regular, bw, constant->value, APInt(constant->value) + 1);

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: variable bitwidth is " << bw << " and constant value is " << constant->value);

               Range tmpT = (variable == bin_op->op0) ? Range::makeSatisfyingCmpRegion(pred, CR) : Range::makeSatisfyingCmpRegion(swappred, CR);

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: condition is true on " << tmpT);

               Range TValues = tmpT.isFullSet() ? Range(Regular, bw) : tmpT;
               Range FValues = tmpT.isFullSet() ? Range(Empty, bw) : Range(Range_base::getAnti(TValues));

               // Create the interval using the intersection in the branch.
               std::shared_ptr<BasicInterval> BT = std::make_shared<BasicInterval>(TValues);
               std::shared_ptr<BasicInterval> BF = std::make_shared<BasicInterval>(FValues);

               ValueBranchMap VBM(variable, TBlock, FBlock, BT, BF);
               valuesBranchMap.insert(std::make_pair(variable, VBM));

               // TODO: check correctness
               // Do the same for the operand of variable (if variable is a cast
               // instruction)
               if(auto* castinst = GetPointer<cast_expr>(GET_NODE(variable)))
               {
                  tree_nodeRef variable_0 = castinst->op;

                  std::shared_ptr<BasicInterval> BT = std::make_shared<BasicInterval>(TValues);
                  std::shared_ptr<BasicInterval> BF = std::make_shared<BasicInterval>(FValues);

                  ValueBranchMap VBM(variable_0, TBlock, FBlock, BT, BF);
                  valuesBranchMap.insert(std::make_pair(variable_0, VBM));
               }
            }
            else
            {
               kind pred = bin_op->get_kind();
               kind invPred = op_inv(pred);

               unsigned bw0 = LoadStoreOperationBW(bin_op->op0, nullptr);
               unsigned bw1 = LoadStoreOperationBW(bin_op->op1, nullptr);
               THROW_ASSERT(bw0 == bw1, "Operands of same operation have different bitwidth "
                  "(Op0 = " + boost::lexical_cast<std::string>(bw0) + ", Op1 = " + boost::lexical_cast<std::string>(bw1) + ").");

               Range CR(Unknown, bw0);

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: variables bitwidth is " << bw0);

               // Symbolic intervals for op0
               std::shared_ptr<BasicInterval> STOp0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op1, pred));
               std::shared_ptr<BasicInterval> SFOp0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op1, invPred));

               ValueBranchMap VBMOp0(bin_op->op0, TBlock, FBlock, STOp0, SFOp0);
               valuesBranchMap.insert(std::make_pair(bin_op->op0, VBMOp0));

               // TODO: check correctness
               // Symbolic intervals for operand of op0 (if op0 is a cast instruction)
               if(auto* castinst = GetPointer<cast_expr>(Op0))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                     " <--RA constraint graph: Op0 is a cast expression");
                  tree_nodeRef Op0_0 = castinst->op;
               
                  std::shared_ptr<BasicInterval> STOp0_0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op1, pred));
                  std::shared_ptr<BasicInterval> SFOp0_0 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op1, invPred));
               
                  ValueBranchMap VBMOp1_1(Op0_0, TBlock, FBlock, STOp0_0, SFOp0_0);
                  valuesBranchMap.insert(std::make_pair(Op0_0, VBMOp1_1));
               }

               // Symbolic intervals for op1
               std::shared_ptr<BasicInterval> STOp1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op0, invPred));
               std::shared_ptr<BasicInterval> SFOp1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op0, pred));
               ValueBranchMap VBMOp1(bin_op->op1, TBlock, FBlock, STOp1, SFOp1);
               valuesBranchMap.insert(std::make_pair(bin_op->op1, VBMOp1));

               // TODO: check correctness
               // Symbolic intervals for operand of op1 (if op1 is a cast instruction)
               if(auto* castinst = GetPointer<cast_expr>(Op1))
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                     " <--RA constraint graph: Op1 is a cast expression");
                  tree_nodeRef Op1_1 = castinst->op;
               
                  std::shared_ptr<BasicInterval> STOp1_1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op0, pred));
                  std::shared_ptr<BasicInterval> SFOp1_1 = std::shared_ptr<BasicInterval>(new SymbInterval(CR, bin_op->op0, invPred));
               
                  ValueBranchMap VBMOp1_1(Op1_1, TBlock, FBlock, STOp1_1, SFOp1_1);
                  valuesBranchMap.insert(std::make_pair(Op1_1, VBMOp1_1));
               }
            }
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
               " <--RA constraint graph: not a compare codition, skipping...");
         }
      }

      void buildValueMultiIfMap(const gimple_multi_way_if* mwi, blocRef /*mwifBB*/, std::map<unsigned int, blocRef>& /*BBs*/)
      {
         const auto& cond_list = mwi->list_of_cond;

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
            " <--RA constraint graph: multi if with " << cond_list.size() << " conditions");

         for(const auto& [cond_ref, bb_id] : cond_list)
         {
            if(cond_ref)
            {
               const auto cond = GET_NODE(cond_ref);

               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: condition of type " << cond->get_kind_text() << " to BB " << bb_id);
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: condition missing to BB " << bb_id);
            }

            // TODO: compute range interval for this case as in buildValueSwitchMap
            THROW_UNREACHABLE("Not yet implemented - multi way if value build");
         }
      }

      void buildValueMaps(std::map<unsigned int, blocRef>& BBs, const application_managerRef /*AppM*/)
      {
         for(const auto& [bb_id, bb] : BBs)
         {
            const auto& stmt_list = bb->CGetStmtList();
            if(stmt_list.empty())
            {
               continue;
            }

            const auto terminator = GET_NODE(stmt_list.back());

            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
               " <--RA constraint graph: BB " << bb->number << " has terminator type " << terminator->get_kind_text());

            if(auto* br = GetPointer<gimple_cond>(terminator))
            {
               buildValueBranchMap(br, bb, BBs);
            }
            else if(auto* mwi = GetPointer<gimple_multi_way_if>(terminator))
            {
               buildValueMultiIfMap(mwi, bb, BBs);
            }
         }
      }

      void addCallOp(tree_nodeRef I, application_managerRef AppM)
      {
         const auto* assign = GetPointer<gimple_assign>(GET_NODE(I));
         const auto* call_op = GetPointer<call_expr>(GET_NODE(assign->op1));
         THROW_ASSERT(call_op, "");

         addVarNode(I, nullptr);
      }

      void addSigmaOp(tree_nodeRef I, blocRef BB, application_managerRef AppM)
      {
         const auto* phi = GetPointer<gimple_phi>(GET_NODE(I));
         THROW_ASSERT(phi, "");
         THROW_ASSERT(phi->CGetDefEdgesList().size() == 1U, "");

         // Create the sink.
         VarNode* sink = addVarNode(I, nullptr);
         std::shared_ptr<BasicInterval> BItv;
         SigmaOp* sigmaOp = nullptr;

         //const BasicBlock* thisbb = Sigma->getParent();
         for(const auto [operand, edge_index] : phi->CGetDefEdgesList())
         {
            VarNode* source = addVarNode(operand, nullptr);

            // Create the operation (two cases from: branch or switch)
            auto vbmit = this->valuesBranchMap.find(operand);

            // Branch case
            if(vbmit != this->valuesBranchMap.end())
            {
               const ValueBranchMap& VBM = vbmit->second;
               if(BB == VBM.getBBTrue())
               {
                  BItv = VBM.getItvT();
               }
               else
               {
                  if(BB == VBM.getBBFalse())
                  {
                     BItv = VBM.getItvF();
                  }
               }
            }
            else
            {
               // Switch case
               auto vsmit = this->valuesSwitchMap.find(operand);

               if(vsmit == this->valuesSwitchMap.end())
               {
                  continue;
               }

               const ValueSwitchMap& VSM = vsmit->second;
               // Find out which case are we dealing with
               for(unsigned idx = 0, e = VSM.getNumOfCases(); idx < e; ++idx)
               {
                  const blocRef bb = VSM.getBB(idx);
                  if(bb == BB)
                  {
                     BItv = VSM.getItv(idx);
                     break;
                  }
               }
            }

            if(BItv == nullptr)
            {
               sigmaOp = new SigmaOp(std::make_shared<BasicInterval>(getGIMPLE_range(reinterpret_cast<const gimple_node*>(phi), AppM)), sink, I, source, nullptr, phi->get_kind());
            }
            else
            {
               #ifdef DEBUG_CGRAPH
               PRINT_MSG("Add SigmaOp: " << BItv << std::endl << BItv->getRange());
               #endif
               VarNode* SymbSrc = nullptr;
               if(auto symb = std::dynamic_pointer_cast<SymbInterval>(BItv))
               {
                  auto bound = symb->getBound();
                  SymbSrc = addVarNode(bound.first, bound.second);
               }
               sigmaOp = new SigmaOp(BItv, sink, I, source, SymbSrc, phi->get_kind());
               if(SymbSrc)
               {
                  this->useMap.find(SymbSrc->getValue())->second.insert(sigmaOp);
               }
            }

            // Insert the operation in the graph.
            this->oprs.insert(sigmaOp);

            // Insert this definition in defmap
            this->defMap[sink->getValue()] = sigmaOp;

            // Inserts the sources of the operation in the use map list.
            this->useMap.find(source->getValue())->second.insert(sigmaOp);
         }
      }

      /// Adds an UnaryOp in the graph.
      void addUnaryOp(tree_nodeRef I, application_managerRef AppM)
      {
         const auto* assign = GetPointer<gimple_assign>(GET_NODE(I));
         const auto* un_op = GetPointer<unary_expr>(GET_NODE(assign->op1));
         THROW_ASSERT(un_op, "");

         // Create the sink.
         VarNode* sink = addVarNode(I, nullptr);
         // Create the source.
         VarNode* source = nullptr;

         // TODO: update with useful operations
         switch(un_op->get_kind())
         {
            //case Instruction::Trunc:
            //case Instruction::ZExt:
            //case Instruction::SExt:
            //   source = addVarNode(I->getOperand(0), nullptr, DL);
            //   break;

            case abs_expr_K:
            case convert_expr_K:
            case nop_expr_K:
            case view_convert_expr_K:
            default:
               return;
         }
         std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getGIMPLE_range(reinterpret_cast<const gimple_node*>(assign), AppM));
         UnaryOp* UOp = new UnaryOp(BI, sink, I, source, un_op->get_kind());

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
            " <--RA constraint graph: added UnaryOp for " << un_op->get_kind_text() << " with range " << BI);

         this->oprs.insert(UOp);
         // Insert this definition in defmap
         this->defMap[sink->getValue()] = UOp;
         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source->getValue())->second.insert(UOp);
      }

      /// XXX: I'm assuming that we are always analyzing bytecodes in e-SSA form.
      /// So, we don't have intersections associated with binary oprs.
      /// To have an intersect, we must have a Sigma instruction.
      /// Adds a BinaryOp in the graph.
      void addBinaryOp(tree_nodeRef I, application_managerRef AppM)
      {
         const auto* assign = GetPointer<gimple_assign>(GET_NODE(I));
         const auto* bin_op = GetPointer<binary_expr>(GET_NODE(assign->op1));
         THROW_ASSERT(bin_op, "");
         
         // Create the sink.
         VarNode* sink = addVarNode(I, nullptr);

         // Create the sources.
         VarNode* source1 = addVarNode(bin_op->op0, nullptr);
         VarNode* source2 = addVarNode(bin_op->op1, nullptr);

         // Create the operation using the intersect to constrain sink's interval.
         std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getGIMPLE_range(reinterpret_cast<const gimple_node*>(assign), AppM));
         BinaryOp* BOp = new BinaryOp(BI, sink, I, source1, source2, bin_op->get_kind());

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
            " <--RA constraint graph: added BinaryOp for " << bin_op->get_kind_text() << " with range " << BI);

         // Insert the operation in the graph.
         this->oprs.insert(BOp);

         // Insert this definition in defmap
         this->defMap[sink->getValue()] = BOp;

         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source1->getValue())->second.insert(BOp);
         this->useMap.find(source2->getValue())->second.insert(BOp);
      }

      void addTernaryOp(tree_nodeRef I, application_managerRef AppM)
      {
         const auto* assign = GetPointer<gimple_assign>(GET_NODE(I));
         const auto* ter_op = GetPointer<ternary_expr>(GET_NODE(assign->op1));
         THROW_ASSERT(ter_op, "");
         // Create the sink.
         VarNode* sink = addVarNode(I, nullptr);

         // Create the sources.
         VarNode* source1 = addVarNode(ter_op->op0, nullptr);
         VarNode* source2 = addVarNode(ter_op->op1, nullptr);
         VarNode* source3 = addVarNode(ter_op->op2, nullptr);

         // Create the operation using the intersect to constrain sink's interval.
         std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getGIMPLE_range(reinterpret_cast<const gimple_node*>(assign), AppM));
         TernaryOp* TOp = new TernaryOp(BI, sink, I, source1, source2, source3, ter_op->get_kind());

         // Insert the operation in the graph.
         this->oprs.insert(TOp);

         // Insert this definition in defmap
         this->defMap[sink->getValue()] = TOp;

         // Inserts the sources of the operation in the use map list.
         this->useMap.find(source1->getValue())->second.insert(TOp);
         this->useMap.find(source2->getValue())->second.insert(TOp);
         this->useMap.find(source3->getValue())->second.insert(TOp);
      }

      /// Add a phi node (actual phi, does not include sigmas)
      void addPhiOp(tree_nodeRef I, application_managerRef AppM)
      {
         const auto* phi = GetPointer<gimple_phi>(GET_NODE(I));
         THROW_ASSERT(phi, "");

         // Create the sink.
         VarNode* sink = addVarNode(I, nullptr);
         std::shared_ptr<BasicInterval> BI = std::make_shared<BasicInterval>(getGIMPLE_range(reinterpret_cast<const gimple_node*>(phi), AppM));
         PhiOp* phiOp = new PhiOp(BI, sink, I);

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
            " <--RA constraint graph: added PhiOp with range " << BI << " and " << phi->CGetDefEdgesList().size() << " sources");

         // Insert the operation in the graph.
         this->oprs.insert(phiOp);

         // Insert this definition in defmap
         this->defMap[sink->getValue()] = phiOp;

         // Create the sources.
         for(const auto& operand : phi->CGetDefEdgesList())
         {
            VarNode* source = addVarNode(operand.first, nullptr);
            phiOp->addSource(source);
            // Inserts the sources of the operation in the use map list.
            this->useMap.find(source->getValue())->second.insert(phiOp);
         }
      }

      void buildOperations(tree_nodeRef I_node, blocRef BB, const application_managerRef AppM)
      {
         auto* I = GetPointer<gimple_node>(GET_NODE(I_node));
         THROW_ASSERT(I, "");
         if(auto* assign = reinterpret_cast<gimple_assign*>(I))
         {
            if(GET_NODE(assign->op0)->get_kind() == mem_ref_K)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing store instruction");
               // TODO: handle store operation
            }
            else if(GET_NODE(assign->op1)->get_kind() == mem_ref_K)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing load instruction");
               // TODO: handle load operation
            }
            else if(auto* un_op = GetPointer<unary_expr>(GET_NODE(assign->op1)))
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing " << un_op->get_kind_text());
               return addUnaryOp(I_node, AppM);
            }
            else if(auto* bin_op = GetPointer<binary_expr>(GET_NODE(assign->op1)))
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing " << bin_op->get_kind_text());
               return addBinaryOp(I_node, AppM);
            }
            else if(auto* ter_op = GetPointer<ternary_expr>(GET_NODE(assign->op1)))
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing " << ter_op->get_kind_text());
               return addTernaryOp(I_node, AppM);
            }
            else if(auto* call_op = GetPointer<call_expr>(GET_NODE(assign->op1)))
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing " << call_op->get_kind_text());
               return addCallOp(I_node, AppM);
            }

            THROW_UNREACHABLE("Unhandled assign operation (" + GET_NODE(assign->op0)->get_kind_text() + " <- " + GET_NODE(assign->op1)->get_kind_text() + ")");
         }
         else if(auto* phi = reinterpret_cast<gimple_phi*>(I))
         {
            if(phi->CGetDefEdgesList().size() == 1)
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing sigma instruction");
               return addSigmaOp(I_node, BB, AppM);
            }
            else
            {
               PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                  " <--RA constraint graph: analyzing phi instruction");
               return addPhiOp(I_node, AppM);
            }
         }
         
         THROW_UNREACHABLE("Unhandled operation (" + I->get_kind_text() + ")");
      }

      /*
       *	This method builds a map that binds each variable label to the
       * operations
       * where this variable is used.
       */
      UseMap buildUseMap(const std::set<VarNode*>& component)
      {
         UseMap compUseMap;
         for(auto vit = component.begin(), vend = component.end(); vit != vend; ++vit)
         {
            const VarNode* var = *vit;
            const auto V = var->getValue();
            // Get the component's use list for V (it does not exist until we try to get it)
            auto& list = compUseMap[V];
            // Get the use list of the variable in component
            auto p = this->useMap.find(V);
            // For each operation in the list, verify if its sink is in the component
            for(BasicOp* opit : p->second)
            {
               VarNode* sink = opit->getSink();
               // If it is, add op to the component's use map
               if(component.count(sink) != 0)
               {
                  list.insert(opit);
               }
            }
         }
         return compUseMap;
      }

      /*
       * Used to insert constant in the right position
       */
      void insertConstantIntoVector(APInt constantval)
      {
         constantvector.push_back(constantval);
      }

      /*
       * Create a vector containing all constants related to the component
       * They include:
       *   - Constants inside component
       *   - Constants that are source of an edge to an entry point
       *   - Constants from intersections generated by sigmas
       */
      void buildConstantVector(const std::set<VarNode*>& component, const UseMap& compusemap)
      {
         // Remove all elements from the vector
         constantvector.clear();

         // Get constants inside component (TODO: may not be necessary, since
         // components with more than 1 node may
         // never have a constant inside them)
         for(VarNode* varNode : component)
         {
            const auto V = varNode->getValue();
            const integer_cst* ci = nullptr;

            if((ci = GetPointer<integer_cst>(GET_NODE(V.first))) != nullptr)
            {
               insertConstantIntoVector(ci->value);
            }
         }

         // Get constants that are sources of operations whose sink belong to the
         // component
         for(VarNode* varNode : component)
         {
            const auto V = varNode->getValue();
            auto dfit = defMap.find(V);
            if(dfit == defMap.end())
            {
               continue;
            }

            // Handle BinaryOp case
            const BinaryOp* bop = reinterpret_cast<BinaryOp*>(dfit->second);
            const PhiOp* pop = reinterpret_cast<PhiOp*>(dfit->second);
            if(bop != nullptr)
            {
               const VarNode* source1 = bop->getSource1();
               const auto sourceval1 = source1->getValue();
               const VarNode* source2 = bop->getSource2();
               const auto sourceval2 = source2->getValue();

               const integer_cst *const1, *const2;
               auto opcode = bop->getOpcode();

               if((const1 = GetPointer<integer_cst>(GET_NODE(sourceval1.first))) != nullptr)
               {
                  const auto& cnstVal = const1->value;
                  if(isCompare(opcode))
                  {
                     auto pred = opcode;
                     if(pred == eq_expr_K || pred == ne_expr_K)
                     {
                        insertConstantIntoVector(cnstVal);
                        insertConstantIntoVector(cnstVal - 1);
                        insertConstantIntoVector(cnstVal + 1);
                     }
                     else if(pred == gt_expr_K || pred == le_expr_K)
                     {
                        insertConstantIntoVector(cnstVal);
                        insertConstantIntoVector(cnstVal + 1);
                     }
                     else if(pred == ge_expr_K || pred == lt_expr_K)
                     {
                        insertConstantIntoVector(cnstVal);
                        insertConstantIntoVector(cnstVal - 1);
                     }
                     else if(pred == ungt_expr_K || pred == unle_expr_K)
                     {
                        auto bw = source1->getBitWidth();
                        auto cnstValU = ap_trunc(UAPInt(const1->value), bw);
                        insertConstantIntoVector(cnstValU);
                        insertConstantIntoVector(cnstValU + 1);
                     }
                     else if(pred == unge_expr_K || pred == unlt_expr_K)
                     {
                        auto bw = source1->getBitWidth();
                        auto cnstValU = ap_trunc(UAPInt(const1->value), bw);
                        insertConstantIntoVector(cnstValU);
                        insertConstantIntoVector(cnstValU - 1);
                     }
                     else
                     {
                        THROW_UNREACHABLE("unexpected condition (" + tree_node::GetString(opcode) + ")");
                     }
                  }
                  else
                  {
                     insertConstantIntoVector(cnstVal);
                  }
               }
               if((const2 = GetPointer<integer_cst>(GET_NODE(sourceval2.first))) != nullptr)
               {
                  const auto& cnstVal = const2->value;
                  if(isCompare(opcode))
                  {
                     auto pred = opcode;
                     if(pred == eq_expr_K || pred == ne_expr_K)
                     {
                        insertConstantIntoVector(cnstVal);
                        insertConstantIntoVector(cnstVal - 1);
                        insertConstantIntoVector(cnstVal + 1);
                     }
                     else if(pred == gt_expr_K || pred == le_expr_K)
                     {
                        insertConstantIntoVector(cnstVal);
                        insertConstantIntoVector(cnstVal + 1);
                     }
                     else if(pred == ge_expr_K || pred == lt_expr_K)
                     {
                        insertConstantIntoVector(cnstVal);
                        insertConstantIntoVector(cnstVal - 1);
                     }
                     else if(pred == ungt_expr_K || pred == unle_expr_K)
                     {
                        auto bw = source2->getBitWidth();
                        auto cnstValU = ap_trunc(UAPInt(const2->value), bw);
                        insertConstantIntoVector(cnstValU);
                        insertConstantIntoVector(cnstValU + 1);
                     }
                     else if(pred == unge_expr_K || pred == unlt_expr_K)
                     {
                        auto bw = source2->getBitWidth();
                        auto cnstValU = ap_trunc(UAPInt(const2->value), bw);
                        insertConstantIntoVector(cnstValU);
                        insertConstantIntoVector(cnstValU - 1);
                     }
                     else
                     {
                        THROW_UNREACHABLE("unexpected condition (" + tree_node::GetString(opcode) + ")");
                     }
                  }
                  else
                  {
                     insertConstantIntoVector(cnstVal);
                  }
               }
            }
            // Handle PhiOp case
            else if(pop != nullptr)
            {
               for(unsigned i = 0, e = pop->getNumSources(); i < e; ++i)
               {
                  const VarNode* source = pop->getSource(i);
                  const auto sourceval = source->getValue();
                  const integer_cst* consti;
                  if((consti = GetPointer<integer_cst>(GET_NODE(sourceval.first))) != nullptr)
                  {
                     insertConstantIntoVector(consti->value);
                  }
               }
            }
         }

         // Get constants used in intersections
         for(auto& pair : compusemap)
         {
            for(BasicOp* op : pair.second)
            {
               const SigmaOp* sigma = reinterpret_cast<SigmaOp*>(op);
               // Symbolic intervals are discarded, as they don't have fixed values yet
               if(sigma == nullptr || SymbInterval::classof(sigma->getIntersect().get()))
               {
                  continue;
               }
               Range rintersect = op->getIntersect()->getRange();
               if(rintersect.isAnti())
               {
                  auto anti = Range(Range_base::getAnti(rintersect));
                  const APInt lb = anti.getLower();
                  const APInt ub = anti.getUpper();
                  if((lb != Min) && (lb != Max))
                  {
                     insertConstantIntoVector(lb - 1);
                     insertConstantIntoVector(lb);
                  }
                  if((ub != Min) && (ub != Max))
                  {
                     insertConstantIntoVector(ub);
                     insertConstantIntoVector(ub + 1);
                  }
               }
               else
               {
                  const APInt& lb = rintersect.getLower();
                  const APInt& ub = rintersect.getUpper();
                  if((lb != Min) && (lb != Max))
                  {
                     insertConstantIntoVector(lb - 1);
                     insertConstantIntoVector(lb);
                  }
                  if((ub != Min) && (ub != Max))
                  {
                     insertConstantIntoVector(ub);
                     insertConstantIntoVector(ub + 1);
                  }
               }
            }
         }

         // Sort vector in ascending order and remove duplicates
         std::sort(constantvector.begin(), constantvector.end(), [](const APInt& i1, const APInt& i2) { return i1 < i2; });

         // std::unique doesn't remove duplicate elements, only
         // move them to the end
         // This is why erase is necessary. To remove these duplicates
         // that will be now at the end.
         auto last = std::unique(constantvector.begin(), constantvector.end());

         constantvector.erase(last, constantvector.end());
      }

      /*
       * This method builds a map of variables to the lists of operations where
       * these variables are used as futures. Its C++ type should be something like
       * map<VarNode, List<Operation>>.
       */
      void buildSymbolicIntersectMap()
      {
         // Creates the symbolic intervals map
         symbMap = SymbMap();

         // Iterate over the operations set
         for(BasicOp* op : oprs)
         {
            // If the operation is unary and its interval is symbolic
            auto* uop = reinterpret_cast<UnaryOp*>(op);
            if((uop != nullptr) && SymbInterval::classof(uop->getIntersect().get()))
            {
               auto symbi = std::dynamic_pointer_cast<SymbInterval>(uop->getIntersect());
               const auto V = symbi->getBound();
               auto p = symbMap.find(V);
               if(p != symbMap.end())
               {
                  p->second.insert(uop);
               }
               else
               {
                  std::set<BasicOp*> l;
                  l.insert(uop);
                  symbMap.insert(std::make_pair(V, l));
               }
            }
         }
      }

      /*
       * This method evaluates once each operation that uses a variable in
       * component, so that the next SCCs after component will have entry
       * points to kick start the range analysis algorithm.
       */
      void propagateToNextSCC(const std::set<VarNode*>& component)
      {
         for(auto var : component)
         {
            const auto V = var->getValue();
            auto p = this->useMap.find(V);
            for(BasicOp* op : p->second)
            {
               /// VarNodes belonging to the current SCC must not be evaluated otherwise we break the fixed point previously computed
               if(component.find(op->getSink()) != component.end())
               {
                  continue;
               }
               auto* sigmaop = reinterpret_cast<SigmaOp*>(op);
               op->getSink()->setRange(op->eval());
               if((sigmaop != nullptr) && sigmaop->getIntersect()->getRange().isUnknown())
               {
                  sigmaop->markUnresolved();
               }
            }
         }
      }

      void generateEntryPoints(const std::set<VarNode*>& component, std::set<eValue>& entryPoints)
      {
         // Iterate over the varnodes in the component
         for(VarNode* varNode : component)
         {
            const auto V = varNode->getValue();

            // TODO: check V.first is the gimple_phi tree_nodeRef when building SigmaOp calling addSigmaOp in buildOperations
            if(GetPointer<gimple_phi>(GET_NODE(V.first)) && GetPointer<gimple_phi>(GET_NODE(V.first))->CGetDefEdgesList().size() == 1)
            {
               auto dit = this->defMap.find(V);
               if(dit != this->defMap.end())
               {
                  BasicOp* bop = dit->second;
                  auto* defop = reinterpret_cast<SigmaOp*>(bop);

                  if((defop != nullptr) && defop->isUnresolved())
                  {
                     defop->getSink()->setRange(bop->eval());
                     defop->markResolved();
                  }
               }
            }
            if(!varNode->getRange().isUnknown())
            {
               entryPoints.insert(V);
            }
         }
      }

      void fixIntersects(const std::set<VarNode*>& component)
      {
         // Iterate again over the varnodes in the component
         for(VarNode* varNode : component)
         {
            fixIntersectsSC(varNode);
         }
      }

      void fixIntersectsSC(VarNode* varNode)
      {
         const auto V = varNode->getValue();
         auto sit = symbMap.find(V);
         if(sit != symbMap.end())
         {
            #ifdef DEBUG_CGRAPH
            PRINT_MSG("fix intesects:" << std::endl << varNode);
            #endif
            for(BasicOp* op : sit->second)
            {
               #ifdef DEBUG_CGRAPH
               PRINT_MSG("op intersects:" << std::endl << op);
               #endif
               op->fixIntersects(varNode);
               #ifdef DEBUG_CGRAPH
               PRINT_MSG("sink:" << op);
               #endif
            }
         }
      }

      void generateActivesVars(const std::set<VarNode*>& component, std::set<eValue>& activeVars)
      {
         for(VarNode* varNode : component)
         {
            const auto V = varNode->getValue();
            const auto* CI = GetPointer<integer_cst>(GET_NODE(V.first));
            if(CI != nullptr)
            {
               continue;
            }
            activeVars.insert(V);
         }
      }

   public:
      ConstraintGraph(int _debug_level) : debug_level(_debug_level) {}

      virtual ~ConstraintGraph() = default;

      /// Adds a VarNode in the graph.
      VarNode* addVarNode(tree_nodeRef V, tree_nodeRef GV)
      {
         eValue ev(V, GV);
         auto vit = vars.find(ev);
         if(vit != vars.end())
         {
            return vit->second;
         }

         auto* node = new VarNode(V, GV);
         vars.insert(std::make_pair(ev, node));

         // Inserts the node in the use map list.
         std::set<BasicOp*> useList;
         useMap.insert(std::make_pair(ev, useList));
         return node;
      }

      GenOprs* getOprs()
      {
         return &oprs;
      }
      DefMap* getDefMap()
      {
         return &defMap;
      }
      UseMap* getUseMap()
      {
         return &useMap;
      }

      /// Iterates through all instructions in the function and builds the graph.
      void buildGraph(unsigned int function_id, const application_managerRef AppM)
      {
         const auto TM = AppM->get_tree_manager();
         auto* FD = GetPointer<function_decl>(TM->get_tree_node_const(function_id));
         auto* SL = GetPointer<statement_list>(GET_NODE(FD->body));

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
            " <--RA constraint graph: analysing function " << GetPointer<identifier_node>(GET_NODE(FD->name))->strg << " with " << SL->list_of_bloc.size() << " blocks");

         buildValueMaps(SL->list_of_bloc, AppM);

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " <--RA constraint graph: value map built");

         for(const auto& [bb_id, bb] : SL->list_of_bloc)
         {
            const auto& stmt_list = bb->CGetStmtList();
            if(stmt_list.size())
            {
               for(const auto& stmt : stmt_list)
               {
                  const auto _I = GET_NODE(stmt);

                  THROW_ASSERT(_I, "Instruction not valid");

                  // TODO: check that instruction deals with integers only

                  if(!isValidInstruction(stmt))
                  {
                     PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
                        " <--RA constraint graph: skipping " << _I->get_kind_text() << "...");
                     continue;
                  }
                  
                  buildOperations(stmt, bb, AppM);
               }
            }
         }
   
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " <--RA: graph built for function " << FD->name->ToString());
      }

      void buildVarNodes()
      {
         // Initializes the nodes and the use map structure.
         for(auto& pair : vars)
         {
            pair.second->init(this->defMap.count(pair.first) == 0u);
         }
      }

      void findIntervals()
      {
         buildSymbolicIntersectMap();

         // List of SCCs
         Nuutila sccList(&vars, &useMap, &symbMap);
         
         for(auto nit = sccList.begin(), nend = sccList.end(); nit != nend; ++nit)
         {
            auto& component = *sccList.components[*nit];

            #ifdef DEBUG_RA
            PRINT_MSG("Components:");
            for(auto var : component)
            {
               PRINT_MSG("  " << var);
            }
            PRINT_MSG("-----------");
            #endif
            if(component.size() == 1)
            {
               VarNode* var = *component.begin();
               fixIntersectsSC(var);
               if(this->defMap.find(var->getValue()) != this->defMap.end())
               {
                  BasicOp* op = this->defMap.find(var->getValue())->second;
                  var->setRange(op->eval());
               }
               if(var->getRange().isUnknown())
               {
                  var->setRange(Range(Regular, var->getBitWidth()));
               }
            }
            else
            {
               UseMap compUseMap = buildUseMap(component);

               // Get the entry points of the SCC
               std::set<eValue> entryPoints;

                #ifdef RA_JUMPSET
               // Create vector of constants inside component
               // Comment this line below to deactivate jump-set
               buildConstantVector(component, compUseMap);
               #endif
               #ifdef DEBUG_RA
               for(auto cnst : constantvector)
               {
                  PRINT_MSG(" " << cnst);
               }
               if(!constantvector.empty())
               {
                  PRINT_MSG("");
               }
               #endif
               generateEntryPoints(component, entryPoints);
               // iterate a fixed number of time before widening
               update(component.size() * 16, compUseMap, entryPoints);
               #ifdef DEBUG_RA
               //if(func != nullptr)
               //{
               //   printToFile(*func, "/tmp/" + func->getName().str() + "cgfixed.dot");
               //}
               #endif

               generateEntryPoints(component, entryPoints);
               #ifdef DEBUG_RA
               PRINT_MSG("entryPoints:");
               for(auto el : entryPoints)
               {
                  PRINT_MSG(el.first->ToString());
               }
               #endif
               // First iterate till fix point
               preUpdate(compUseMap, entryPoints);
               #ifdef DEBUG_RA
               PRINT_MSG("fixIntersects");
               #endif
               fixIntersects(component);
               #ifdef DEBUG_RA
               PRINT_MSG("--");
               #endif

               #ifdef DEBUG_RA
               //if(func != nullptr)
               //{
               //   printToFile(*func, "/tmp/" + func->getName().str() + "cgfixintersect.dot");
               //}
               #endif

               for(VarNode* varNode : component)
               {
                  if(varNode->getRange().isUnknown())
                  {
                     #ifdef DEBUG_RA
                     PRINT_MSG("initialize unknown: " << varNode->getValue().first->ToString());
                     #endif
                     // THROW_UNREACHABLE("unexpected condition");
                     varNode->setRange(Range(Regular, varNode->getBitWidth(), Min, Max));
                  }
               }

               #ifdef DEBUG_RA
               //if(func != nullptr)
               //{
               //   printToFile(*func, "/tmp/" + func->getName().str() + "cgint.dot");
               //}
               #endif

               // Second iterate till fix point
               std::set<eValue> activeVars;
               generateActivesVars(component, activeVars);
               posUpdate(compUseMap, activeVars, &component);
            }
            propagateToNextSCC(component);
         }
      }

      Range getRange(eValue v)
      {
         auto vit = this->vars.find(v);
         if(vit == this->vars.end())
         {
            // If the value doesn't have a range,
            // it wasn't considered by the range analysis
            // for some reason.
            // It gets an unknown range if it's a variable,
            // or the tight range if it's a constant
            //
            // I decided NOT to insert these uncovered
            // values to the node set after their range
            // is created here.
            auto bw = LoadStoreOperationBW(v.first, v.second);
            THROW_ASSERT(bw, "Invalid bitwidth");
            const auto* ci = GetPointer<integer_cst>(GET_NODE(v.first));
            if(ci == nullptr)
            {
               return Range(Unknown, bw);
            }
            APInt tmp = ci->value;
            return Range(Regular, bw, tmp, tmp);
         }
         return vit->second->getRange();
      }
   };

   // ========================================================================== //
   // Cousot
   // ========================================================================== //
   class Cousot : public ConstraintGraph
   {
    private:
      void preUpdate(const UseMap& compUseMap, std::set<eValue>& entryPoints) override
      {
         update(compUseMap, entryPoints, Meet::widen);
      }

      void posUpdate(const UseMap& compUseMap, std::set<eValue>& entryPoints, const std::set<VarNode*>* component) override
      {
         update(compUseMap, entryPoints, Meet::narrow);
      }

    public:
      Cousot(int _debug_level) : ConstraintGraph(_debug_level) {}
   };

   // ========================================================================== //
   // CropDFS
   // ========================================================================== //
   class CropDFS : public ConstraintGraph
   {
    private:
      void preUpdate(const UseMap& compUseMap, std::set<eValue>& entryPoints) override
      {
         update(compUseMap, entryPoints, Meet::growth);
      }

      void posUpdate(const UseMap& compUseMap, std::set<eValue>& activeVars, const std::set<VarNode*>* component) override
      {
         storeAbstractStates(*component);
         auto obgn = oprs.begin(), oend = oprs.end();
         for(; obgn != oend; ++obgn)
         {
            BasicOp* op = *obgn;
            if(component->count(op->getSink()) != 0u)
            {
               crop(compUseMap, op);
            }
         }
      }

      void storeAbstractStates(const std::set<VarNode*>& component)
      {
         for(auto varNode : component)
         {
            varNode->storeAbstractState();
         }
      }

      void crop(const UseMap& compUseMap, BasicOp* op)
      {
         std::set<BasicOp*> activeOps;
         std::set<const VarNode*> visitedOps;

         // init the activeOps only with the op received
         activeOps.insert(op);

         while(!activeOps.empty())
         {
            BasicOp* V = *activeOps.begin();
            activeOps.erase(V);
            const VarNode* sink = V->getSink();

            // if the sink has been visited go to the next activeOps
            if(visitedOps.count(sink) != 0u)
            {
               continue;
            }

            Meet::crop(V, nullptr);
            visitedOps.insert(sink);

            // The use list.of sink
            const auto& L = compUseMap.find(sink->getValue())->second;
            for(BasicOp* op : L)
            {
               activeOps.insert(op);
            }
         }
      }

    public:
      CropDFS(int _debug_level) : ConstraintGraph(_debug_level) {}
   };

   static void MatchParametersAndReturnValues(unsigned int function_id, application_managerRef AppM, RangeAnalysis::ConstraintGraph* CG, int debug_level)
   {
      const auto TM = AppM->get_tree_manager();
      auto* FD = GetPointer<function_decl>(TM->GetTreeNode(function_id));
      auto* FN = GetPointer<identifier_node>(GET_NODE(FD->name));
      auto* FT = GetPointer<function_type>(GET_NODE(FD->type));
      auto* SL = GetPointer<statement_list>(GET_NODE(FD->body));
      
      // Only do the matching if the function has any use
      const auto& RBD = AppM->CGetCallGraphManager()->GetReachedBodyFunctions();
      if(RBD.find(function_id) == RBD.end())
      {
         return;
      }

      // Data structure which contains the matches between formal and real
      // parameters
      // First: formal parameter
      // Second: real parameter
      std::vector<std::pair<tree_nodeRef, tree_nodeRef>> parameters(FD->list_of_args.size());

      // Fetch the function arguments (formal parameters) into the data structure
      unsigned int i = 0;
      for(auto arg_it = FD->list_of_args.begin(), arg_end = FD->list_of_args.end(); arg_it != arg_end; ++arg_it, ++i)
      {
         auto* arg = GetPointer<parm_decl>(GET_NODE(*arg_it));
         parameters[i].first = *arg_it;
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <--RA return values match: function " << FN->strg << " has " << i << " arguments");

      // Check if the function returns a supported value type. If not, no return
      // value matching is done
      bool noReturn = GET_NODE(FT->retn)->get_kind() == void_type_K;

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <--RA return values match: function " << FN->strg << " has " << (noReturn ? "no return" : "return of type ") << (noReturn ? "" : GET_NODE(FT->retn)->get_kind_text()));

      // Creates the data structure which receives the return values of the
      // function, if there is any
      std::set<tree_nodeRef> returnValues;

      if(!noReturn)
      {
         for(const auto& [bb_id, bb] : SL->list_of_bloc)
         {
            const auto& stmt_list = bb->CGetStmtList();
            
            if(stmt_list.size())
            if(auto* RI = GetPointer<gimple_return>(GET_NODE(stmt_list.back())))
            {
               returnValues.insert(RI->op);
            }
         }
      }
      if(returnValues.empty())
      {
         noReturn = true;
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " <--RA return values match: function " 
         << FN->strg << (noReturn ? " has no" : " has explicit") << " return statements");

      std::vector<PhiOp*> matchers(parameters.size(), nullptr);

      for(size_t i = 0, e = parameters.size(); i < e; ++i)
      {
         VarNode* sink = CG->addVarNode(parameters[i].first, nullptr);
         sink->setRange(Range(Regular, sink->getBitWidth(), Min, Max));
         matchers[i] = new PhiOp(std::make_shared<BasicInterval>(), sink, nullptr);
         // Insert the operation in the graph.
         CG->getOprs()->insert(matchers[i]);
         // Insert this definition in defmap
         (*CG->getDefMap())[sink->getValue()] = matchers[i];
      }

      std::vector<VarNode*> returnVars;

      for(auto returnValue : returnValues)
      {
         VarNode* from = CG->addVarNode(returnValue, nullptr);
         returnVars.push_back(from);
      }

      // TODO: for each use of the function match formal and real parameters for caller instruction
   }

   // ========================================================================== //
   // RangeAnalysis
   // ========================================================================== //
   Range_Analysis::Range_Analysis(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par)
      : ApplicationFrontendFlowStep(AM, RANGE_ANALYSIS, dfm, par)
   {
      debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
   }

   Range_Analysis::~Range_Analysis() = default;

   const std::unordered_set<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> 
   Range_Analysis::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
   {
      std::unordered_set<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
      switch(relationship_type)
      {
         case DEPENDENCE_RELATIONSHIP:
         {
            relationships.insert(std::make_pair(ESSA, ALL_FUNCTIONS));
            break;
         }
         case PRECEDENCE_RELATIONSHIP:
         {
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

   bool Range_Analysis::HasToBeExecuted() const
   {
      return true;
   }

   DesignFlowStep_Status Range_Analysis::Exec()
   {
      /// Build Constraint graph on every function
      RangeAnalysis::ConstraintGraph* CG = new Cousot(debug_level);

      MAX_BIT_INT = getMaxBitWidth();
      updateConstantIntegers(MAX_BIT_INT);

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, 
         " <--RA: maximum bitwidth is " << MAX_BIT_INT << " bits");

      const auto functions = AppM->get_functions_with_body();
      for(unsigned int f : functions)
      {
         CG->buildGraph(f, AppM);
         MatchParametersAndReturnValues(f, AppM, CG, debug_level);
      }
      CG->buildVarNodes();

      CG->findIntervals();

      finalizeRangeAnalysis(CG);

      delete CG;
      
      return DesignFlowStep_Status::UNCHANGED;
   }

   void Range_Analysis::Initialize()
   {
   }

   unsigned Range_Analysis::getMaxBitWidth(unsigned int F)
   {
      unsigned int InstBitSize = 0, opBitSize = 0, max = 0;

      const auto TM = AppM->get_tree_manager();
      auto* FD = GetPointer<function_decl>(TM->get_tree_node_const(F));
      auto* SL = GetPointer<statement_list>(GET_NODE(FD->body));

      for(const auto& [bb_id, bb] : SL->list_of_bloc)
      {
         const auto& stmt_list = bb->CGetStmtList();
         if(stmt_list.size())
         {
            for(const auto& stmt : stmt_list)
            {
               auto* I = GetPointer<gimple_node>(GET_NODE(stmt));

               THROW_ASSERT(I != nullptr, "Instruction should be valid");

               InstBitSize = getGIMPLE_BW(I);
               if(InstBitSize > max)   // TODO: check that instruction is about integer types
               {
                  max = InstBitSize;
               }
               
               // TODO: check instruction operands dimension and update max if necessary
            }
         }
      }
      // Bit-width equal to 0 is not valid, so we increment to 1
      if(max == 0)
      {
         ++max;
      }
      return max;
   }

   unsigned Range_Analysis::getMaxBitWidth()
   {
      unsigned max = 0;

      const auto functions = AppM->get_functions_with_body();
      for(unsigned int f : functions)
      {
         unsigned bitwidth = getMaxBitWidth(f);
         if(bitwidth > max)
         {
            max = bitwidth;
         }
      }

      return max + 1;
   }

   void Range_Analysis::updateConstantIntegers(unsigned maxBitWidth)
   {
      // Updates the Min and Max values.
      Min = getSignedMinValue(maxBitWidth);
      Max = getSignedMaxValue(maxBitWidth);
   }

   void Range_Analysis::finalizeRangeAnalysis(void* CGp)
   {
      ConstraintGraph* CG = reinterpret_cast<ConstraintGraph*>(CGp);
      
      const auto TM = AppM->get_tree_manager();
      
      const auto functions = AppM->get_functions_with_body();
      for(unsigned int f : functions)
      {
         auto* FD = GetPointer<function_decl>(TM->get_tree_node_const(f));
         auto* SL = GetPointer<statement_list>(GET_NODE(FD->body));
      
         for(const auto& [bb_id, bb] : SL->list_of_bloc)
         {
            const auto& stmt_list = bb->CGetStmtList();
            if(stmt_list.size())
            {
               for(auto& stmt : stmt_list)
               {
                  auto* I = GetPointer<gimple_node>(GET_NODE(stmt));
      
                  // TODO: check instruction is not void type
      
                  // TODO: maybe adapt following operation to consider the variable associated
                  //       with the statement and not the statement itself
                  ranges.insert(std::make_pair(I, CG->getRange(std::make_pair(stmt, nullptr))));
               }
            }
         }
      }
   }
}