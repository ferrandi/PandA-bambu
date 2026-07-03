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
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "IR_lowering.hpp"

#include "Discrepancy.hpp"
#include "Parameter.hpp"
#include "application_manager.hpp"
#include "area_info.hpp"
#include "constdiv_magic.hpp"
#include "custom_map.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"
#include "frontend_flow_step.hpp"
#include "function_behavior.hpp"
#include "function_frontend_flow_step.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_common.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "module_allocation/area_estimation.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "time_info.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#define FMT_HEADER_ONLY 1
#define PHMAP_BIDIRECTIONAL 0
#define MCDBGQ_NOLOCKFREE_FREELIST 0
#define MCDBGQ_TRACKMEM 0
#define MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX 0
#define MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH 0
#define MCDBGQ_USEDEBUGFREELIST 0
#define DISABLE_NAUTY

#include <kitty/print.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>
#include <mockturtle/algorithms/node_resynthesis/shannon.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/mapping_view.hpp>
#include <mockturtle/views/topo_view.hpp>
#pragma GCC diagnostic pop

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"

std::string IR_lowering::buildLocInfoDefault(const ir_nodeRef& stmt) const
{
   const auto gn = GetPointer<node_stmt>(stmt);
   if(gn)
   {
      return gn->include_name + ":" + STR(gn->line_number) + ":" + STR(gn->column_number);
   }
   return "";
}

ir_nodeRef IR_lowering::createAndInsertAssign(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos,
                                              const ir_nodeRef& type, const ir_nodeRef& min, const ir_nodeRef& max,
                                              const ir_nodeRef& expr, const std::string& locInfoDefault,
                                              bool setTempAddress, bool& restartAnalysis)
{
   const auto newGa = ir_man->CreateAssignStmt(type, min, max, expr, function_id, locInfoDefault);
   if(setTempAddress)
   {
      GetPointer<assign_stmt>(newGa)->temporary_address = true;
   }
   block->PushBefore(newGa, *itLos, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + newGa->ToString());
   restartAnalysis = true;
   return GetPointer<assign_stmt>(newGa)->op0;
}

void IR_lowering::extractExpr(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, ir_nodeRef& op,
                              bool setTempAddress, const std::string& locInfoDefault, bool& restartAnalysis)
{
   ir_nodeRef min;
   ir_nodeRef max;
   if(op->get_kind() == nop_node_K)
   {
      auto nop = GetPointer<nop_node>(op);
      if(nop->op->get_kind() == ssa_node_K)
      {
         auto opssa = GetPointerS<ssa_node>(nop->op);
         if(opssa->min && opssa->max)
         {
            const auto isOpSigned = ir_helper::IsSignedIntegerType(nop->op);
            const auto isResSigned = ir_helper::IsSignedIntegerType(nop->type);
            if(isOpSigned == isResSigned)
            {
               min = opssa->min;
               max = opssa->max;
            }
            else
            {
               const auto sizeOp = ir_helper::Size(nop->op);
               if(isResSigned)
               {
                  min = TM->CreateUniqueIntegerCst(-(integer_cst_t(1ll) << (sizeOp - 1)), nop->type);
                  max = TM->CreateUniqueIntegerCst((integer_cst_t(1ll) << (sizeOp - 1)) - 1, nop->type);
               }
               else
               {
                  min = TM->CreateUniqueIntegerCst(0, nop->type);
                  max = TM->CreateUniqueIntegerCst((integer_cst_t(1ll) << sizeOp) - 1, nop->type);
               }
            }
         }
      }
   }
   const auto exprType =
       GetPointer<expr_node>(op) ? GetPointerS<expr_node>(op)->type : GetPointer<constructor_node>(op)->type;
   op = createAndInsertAssign(block, itLos, exprType, min, max, op, locInfoDefault, setTempAddress, restartAnalysis);
}

void IR_lowering::extractUnaryExpr(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos,
                                   ir_nodeRef& op, bool duplicate, bool setTempAddress,
                                   const std::string& locInfoDefault, bool& restartAnalysis)
{
   if(duplicate)
   {
      auto* ue = GetPointer<unary_node>(op);
      op = ir_man->create_unary_operation(ue->type, ue->op, locInfoDefault, ue->get_kind());
   }
   extractExpr(block, itLos, op, setTempAddress, locInfoDefault, restartAnalysis);
}

void IR_lowering::typeCastExpr(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos, ir_nodeRef& op,
                               const ir_nodeRef& type, const std::string& locInfoDefault, bool& restartAnalysis)
{
   static_cast<void>(locInfoDefault);
   const auto nop = ir_man->CreateNopExpr(op, type, ir_nodeConstRef(), ir_nodeConstRef(), function_id);
   op = GetPointerS<const assign_stmt>(nop)->op0;
   block->PushBefore(nop, *itLos, AppM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + nop->ToString());
   restartAnalysis = true;
}

void IR_lowering::normalizeOperands(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos,
                                    assign_stmt* ga, enum kind code0, enum kind code1,
                                    const std::string& locInfoDefault, bool& restartAnalysis)
{
   if(GetPointer<unary_node>(ga->op1) && ga->op1->get_kind() != addr_node_K) /// required by the CLANG/LLVM plugin
   {
      auto ue = GetPointer<unary_node>(ga->op1);
      if(GetPointer<unary_node>(ue->op))
      {
         extractUnaryExpr(block, itLos, ue->op, ue->op->get_kind() == addr_node_K || ue->op->get_kind() == nop_node_K,
                          ga->temporary_address || code1 == mem_access_node_K, locInfoDefault, restartAnalysis);
      }
      if(GetPointer<binary_node>(ue->op) || GetPointer<ternary_node>(ue->op))
      {
         extractExpr(block, itLos, ue->op, ga->temporary_address || code1 == mem_access_node_K, locInfoDefault,
                     restartAnalysis);
      }
   }
   if(GetPointer<binary_node>(ga->op1)) /// required by the CLANG/LLVM plugin
   {
      auto be = GetPointer<binary_node>(ga->op1);
      if(GetPointer<unary_node>(be->op0))
      {
         extractUnaryExpr(block, itLos, be->op0,
                          be->op0->get_kind() == addr_node_K || be->op0->get_kind() == nop_node_K,
                          ga->temporary_address || code1 == mem_access_node_K, locInfoDefault, restartAnalysis);
      }
      if(GetPointer<unary_node>(be->op1))
      {
         extractUnaryExpr(block, itLos, be->op1,
                          be->op0->get_kind() == addr_node_K || be->op0->get_kind() == nop_node_K, false,
                          locInfoDefault, restartAnalysis);
      }
      if(GetPointer<binary_node>(be->op0) || GetPointer<constructor_node>(be->op0) || GetPointer<ternary_node>(be->op0))
      {
         extractExpr(block, itLos, be->op0, ga->temporary_address || code1 == mem_access_node_K, locInfoDefault,
                     restartAnalysis);
      }
      if(GetPointer<binary_node>(be->op1) || GetPointer<constructor_node>(be->op1) || GetPointer<ternary_node>(be->op1))
      {
         extractExpr(block, itLos, be->op1, false, locInfoDefault, restartAnalysis);
      }
      const auto be_kind = be->get_kind();
      if(be_kind == and_node_K || be_kind == or_node_K || be_kind == xor_node_K || be_kind == add_node_K ||
         be_kind == sub_node_K || be_kind == mul_node_K || be_kind == idiv_node_K || be_kind == irem_node_K ||
         be_kind == add_sat_node_K || be_kind == sub_sat_node_K)
      {
         if(!ir_helper::IsSameType(be->op0, be->type))
         {
            typeCastExpr(block, itLos, be->op0, be->type, locInfoDefault, restartAnalysis);
         }
         if(!ir_helper::IsSameType(be->op1, be->type))
         {
            typeCastExpr(block, itLos, be->op1, be->type, locInfoDefault, restartAnalysis);
         }
      }
   }
   if(GetPointer<ternary_node>(ga->op1)) /// required by the CLANG/LLVM plugin
   {
      auto te = GetPointer<ternary_node>(ga->op1);
      if(GetPointer<unary_node>(te->op0) || GetPointer<binary_node>(te->op0) || GetPointer<constructor_node>(te->op0) ||
         GetPointer<ternary_node>(te->op0))
      {
         extractExpr(block, itLos, te->op0, false, locInfoDefault, restartAnalysis);
      }
      if(GetPointer<unary_node>(te->op1) || GetPointer<binary_node>(te->op1) || GetPointer<constructor_node>(te->op1) ||
         GetPointer<ternary_node>(te->op1))
      {
         extractExpr(block, itLos, te->op1, false, locInfoDefault, restartAnalysis);
      }
      if(te->op2 && (GetPointer<unary_node>(te->op2) || GetPointer<binary_node>(te->op2) ||
                     GetPointer<constructor_node>(te->op2) || GetPointer<ternary_node>(te->op2)))
      {
         extractExpr(block, itLos, te->op2, false, locInfoDefault, restartAnalysis);
      }
   }

   if(GetPointer<unary_node>(ga->op0)) /// required by the CLANG/LLVM plugin
   {
      auto ue = GetPointer<unary_node>(ga->op0);
      if(GetPointer<unary_node>(ue->op))
      {
         extractUnaryExpr(block, itLos, ue->op, ue->op->get_kind() == addr_node_K, code0 == mem_access_node_K,
                          locInfoDefault, restartAnalysis);
      }
      if(GetPointer<binary_node>(ue->op) || GetPointer<ternary_node>(ue->op))
      {
         extractExpr(block, itLos, ue->op, code0 == mem_access_node_K, locInfoDefault, restartAnalysis);
      }
   }
   if(GetPointer<binary_node>(ga->op0)) /// required by the CLANG/LLVM plugin
   {
      auto be = GetPointer<binary_node>(ga->op0);
      if(GetPointer<unary_node>(be->op0))
      {
         extractUnaryExpr(block, itLos, be->op0, be->op0->get_kind() == addr_node_K, code0 == mem_access_node_K,
                          locInfoDefault, restartAnalysis);
      }
      if(GetPointer<binary_node>(be->op0) || GetPointer<ternary_node>(be->op0))
      {
         extractExpr(block, itLos, be->op0, code0 == mem_access_node_K, locInfoDefault, restartAnalysis);
      }
      if(GetPointer<unary_node>(be->op1) || GetPointer<binary_node>(be->op1) || GetPointer<ternary_node>(be->op1))
      {
         extractExpr(block, itLos, be->op1, false, locInfoDefault, restartAnalysis);
      }
   }
}

bool IR_lowering::handleMemRef(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos,
                               assign_stmt* ga, enum kind code0, enum kind code1, const std::string& locInfoDefault,
                               bool& restartAnalysis)
{
   if(code1 == addr_node_K)
   {
      auto* ae = GetPointer<addr_node>(ga->op1);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Op of addr expr is " + ae->op->get_kind_text());
      enum kind ae_code = ae->op->get_kind();
      if(ae_code == mem_access_node_K)
      {
         /// normalize op0
         auto* MR = GetPointer<mem_access_node>(ae->op);
         const auto mem_op0_kind = MR->op->get_kind();
         if(mem_op0_kind == addr_node_K || mem_op0_kind == gep_node_K || mem_op0_kind == bitcast_node_K)
         {
            // BEAWARE: it is ok to use this function even with binary_node provided that duplicate = false
            // in those cases
            extractUnaryExpr(block, itLos, MR->op, mem_op0_kind == addr_node_K, ga->temporary_address, locInfoDefault,
                             restartAnalysis);
         }

         if(mem_op0_kind == constant_int_val_node_K)
         {
            ga->op1 = MR->op;
         }
         else if(ga->temporary_address)
         {
            ga->op1 = MR->op;
         }
         else
         {
            if(ir_helper::CGetType(MR->op)->index != ae->type->index)
            {
               const auto ga_nop = ir_man->CreateNopExpr(MR->op, ae->type, ir_nodeRef(), ir_nodeRef(), function_id);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ga_nop->ToString());
               const auto nop_vd = GetPointer<assign_stmt>(ga_nop)->op0;
               block->PushBefore(ga_nop, *itLos, AppM);
               ga->op1 = nop_vd;
            }
            else
            {
               ga->op1 = MR->op;
            }
         }
         restartAnalysis = true;
      }
      else if(ae_code == variable_val_node_K || ae_code == argument_val_node_K || ae_code == function_val_node_K)
      {
         if(code0 == ssa_node_K && ae_code != function_val_node_K && ae_code != argument_val_node_K)
         {
            auto ssa_var = GetPointerS<ssa_node>(ga->op0);
            if(ssa_var->use_set.variables.empty())
            {
               ssa_var->use_set.Add(ae->op);
            }
         }
      }
      else
      {
         THROW_ERROR("not supported " + ae->op->get_kind_text());
      }

      /// check missing cast
#if 1
      auto* pt_ae = GetPointerS<pointer_ty_node>(ae->type);
      auto ptd_index = pt_ae->ptd->index;
      const auto op_type_node = ir_helper::CGetType(ae->op);
      const auto op_type_id = op_type_node->index;
      if(op_type_id != ptd_index)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix missing cast");
         const auto ae_new_node = ir_man->CreateAddrExpr(ae->op, locInfoDefault);
         auto* ae_new = GetPointer<addr_node>(ae_new_node);
         const auto a_ga = ir_man->CreateAssignStmt(ae_new->type, ir_nodeRef(), ir_nodeRef(), ae_new_node, function_id,
                                                    locInfoDefault);
         GetPointer<assign_stmt>(a_ga)->temporary_address = ga->temporary_address;
         block->PushBefore(a_ga, *itLos, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + a_ga->ToString());
         const auto nop_ae =
             ir_man->create_unary_operation(ae->type, GetPointer<assign_stmt>(a_ga)->op0, locInfoDefault, nop_node_K);
         ga->op1 = nop_ae;
         ga->temporary_address = true;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed missing cast");
         restartAnalysis = true;
      }
#endif
      return true;
   }
   if(code1 == mem_access_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---expand mem_access_node 1 " + STR(ga->op1->index));
      auto* MR = GetPointer<mem_access_node>(ga->op1);
      const auto mem_op0_kind = MR->op->get_kind();
      if(mem_op0_kind == addr_node_K || mem_op0_kind == gep_node_K || mem_op0_kind == bitcast_node_K)
      {
         // BEAWARE: it is ok to use this function even with binary_node provided that duplicate = false
         // in those cases
         extractUnaryExpr(block, itLos, MR->op, mem_op0_kind == addr_node_K, true, locInfoDefault, restartAnalysis);
      }
      return true;
   }
   return false;
}

bool IR_lowering::lowerArithExpr(const std::pair<unsigned int, blocRef>& block,
                                 std::list<ir_nodeRef>::const_iterator& itLos, assign_stmt* ga, enum kind code1,
                                 const std::string& locInfoDefault, bool& restartAnalysis)
{
   if(code1 == idiv_node_K || code1 == irem_node_K)
   {
      auto be = GetPointer<binary_node>(ga->op1);
      ir_nodeRef op1 = be->op1;
      if(GetPointer<cst_node>(op1))
      {
         decomposeDivisionByConstant(block, itLos, ga, op1, code1, restartAnalysis, locInfoDefault, GetName());
      }
      return true;
   }
   if(code1 == mul_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Expanding mul_node  " + STR(ga->op1->index));
      ir_nodeRef op1 = GetPointer<binary_node>(ga->op1)->op1;
      if(GetPointer<constant_int_val_node>(op1))
      {
         auto* cn = GetPointer<constant_int_val_node>(op1);
         ir_nodeRef op0 = GetPointer<binary_node>(ga->op1)->op0;
         ir_nodeRef type_expr = GetPointer<binary_node>(ga->op1)->type;

         if(cn)
         {
            auto prev_index = ga->op1->index;
            ga->op1 =
                decomposeMultiplicationByConstant(op0, cn, ga->op1, *itLos, block.second, type_expr, locInfoDefault);
            restartAnalysis = restartAnalysis || (prev_index != ga->op1->index);
            if(prev_index != ga->op1->index)
            {
               AppM->RegisterTransformation(GetName(), *itLos);
            }
         }
      }
      /// if the previous transformation still give a multiplication let's check if this
      /// multiplication is actually widen_mul_node
      if(not reached_max_transformation_limit(*itLos) and ga->op1->get_kind() == mul_node_K)
      {
         ir_nodeRef type_expr = GetPointer<binary_node>(ga->op1)->type;

         bool realp = ir_helper::IsRealType(type_expr);
         if(!realp)
         {
            /// check if a mul_node may become a widen_mul_node
            auto dw_out = ir_helper::Size(ga->op0);
            auto data_bitsize_out = ceil_pow2(dw_out);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---data_bitsize_out " + STR(data_bitsize_out) + " <- " + STR(dw_out) + "\n");
            auto dw_in0 = ir_helper::Size(GetPointer<binary_node>(ga->op1)->op0);
            auto data_bitsize_in0 = ceil_pow2(dw_in0);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---data_bitsize_in0 " + STR(data_bitsize_in0) + " <- " + STR(dw_in0) + "\n");
            auto dw_in1 = ir_helper::Size(GetPointer<binary_node>(ga->op1)->op1);
            auto data_bitsize_in1 = ceil_pow2(dw_in1);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---data_bitsize_in1 " + STR(data_bitsize_in1) + " <- " + STR(dw_in1) + "\n");
            if(std::max(data_bitsize_in0, data_bitsize_in1) * 2 == data_bitsize_out)
            {
               auto op0_type = ir_helper::CGetType(ga->op0);
               ga->op1 = ir_man->create_binary_operation(op0_type, GetPointer<binary_node>(ga->op1)->op0,
                                                         GetPointer<binary_node>(ga->op1)->op1, locInfoDefault,
                                                         widen_mul_node_K);
               restartAnalysis = true;
               AppM->RegisterTransformation(GetName(), *itLos);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded");
      return true;
   }
   if(code1 == widen_mul_node_K)
   {
      const auto be = GetPointer<binary_node>(ga->op1);
      ir_nodeRef op1 = be->op1;
      if(GetPointer<cst_node>(op1) && !GetPointer<constant_vector_val_node>(op1))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Expanding widen_mul_node  " + STR(ga->op1->index));
         auto* cn = GetPointer<cst_node>(op1);
         ir_nodeRef op0 = GetPointer<binary_node>(ga->op1)->op0;
         ir_nodeRef type_expr = GetPointer<binary_node>(ga->op1)->type;

         bool realp = ir_helper::IsRealType(type_expr);
         if(!realp)
         {
            auto prev_index = ga->op1->index;
            ga->op1 = decomposeMultiplicationByConstant(op0, static_cast<constant_int_val_node*>(cn), ga->op1, *itLos,
                                                        block.second, type_expr, locInfoDefault);
            restartAnalysis = restartAnalysis || (prev_index != ga->op1->index);
            if(prev_index != ga->op1->index)
            {
               AppM->RegisterTransformation(GetName(), *itLos);
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded");
      }
      else
      {
         if(ir_helper::IsUnsignedIntegerType(be->type) != ir_helper::IsUnsignedIntegerType(be->op0))
         {
            THROW_ASSERT(ir_helper::IsUnsignedIntegerType(be->type),
                         "Conversion from unsigned to signed is required for first input of " + STR(ga->op1->index));
            const auto ga_nop = ir_man->CreateNopExpr(be->op0, ir_man->CreateUnsigned(ir_helper::CGetType(be->op0)),
                                                      ir_nodeRef(), ir_nodeRef(), function_id);
            if(ga_nop)
            {
               be->op0 = GetPointer<assign_stmt>(ga_nop)->op0;
               AppM->RegisterTransformation(GetName(), ga_nop);
               block.second->PushBefore(ga_nop, *itLos, AppM);
               restartAnalysis = true;
            }
            else
            {
#ifndef NDEBUG
               THROW_UNREACHABLE("Conversion of " + be->op0->ToString() + " cannot be created");
#else
               THROW_WARNING("Implicit type conversion for first input of " + ga->ToString());
#endif
            }
         }
         if(ir_helper::IsUnsignedIntegerType(be->type) != ir_helper::IsUnsignedIntegerType(be->op1))
         {
            THROW_ASSERT(ir_helper::IsUnsignedIntegerType(be->type),
                         "Conversion from unsigned to signed is required for first input of " + STR(ga->op1->index));
            const auto ga_nop = ir_man->CreateNopExpr(be->op1, ir_man->CreateUnsigned(ir_helper::CGetType(be->op1)),
                                                      ir_nodeRef(), ir_nodeRef(), function_id);
            if(ga_nop)
            {
               be->op1 = GetPointer<assign_stmt>(ga_nop)->op0;
               block.second->PushBefore(ga_nop, *itLos, AppM);
               AppM->RegisterTransformation(GetName(), ga_nop);
               restartAnalysis = true;
            }
            else
            {
#ifndef NDEBUG
               THROW_UNREACHABLE("Conversion of " + be->op1->ToString() + " cannot be created");
#else
               THROW_WARNING("Implicit type conversion for first input of " + ga->ToString());
#endif
            }
         }
      }
      return true;
   }
   if(code1 == lt_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Expanding lt_node " + STR(ga->op1->index));
      auto be = GetPointer<binary_node>(ga->op1);
      ir_nodeRef op0 = be->op0;
      bool intp = ir_helper::IsSignedIntegerType(op0);
      if(intp)
      {
         ir_nodeRef op1 = be->op1;
         if(GetPointer<constant_int_val_node>(op1))
         {
            if(ir_helper::GetConstValue(op1) == 0)
            {
               auto op0_type = ir_helper::CGetType(op0);
               ir_nodeRef right_shift_value =
                   TM->CreateUniqueIntegerCst(static_cast<long long>(ir_helper::Size(op0) - 1), op0_type);
               ir_nodeRef rshift1 =
                   ir_man->create_binary_operation(op0_type, op0, right_shift_value, locInfoDefault, shr_node_K);
               ir_nodeRef rshift1_ga =
                   ir_man->CreateAssignStmt(op0_type, ir_nodeRef(), ir_nodeRef(), rshift1, function_id, locInfoDefault);
               block.second->PushBefore(rshift1_ga, *itLos, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + rshift1_ga->ToString());
               ir_nodeRef rshift1_ga_var = GetPointer<assign_stmt>(rshift1_ga)->op0;
               ir_nodeRef bitwise_mask_value = TM->CreateUniqueIntegerCst(1, op0_type);
               ir_nodeRef bitwise_masked = ir_man->create_binary_operation(op0_type, rshift1_ga_var, bitwise_mask_value,
                                                                           locInfoDefault, and_node_K);
               ir_nodeRef bitwise_masked_ga = ir_man->CreateAssignStmt(op0_type, ir_nodeRef(), ir_nodeRef(),
                                                                       bitwise_masked, function_id, locInfoDefault);
               block.second->PushBefore(bitwise_masked_ga, *itLos, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---adding statement " + bitwise_masked_ga->ToString());
               ir_nodeRef bitwise_masked_var = GetPointer<assign_stmt>(bitwise_masked_ga)->op0;
               ir_nodeRef ne = ir_man->create_unary_operation(be->type, bitwise_masked_var, locInfoDefault, nop_node_K);
               ir_nodeRef ga_nop =
                   ir_man->CreateAssignStmt(be->type, ir_nodeRef(), ir_nodeRef(), ne, function_id, locInfoDefault);
               block.second->PushBefore(ga_nop, *itLos, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ga_nop->ToString());
               AppM->RegisterTransformation(GetName(), ga_nop);
               ga->op1 = GetPointer<assign_stmt>(ga_nop)->op0;
               restartAnalysis = true;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
      }
      return true;
   }
   if(code1 == ge_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Expanding ge_node " + STR(ga->op1->index));
      ir_nodeRef op0 = GetPointer<binary_node>(ga->op1)->op0;
      bool intp = ir_helper::IsSignedIntegerType(op0);
      if(intp)
      {
         ir_nodeRef op1 = GetPointer<binary_node>(ga->op1)->op1;
         if(GetPointer<constant_int_val_node>(op1))
         {
            const auto op1_value = ir_helper::GetConstValue(op1);
            if(op1_value == 0)
            {
               auto op0_type = ir_helper::CGetType(op0);
               ir_nodeRef right_shift_value =
                   TM->CreateUniqueIntegerCst(static_cast<long long>(ir_helper::Size(op0) - 1), op0_type);
               ir_nodeRef rshift1 =
                   ir_man->create_binary_operation(op0_type, op0, right_shift_value, locInfoDefault, shr_node_K);
               ir_nodeRef rshift1_ga =
                   ir_man->CreateAssignStmt(op0_type, ir_nodeRef(), ir_nodeRef(), rshift1, function_id, locInfoDefault);
               block.second->PushBefore(rshift1_ga, *itLos, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + rshift1_ga->ToString());
               ir_nodeRef rshift1_ga_var = GetPointer<assign_stmt>(rshift1_ga)->op0;
               ir_nodeRef bitwise_mask_value = TM->CreateUniqueIntegerCst(1, op0_type);
               ir_nodeRef bitwise_masked = ir_man->create_binary_operation(op0_type, rshift1_ga_var, bitwise_mask_value,
                                                                           locInfoDefault, and_node_K);
               ir_nodeRef bitwise_masked_ga = ir_man->CreateAssignStmt(op0_type, ir_nodeRef(), ir_nodeRef(),
                                                                       bitwise_masked, function_id, locInfoDefault);
               block.second->PushBefore(bitwise_masked_ga, *itLos, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---adding statement " + bitwise_masked_ga->ToString());
               ir_nodeRef bitwise_masked_var = GetPointer<assign_stmt>(bitwise_masked_ga)->op0;
               ir_nodeRef ne = ir_man->create_unary_operation(GetPointer<binary_node>(ga->op1)->type,
                                                              bitwise_masked_var, locInfoDefault, nop_node_K);
               ir_nodeRef ga_nop = ir_man->CreateAssignStmt(GetPointer<binary_node>(ga->op1)->type, ir_nodeRef(),
                                                            ir_nodeRef(), ne, function_id, locInfoDefault);
               block.second->PushBefore(ga_nop, *itLos, AppM);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ga_nop->ToString());
               ir_nodeRef ga_nop_var = GetPointer<assign_stmt>(ga_nop)->op0;
               auto booleanType = ir_man->GetBooleanType();
               ir_nodeRef not_masked =
                   ir_man->create_unary_operation(booleanType, ga_nop_var, locInfoDefault, not_node_K);
               ga->op1 = not_masked;
               restartAnalysis = true;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Not expanded");
      }
      return true;
   }
   if(code1 == add_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Expanding add_node " + STR(ga->op1->index));
      ir_nodeRef bop0 = GetPointer<binary_node>(ga->op1)->op0;
      ir_nodeRef bop1 = GetPointer<binary_node>(ga->op1)->op1;
      if(ir_helper::Size(ir_helper::CGetType(ga->op0)) != ir_helper::Size(ir_helper::CGetType(bop0)))
      {
         auto ssa0 = GetPointerS<ssa_node>(ga->op0);
         const auto ga_nop = ir_man->CreateNopExpr(bop0, ssa0->type, ssa0->min, ssa0->max, function_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ga_nop->ToString());
         const auto nop_vd = GetPointer<assign_stmt>(ga_nop)->op0;
         block.second->PushBefore(ga_nop, *itLos, AppM);
         GetPointer<binary_node>(ga->op1)->op0 = bop1 = nop_vd;
         restartAnalysis = true;
      }
      if(ir_helper::Size(ir_helper::CGetType(ga->op0)) != ir_helper::Size(ir_helper::CGetType(bop1)))
      {
         auto ssa0 = GetPointerS<ssa_node>(ga->op0);
         const auto ga_nop = ir_man->CreateNopExpr(bop1, ssa0->type, ssa0->min, ssa0->max, function_id);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + ga_nop->ToString());
         const auto nop_vd = GetPointer<assign_stmt>(ga_nop)->op0;
         block.second->PushBefore(ga_nop, *itLos, AppM);
         GetPointer<binary_node>(ga->op1)->op1 = bop1 = nop_vd;
         restartAnalysis = true;
      }
      bool intp = ir_helper::IsSignedIntegerType(bop0) || ir_helper::IsUnsignedIntegerType(bop0);
      if(intp)
      {
         if(bop0->index == bop1->index)
         {
            ir_nodeRef type = GetPointer<binary_node>(ga->op1)->type;
            ir_nodeRef left_shift_value = TM->CreateUniqueIntegerCst(1, type);
            ir_nodeRef left1 =
                ir_man->create_binary_operation(type, bop0, left_shift_value, locInfoDefault, shl_node_K);
            ga->op1 = left1;
            restartAnalysis = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Expanded");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not Expanded");
         }
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Not Expanded");
      }
      return true;
   }
   return false;
}

bool IR_lowering::handleMemRefOutput(const blocRef& block, const std::list<ir_nodeRef>::const_iterator& itLos,
                                     assign_stmt* ga, enum kind code0, const std::string& locInfoDefault,
                                     bool& restartAnalysis)
{
   if(code0 == mem_access_node_K)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---expand mem_access_node 2 " + STR(ga->op0->index));
      auto* MR = GetPointer<mem_access_node>(ga->op0);
      const auto mem_op0_kind = MR->op->get_kind();
      if(mem_op0_kind == addr_node_K || mem_op0_kind == gep_node_K || mem_op0_kind == bitcast_node_K)
      {
         extractUnaryExpr(block, itLos, MR->op, mem_op0_kind == addr_node_K, true, locInfoDefault, restartAnalysis);
      }
      const auto op1_type = ir_helper::CGetType(ga->op1);
      auto bitcast_pattern = op1_type->get_kind() == struct_ty_node_K && ga->op1->get_kind() == bitcast_node_K;
      if(!bitcast_pattern && ga->op1->get_kind() != ssa_node_K && !GetPointer<cst_node>(ga->op1) &&
         ga->op1->get_kind() != mem_access_node_K && ga->op1->get_kind() != constructor_node_K)
      {
         const auto op_type = ir_helper::CGetType(ga->op1);
         const auto op_ga =
             ir_man->CreateAssignStmt(op_type, ir_nodeRef(), ir_nodeRef(), ga->op1, function_id, locInfoDefault);
         const auto op_vd = GetPointer<assign_stmt>(op_ga)->op0;
         block->PushBefore(op_ga, *itLos, AppM);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + op_ga->ToString());
         ga->op1 = op_vd;
         restartAnalysis = true;
      }
      return true;
   }
   if(code0 == variable_val_node_K)
   {
      auto* vd = GetPointer<variable_val_node>(ga->op0);
      ir_nodeRef type = vd->type;
      // std::cerr << "algn" << GetPointer<type_node>(type)->algn << "\n";
      ir_nodeRef pt = ir_man->GetPointerType(type, GetPointer<type_node>(type)->algn);
      ir_nodeRef ae = ir_man->create_unary_operation(pt, ga->op0, locInfoDefault, addr_node_K);
      ir_nodeRef new_ga = ir_man->CreateAssignStmt(pt, ir_nodeRef(), ir_nodeRef(), ae, function_id, locInfoDefault);
      GetPointer<assign_stmt>(new_ga)->temporary_address = true;
      ir_nodeRef ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;
      auto ssa_var_decl = GetPointer<ssa_node>(ssa_vd);
      ssa_var_decl->use_set.Add(ga->op0);
      ir_nodeRef mr = ir_man->create_unary_operation(type, ssa_vd, locInfoDefault, mem_access_node_K);

      ga->op0 = mr;
      block->PushBefore(new_ga, *itLos, AppM);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + new_ga->ToString());
      restartAnalysis = true;
      return true;
   }
   if(code0 == unaligned_mem_access_node_K)
   {
      auto* MIR = GetPointer<unaligned_mem_access_node>(ga->op0);
      function_behavior->set_unaligned_accesses(true);
      ir_nodeRef type = MIR->type;
      auto pt = ir_man->GetPointerType(type, 8);
      ir_nodeRef mr = ir_man->create_unary_operation(type, MIR->op, locInfoDefault, mem_access_node_K);
      ga->op0 = mr;
      restartAnalysis = true;
      return true;
   }
   return false;
}

void IR_lowering::normalizePhiNodes(const statement_list_node* stmtList)
{
   /// first analyze phis
   for(const auto& [blockId, blockRef] : stmtList->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining PHI of BB" + STR(blockId));
      for(const auto& phi : blockRef->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---phi operation");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---phi index: " + STR(phi->index));
         const auto pn = GetPointerS<phi_stmt>(phi);
         const auto loc_info_default = pn->include_name + ":" + STR(pn->line_number) + ":" + STR(pn->column_number);

         const bool is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            phi_stmt::DefEdgeList to_be_replaced;
            for(const auto& def_edge : pn->CGetDefEdgesList())
            {
               THROW_ASSERT(ir_helper::IsSameType(pn->res, def_edge.first), "required a conversion");
               const auto def_kind = def_edge.first->get_kind();
               if(def_kind == addr_node_K || def_kind == bitcast_node_K || def_kind == nop_node_K ||
                  def_kind == gep_node_K || def_kind == sub_node_K || def_kind == constructor_node_K)
               {
                  to_be_replaced.push_back(def_edge);
               }
            }
            for(const auto& def_edge : to_be_replaced)
            {
               const auto ue = GetPointer<unary_node>(def_edge.first);
               ir_nodeRef op_ga;
               if(ue)
               {
                  const auto ue_expr = ir_man->create_unary_operation(
                      ue->type, ue->op, loc_info_default,
                      def_edge.first->get_kind()); /// It is required to de-share some IR nodes
                  op_ga = ir_man->CreateAssignStmt(ue->type, ir_nodeRef(), ir_nodeRef(), ue_expr, function_id,
                                                   loc_info_default);
               }
               else
               {
                  auto op_node = def_edge.first;
                  const auto en_type = GetPointer<expr_node>(op_node) ? GetPointerS<expr_node>(op_node)->type :
                                                                        GetPointer<constructor_node>(op_node)->type;
                  op_ga = ir_man->CreateAssignStmt(en_type, ir_nodeRef(), ir_nodeRef(), def_edge.first, function_id,
                                                   loc_info_default);
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + op_ga->ToString());
               const auto ue_vd = GetPointerS<assign_stmt>(op_ga)->op0;
               const auto pred_block = stmtList->list_of_bloc.at(def_edge.second);
               if(pred_block->CGetStmtList().empty())
               {
                  pred_block->PushBack(op_ga, AppM);
               }
               else
               {
                  const auto last_statement = pred_block->CGetStmtList().back();
                  const auto last_stmt_kind = last_statement->get_kind();
                  if(last_stmt_kind == multi_way_if_stmt_K || last_stmt_kind == return_stmt_K)
                  {
                     pred_block->PushBefore(op_ga, last_statement, AppM);
                  }
                  else
                  {
                     pred_block->PushAfter(op_ga, last_statement, AppM);
                     GetPointerS<ssa_node>(ue_vd)->SetDefStmt(op_ga);
                  }
               }
               pn->ReplaceDefEdge(TM, def_edge, phi_stmt::DefEdge(ue_vd, def_edge.second));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined PHI of BB" + STR(blockId));
   }
}

DesignFlowStep_Status IR_lowering::InternalExec()
{
   const auto fnode = TM->GetIRNode(function_id);
   auto* fd = GetPointerS<function_val_node>(fnode);
   THROW_ASSERT(fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointerS<statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   normalizePhiNodes(sl);

   /// for each basic block B in CFG do > Consider all blocks successively
   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      const auto& list_of_stmt = block.second->CGetStmtList();
      bool restart_analysis;
      do
      {
         restart_analysis = false;
         auto it_los_end = list_of_stmt.end();
         auto it_los = list_of_stmt.begin();
         IRNodeSet bitfield_vuses;
         IRNodeSet bitfield_vdefs;
         while(it_los != it_los_end)
         {
            if(GetPointer<node_stmt>(*it_los) && GetPointerS<node_stmt>(*it_los)->vdef)
            {
               bitfield_vuses.insert(GetPointer<node_stmt>(*it_los)->vdef);
            }
            if(GetPointer<node_stmt>(*it_los) &&
               (GetPointerS<node_stmt>(*it_los)->vdef || !GetPointerS<node_stmt>(*it_los)->vuses.empty()))
            {
               for(const auto& vd : bitfield_vdefs)
               {
                  GetPointer<node_stmt>(*it_los)->AddVuse(vd);
               }
            }

            const auto loc_info_default = buildLocInfoDefault(*it_los);

            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + (*it_los)->ToString());
            if((*it_los)->get_kind() == assign_stmt_K)
            {
               auto* ga = GetPointer<assign_stmt>(*it_los);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Left part is " + ga->op0->get_kind_text() + " - Right part is " +
                                  ga->op1->get_kind_text());
               const auto code0 = ga->op0->get_kind();
               const auto code1 = ga->op1->get_kind();
               normalizeOperands(block.second, it_los, ga, code0, code1, loc_info_default, restart_analysis);

               if(code1 == ssa_node_K && code0 == ssa_node_K)
               {
                  /// check for a missing cast
                  if(ir_helper::CGetType(ga->op0) != ir_helper::CGetType(ga->op1))
                  {
                     auto ssa0 = GetPointerS<ssa_node>(ga->op0);
                     const auto ga_nop = ir_man->CreateNopExpr(ga->op1, ssa0->type, ssa0->min, ssa0->max, function_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + ga_nop->ToString());
                     const auto nop_vd = GetPointer<assign_stmt>(ga_nop)->op0;
                     block.second->PushBefore(ga_nop, *it_los, AppM);
                     ga->op1 = nop_vd;
                     restart_analysis = true;
                  }
               }
               else if(handleMemRef(block.second, it_los, ga, code0, code1, loc_info_default, restart_analysis))
               {
               }
               else if(code1 == eq_node_K or code1 == ne_node_K or code1 == gt_node_K or code1 == lt_node_K or
                       code1 == ge_node_K or code1 == le_node_K)
               {
                  auto rel_expr1 = [&] {
                     const auto lhs_type = ir_helper::CGetType(ga->op0);
                     if(code0 == ssa_node_K && !ir_helper::IsBooleanType(lhs_type) &&
                        !(ir_helper::IsVectorType(lhs_type) &&
                          ir_helper::IsBooleanType(ir_helper::CGetElements(lhs_type))))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Fix lhs to be bool");
                        // fix the left hand side to be a bool
                        const auto new_left_type = [&]() -> ir_nodeRef {
                           if(!ir_helper::IsVectorType(ga->op0))
                           {
                              return ir_man->GetBooleanType();
                           }
                           const auto element_type = ir_helper::CGetElements(lhs_type);
                           const auto element_size = ir_helper::SizeAlloc(element_type);
                           const auto vector_size = ir_helper::SizeAlloc(lhs_type);
                           const auto num_elements = vector_size / element_size;
                           return ir_man->CreateVectorType(ir_man->GetBooleanType(), num_elements);
                        }();
                        GetPointer<binary_node>(ga->op1)->type = new_left_type;
                        const auto lt_ga = ir_man->CreateAssignStmt(new_left_type, nullptr, nullptr, ga->op1,
                                                                    function_id, loc_info_default);
                        block.second->PushBefore(lt_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + lt_ga->ToString());
                        const auto type_node = ir_helper::CGetType(ga->op0);
                        const auto nop_e = ir_man->create_unary_operation(
                            type_node, GetPointer<const assign_stmt>(lt_ga)->op0, loc_info_default, nop_node_K);
                        ga->op1 = nop_e;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Fixed lhs to be bool");
                        restart_analysis = true;
                     }
                  };
                  rel_expr1();
               }
               else if(code1 == call_node_K)
               {
                  auto* ce = GetPointer<call_node>(ga->op1);
                  for(auto& arg : ce->args)
                  {
                     if(GetPointer<unary_node>(arg) || GetPointer<binary_node>(arg))
                     {
                        extractUnaryExpr(block.second, it_los, arg, arg->get_kind() == addr_node_K, false,
                                         loc_info_default, restart_analysis);
                     }
                  }
                  if(ce->fn->get_kind() == addr_node_K)
                  {
                     auto* ae = GetPointerS<addr_node>(ce->fn);
                     auto actual_ftype = ir_helper::CGetType(ae->op);
                     THROW_ASSERT(actual_ftype->get_kind() == function_ty_node_K,
                                  "addr_node from call_node should have function as operand");
                     ae->type = ir_man->GetPointerType(actual_ftype);
                  }
               }
               else if(code1 == gep_node_K)
               {
                  auto pp_expr1 = [&] {
                     auto* ppe = GetPointer<gep_node>(ga->op1);
                     THROW_ASSERT(ppe->op0 && ppe->op1, "expected two parameters");
                     if(GetPointer<addr_node>(ppe->op0))
                     {
                        extractExpr(block.second, it_los, ppe->op0, ga->temporary_address, loc_info_default,
                                    restart_analysis);
                     }
                     else if(GetPointer<gep_node>(ppe->op0)) /// required by CLANG/LLVM plugin
                     {
                        extractExpr(block.second, it_los, ppe->op0, ga->temporary_address, loc_info_default,
                                    restart_analysis);
                     }
                     else if(GetPointer<mul_node>(ppe->op1)) /// required by CLANG/LLVM plugin
                     {
                        extractExpr(block.second, it_los, ppe->op1, false, loc_info_default, restart_analysis);
                     }
                     else if(GetPointer<variable_val_node>(ppe->op0))
                     {
                        auto* vd = GetPointer<variable_val_node>(ppe->op0);
                        const auto type = vd->type;
                        const auto pt = ir_man->GetPointerType(type, GetPointer<type_node>(type)->algn);
                        const auto ae = ir_man->create_unary_operation(pt, ppe->op0, loc_info_default, addr_node_K);
                        const auto new_ga =
                            ir_man->CreateAssignStmt(pt, ir_nodeRef(), ir_nodeRef(), ae, function_id, loc_info_default);
                        GetPointer<assign_stmt>(new_ga)->temporary_address = true;
                        const auto ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;
                        ppe->op0 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     else if(GetPointer<ssa_node>(ppe->op0) && GetPointer<constant_int_val_node>(ppe->op1))
                     {
                        auto temp_def = GetPointer<const ssa_node>(ppe->op0)->GetDefStmt();
                        if(temp_def->get_kind() == assign_stmt_K)
                        {
                           const auto prev_ga = GetPointer<const assign_stmt>(temp_def);
                           if(prev_ga->op1->get_kind() == gep_node_K)
                           {
                              const auto prev_ppe = GetPointer<const gep_node>(prev_ga->op1);
                              if(GetPointer<ssa_node>(prev_ppe->op0) &&
                                 GetPointer<constant_int_val_node>(prev_ppe->op1))
                              {
                                 const auto prev_val = ir_helper::GetConstValue(prev_ppe->op1);
                                 const auto curr_val = ir_helper::GetConstValue(ppe->op1);
                                 ppe->op1 =
                                     TM->CreateUniqueIntegerCst(prev_val + curr_val, ir_helper::CGetType(ppe->op1));
                                 ppe->op0 = prev_ppe->op0;
                                 restart_analysis = true;
                              }
                           }
                        }
                     }
                     else if(GetPointer<ssa_node>(ppe->op0))
                     {
                        auto temp_def = GetPointer<const ssa_node>(ppe->op0)->GetDefStmt();
                        if(temp_def->get_kind() == assign_stmt_K)
                        {
                           const auto prev_ga = GetPointer<const assign_stmt>(temp_def);
                           if(prev_ga->op1->get_kind() == addr_node_K)
                           {
                              auto* prev_ae = GetPointer<addr_node>(prev_ga->op1);
                              enum kind prev_ae_code = prev_ae->op->get_kind();
                              if(prev_ae_code == mem_access_node_K)
                              {
                                 auto* prev_MR = GetPointer<mem_access_node>(prev_ae->op);
                                 if(prev_MR->op->get_kind() == ssa_node_K)
                                 {
                                    ppe->op0 = prev_MR->op;
                                    restart_analysis = true;
                                 }
                              }
                           }
                        }
                     }
                  };
                  pp_expr1();
               }
               else if(code1 == bitcast_node_K || code1 == nop_node_K)
               {
                  auto vcne_expr1 = [&] {
                     auto* ue = GetPointer<unary_node>(ga->op1);
                     if(ue->op->get_kind() == variable_val_node_K)
                     {
                        auto bitcast_expr = GetPointer<bitcast_node>(ga->op1);
                        ir_nodeRef pt =
                            ir_man->GetPointerType(bitcast_expr->type, GetPointer<type_node>(bitcast_expr->type)->algn);
                        ir_nodeRef ae = ir_man->create_unary_operation(pt, ue->op, loc_info_default, addr_node_K);
                        ir_nodeRef new_ga =
                            ir_man->CreateAssignStmt(pt, ir_nodeRef(), ir_nodeRef(), ae, function_id, loc_info_default);
                        GetPointer<assign_stmt>(new_ga)->temporary_address = true;
                        ir_nodeRef ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;
                        ir_nodeRef mr = ir_man->create_unary_operation(bitcast_expr->type, ssa_vd, loc_info_default,
                                                                       mem_access_node_K);
                        ga->op1 = mr;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     else if(ue->op->get_kind() != ssa_node_K && !GetPointer<cst_node>(ue->op))
                     {
                        auto op_type = ir_helper::CGetType(ue->op);
                        ir_nodeRef op_ga = ir_man->CreateAssignStmt(op_type, ir_nodeRef(), ir_nodeRef(), ue->op,
                                                                    function_id, loc_info_default);
                        ir_nodeRef op_vd = GetPointer<assign_stmt>(op_ga)->op0;
                        if(ga->temporary_address)
                        {
                           GetPointer<assign_stmt>(op_ga)->temporary_address = true;
                        }
                        block.second->PushBefore(op_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + op_ga->ToString());
                        ue->op = op_vd;
                        restart_analysis = true;
                     }
                  };
                  vcne_expr1();
               }
               else if(code1 == select_node_K)
               {
                  auto ce_expr1 = [&] {
                     auto* ce = GetPointer<select_node>(ga->op1);
                     THROW_ASSERT(ce->op1 && ce->op2, "expected three parameters");
                     if(GetPointer<binary_node>(ce->op0))
                     {
#if HAVE_ASSERTS
                        auto* be = GetPointer<binary_node>(ce->op0);
                        THROW_ASSERT(be->get_kind() == le_node_K or be->get_kind() == eq_node_K or
                                         be->get_kind() == ne_node_K or be->get_kind() == gt_node_K or
                                         be->get_kind() == lt_node_K or be->get_kind() == ge_node_K,
                                     be->get_kind_text());
#endif
                        auto bt = ir_man->GetBooleanType();
                        ir_nodeRef new_ga = ir_man->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                     TM->CreateUniqueIntegerCst(1, bt), ce->op0,
                                                                     function_id, loc_info_default);
                        ir_nodeRef ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;

                        ce->op0 = ssa_vd;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        restart_analysis = true;
                     }
                     else if(!ir_helper::IsBooleanType(ce->op0))
                     {
                        const auto bt = ir_man->GetBooleanType();
                        const auto ga_nop = ir_man->CreateNopExpr(ce->op0, bt, TM->CreateUniqueIntegerCst(0, bt),
                                                                  TM->CreateUniqueIntegerCst(1, bt), function_id);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + ga_nop->ToString());
                        block.second->PushBefore(ga_nop, *it_los, AppM);
                        ce->op0 = GetPointer<assign_stmt>(ga_nop)->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---modified statement " + STR(ga));
                        restart_analysis = true;
                     }
                  };
                  ce_expr1();
               }
               else if(code1 == unaligned_mem_access_node_K)
               {
                  auto* MIR = GetPointer<unaligned_mem_access_node>(ga->op1);
                  function_behavior->set_unaligned_accesses(true);
                  ir_nodeRef type = MIR->type;
                  auto pt = ir_man->GetPointerType(type, 8);
                  ir_nodeRef mr = ir_man->create_unary_operation(type, MIR->op, loc_info_default, mem_access_node_K);
                  ga->op1 = mr;
                  restart_analysis = true;
               }
               else if(code1 == variable_val_node_K)
               {
                  auto vd_expr1 = [&] {
                     auto* vd = GetPointer<variable_val_node>(ga->op1);
                     ir_nodeRef type = vd->type;
                     ir_nodeRef pt = ir_man->GetPointerType(type, GetPointer<type_node>(type)->algn);
                     ir_nodeRef ae = ir_man->create_unary_operation(pt, ga->op1, loc_info_default, addr_node_K);
                     ir_nodeRef new_ga =
                         ir_man->CreateAssignStmt(pt, ir_nodeRef(), ir_nodeRef(), ae, function_id, loc_info_default);
                     GetPointer<assign_stmt>(new_ga)->temporary_address = true;
                     ir_nodeRef ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;
                     auto ssa_var_decl = GetPointer<ssa_node>(ssa_vd);
                     ssa_var_decl->use_set.Add(ga->op1);
                     ir_nodeRef offset = TM->CreateUniqueIntegerCst(0, pt);
                     ir_nodeRef mr = ir_man->create_unary_operation(type, ssa_vd, loc_info_default, mem_access_node_K);

                     ga->op1 = mr;
                     block.second->PushBefore(new_ga, *it_los, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---adding statement " + new_ga->ToString());
                     restart_analysis = true;
                  };
                  vd_expr1();
               }
               else if(reached_max_transformation_limit(*it_los))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Reached max cfg transformations (" + ga->op1->get_kind_text() + ")");
               }
               else
               {
                  if(!lowerArithExpr(block, it_los, ga, code1, loc_info_default, restart_analysis))
                  {
                     if(code1 == argument_val_node_K)
                     {
                        auto* pd = GetPointer<argument_val_node>(ga->op1);
                        ir_nodeRef type = pd->type;
                        ir_nodeRef pt = ir_man->GetPointerType(type, 8);
                        ir_nodeRef ae = ir_man->create_unary_operation(pt, ga->op1, loc_info_default, addr_node_K);
                        ir_nodeRef new_ga =
                            ir_man->CreateAssignStmt(pt, ir_nodeRef(), ir_nodeRef(), ae, function_id, loc_info_default);
                        GetPointer<assign_stmt>(new_ga)->temporary_address = true;
                        ir_nodeRef ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;
                        ir_nodeRef mr =
                            ir_man->create_unary_operation(type, ssa_vd, loc_info_default, mem_access_node_K);

                        ga->op1 = mr;
                        block.second->PushBefore(new_ga, *it_los, AppM);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---adding statement " + new_ga->ToString());
                        AppM->RegisterTransformation(GetName(), new_ga);
                        restart_analysis = true;
                     }
                  }
               }

               if(!handleMemRefOutput(block.second, it_los, ga, code0, loc_info_default, restart_analysis))
               {
                  if(reached_max_transformation_limit(*it_los))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Reached max cfg transformations");
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "<--Examined statement " + (*it_los)->ToString());
                     it_los++;
                     continue;
                  }
               }
               if(code0 == argument_val_node_K)
               {
                  auto* pd = GetPointer<argument_val_node>(ga->op0);
                  ir_nodeRef type = pd->type;
                  ir_nodeRef pt = ir_man->GetPointerType(type, 8);
                  ir_nodeRef ae = ir_man->create_unary_operation(pt, ga->op0, loc_info_default, addr_node_K);
                  ir_nodeRef new_ga =
                      ir_man->CreateAssignStmt(pt, ir_nodeRef(), ir_nodeRef(), ae, function_id, loc_info_default);
                  GetPointer<assign_stmt>(new_ga)->temporary_address = true;
                  ir_nodeRef ssa_vd = GetPointer<assign_stmt>(new_ga)->op0;
                  ir_nodeRef mr = ir_man->create_unary_operation(type, ssa_vd, loc_info_default, mem_access_node_K);

                  ga->op0 = mr;
                  block.second->PushBefore(new_ga, *it_los, AppM);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---adding statement " + new_ga->ToString());
                  AppM->RegisterTransformation(GetName(), new_ga);
                  restart_analysis = true;
               }
            }
            else if((*it_los)->get_kind() == call_stmt_K)
            {
               auto* gc = GetPointer<call_stmt>(*it_los);
               for(auto& arg : gc->args)
               {
                  if(GetPointer<unary_node>(arg) || GetPointer<binary_node>(arg)) /// required by the CLANG/LLVM plugin
                  {
                     extractUnaryExpr(block.second, it_los, arg, arg->get_kind() == addr_node_K, false,
                                      loc_info_default, restart_analysis);
                  }
               }
            }
            else if((*it_los)->get_kind() == return_stmt_K)
            {
               auto* gr = GetPointer<return_stmt>(*it_los);
               if(gr->op)
               {
                  if(GetPointer<unary_node>(gr->op) || GetPointer<binary_node>(gr->op))
                  {
                     extractExpr(block.second, it_los, gr->op, false, loc_info_default, restart_analysis);
                  }
               }
               else
               {
                  const auto ret_type = ir_helper::GetFunctionReturnType(fnode);
                  if(ret_type)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level,
                                    "Control reaches end of non-void function, '" + ir_helper::GetFunctionName(fnode) +
                                        "' will return zero");
                     gr->op = TM->CreateUniqueIntegerCst(0LL, ret_type);
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + (*it_los)->ToString());
            it_los++;
         }
      } while(restart_analysis);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined BB" + STR(block.first));
   }
   function_behavior->UpdateBBVersion();
   return DesignFlowStep_Status::SUCCESS;
}
