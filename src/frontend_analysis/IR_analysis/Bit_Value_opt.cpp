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
 * @file Bit_Value_opt.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "Bit_Value_opt.hpp"

#include "BitLatticeManipulator.hpp"
#include "Discrepancy.hpp"
#include "Parameter.hpp"
#include "Range.hpp"
#include "allocation_constants.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include "utility.hpp"

#include <boost/range/adaptor/reversed.hpp>

#include <cmath>
#include <fstream>
#include <string>

#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"

#define DEBUG_CALLSITE (__FILE__ + std::string(":") + STR(__LINE__))

Bit_Value_opt::Bit_Value_opt(const ParameterConstRef _parameters, const application_managerRef _AppM,
                             unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BIT_VALUE_OPT, _design_flow_manager, _parameters), modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
Bit_Value_opt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
         if(parameters->isOption(OPT_bitvalue_ipa) && parameters->getOption<bool>(OPT_bitvalue_ipa))
         {
            relationships.insert(std::make_pair(BIT_VALUE_IPA, WHOLE_APPLICATION));
         }

         relationships.insert(std::make_pair(BIT_VALUE_OPT, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(PARM2SSA, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

bool Bit_Value_opt::HasToBeExecuted() const
{
   return FunctionFrontendFlowStep::HasToBeExecuted() || bitvalue_version != function_behavior->GetBitValueVersion();
}

DesignFlowStep_Status Bit_Value_opt::InternalExec()
{
   if(parameters->IsParameter("bitvalue-opt") && !parameters->GetParameter<unsigned int>("bitvalue-opt"))
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " --------- BIT_VALUE_OPT ----------");
   const auto TM = AppM->get_ir_manager();
   const auto IRman = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));

   const auto fd = GetPointerS<const function_val_node>(TM->GetIRNode(function_id));
   THROW_ASSERT(fd->body, "Function has no implementation");
   modified = false;
   optimize(fd, TM, IRman);
   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void Bit_Value_opt::constrainSSA(ssa_node* op_ssa, const ir_managerRef& TM)
{
   if(ir_helper::IsRealType(op_ssa->type))
   {
      return;
   }
   const auto nbit = op_ssa->bit_values.size();
   THROW_ASSERT(!op_ssa->bit_values.empty(), "unexpected condition");
   const auto nbitType = ir_helper::TypeSize(op_ssa->type);
   if(nbit != nbitType)
   {
      const bool isSigned = ir_helper::IsSignedIntegerType(op_ssa->type);
      if(isSigned)
      {
         Range constraintRange(Regular, static_cast<Range::bw_t>(nbitType), -(1ll << (nbit - 1)),
                               (1ll << (nbit - 1)) - 1);
         if(!op_ssa->range.isUnknown())
         {
            if(op_ssa->range.getSpan() < constraintRange.getSpan())
            {
               return;
            }
         }
         else
         {
            op_ssa->range = constraintRange;
         }
         op_ssa->min = TM->CreateUniqueIntegerCst(-(1ll << (nbit - 1)), op_ssa->type);
         op_ssa->max = TM->CreateUniqueIntegerCst((1ll << (nbit - 1)) - 1, op_ssa->type);
      }
      else
      {
         Range constraintRange(Regular, static_cast<Range::bw_t>(nbitType), 0, (1ll << nbit) - 1);
         if(!op_ssa->range.isUnknown())
         {
            if(op_ssa->range.getSpan() < constraintRange.getSpan())
            {
               return;
            }
         }
         else
         {
            op_ssa->range = constraintRange;
         }
         op_ssa->min = TM->CreateUniqueIntegerCst(0, op_ssa->type);
         op_ssa->max = TM->CreateUniqueIntegerCst((1ll << nbit) - 1, op_ssa->type);
      }
      // std::cerr<<"var " << op_ssa->ToString()<<" ";
      // std::cerr << "min " <<op_ssa->min->ToString() << " max " <<op_ssa->max->ToString()<<"\n";
      // std::cerr << "nbit "<< nbit << " nbitType " << nbitType <<"\n";
   }
}

static integer_cst_t convert_bitvalue_to_integer_cst(const std::string& bit_values, const ir_nodeConstRef& var_node)
{
   integer_cst_t const_value = 0;
   size_t index_val = 0;
   for(auto current_el : boost::adaptors::reverse(bit_values))
   {
      if(current_el == '1')
      {
         const_value |= integer_cst_t(1) << index_val;
      }
      ++index_val;
   }
   /// in case do sign extension
   const auto is_signed = ir_helper::IsSignedIntegerType(var_node);
   if(is_signed && bit_values[0] == '1')
   {
      const_value |= integer_cst_t(-1) << index_val;
      THROW_ASSERT(const_value < 0, "");
   }
   return const_value;
}

static inline bool is_bit_values_constant(const std::string& bit_values)
{
   return bit_values.size() && bit_values.find('U') == std::string::npos;
}

void Bit_Value_opt::propagateValue(const ir_managerRef& TM, const ir_nodeRef& old_val, const ir_nodeRef& new_val,
                                   const std::string&
#if HAVE_ASSERTS
                                       callSiteString
#endif
)
{
   THROW_ASSERT(old_val->get_kind() == ssa_node_K, "unexpected condition");
   const auto* old_ssa = GetPointerS<ssa_node>(old_val);
   const auto replace_with_cst = ir_helper::IsConstant(new_val);
   THROW_ASSERT(replace_with_cst || ir_helper::Size(old_val) >= ir_helper::Size(new_val),
                "unexpected case " + STR(old_val) + "(bw: " + STR(ir_helper::Size(old_val)) +
                    ", bv: " + old_ssa->bit_values + ") " + STR(new_val) + "(bw: " + STR(ir_helper::Size(new_val)) +
                    ")\n        " + callSiteString);
   const auto def = old_ssa->GetDefStmt();
   const auto old_uses = old_ssa->CGetUseStmts();
   for(const auto& [user, use_count] : old_uses)
   {
      if(!AppM->ApplyNewTransformation())
      {
         break;
      }
      if(replace_with_cst && user->index == def->index)
      {
         THROW_ASSERT(def->get_kind() == phi_stmt_K, "Self-use in non-phi statement: " + STR(user));
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---skip self phi-var usage : " + user->ToString());
         continue;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + user->ToString());
      TM->ReplaceIRNode(user, old_val, new_val);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after : " + user->ToString());
      modified = true;
      AppM->RegisterTransformation(GetName(), user);
   }
}

void Bit_Value_opt::optimize(const function_val_node* fd, const ir_managerRef& TM, const ir_manipulationRef& IRman)
{
   THROW_ASSERT(GetPointerS<const HLS_manager>(AppM)->get_HLS_device(), "unexpected condition");
   const auto hls_d = GetPointerS<const HLS_manager>(AppM)->get_HLS_device();
   const auto max_lut_size = AppM->GetRequiredParameterFromParameterOrDevice<size_t>("max_lut_size", hls_d);
   THROW_ASSERT(max_lut_size > 0, "Invalid parameter \"max_lut_size\": expected value > 0");

   /// in case propagate constants from parameters
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      const unsigned int p_decl_id = AppM->getSSAFromParm(function_id, parm_decl_node->index);
      const auto param_ssa_ref = TM->GetIRNode(p_decl_id);
      const auto ssa_type = ir_helper::CGetType(param_ssa_ref);
      if(!BitLatticeManipulator::IsHandledByBitvalue(ssa_type))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Parameter not supported " + param_ssa_ref->ToString());
         continue;
      }
      if(AppM->ApplyNewTransformation())
      {
         const auto p = GetPointerS<const ssa_node>(param_ssa_ref);
         THROW_ASSERT(!p->bit_values.empty(), "unexpected condition");
         if(is_bit_values_constant(p->bit_values))
         {
            const auto ull_value = convert_bitvalue_to_integer_cst(p->bit_values, param_ssa_ref);
            const auto val = TM->CreateUniqueIntegerCst(ull_value, ir_helper::CGetType(parm_decl_node));
            propagateValue(TM, param_ssa_ref, val, DEBUG_CALLSITE);
         }
      }
   }
   auto sl = GetPointerS<statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   for(const auto& idx_bb : sl->list_of_bloc)
   {
      const auto& B = idx_bb.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing BB" + STR(B->number));
      const auto list_of_stmt = B->CGetStmtList();
      for(const auto& stmt : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing statement " + stmt->ToString());
         if(!AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Skipped because reached limit of CFG transformations");
            break;
         }
         if(GetPointerS<node_stmt>(stmt)->keep)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Skipped because the statement has been annotated with the keep tag");
            continue;
         }
         if(stmt->get_kind() != assign_stmt_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because not an assign statement");
            continue;
         }
         const auto ga = GetPointerS<assign_stmt>(stmt);
         const auto& lhs_node = ga->op0;
         if(lhs_node->get_kind() != ssa_node_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Skipped because left hand side is not an ssa name");
            continue;
         }
         const auto ssa = GetPointerS<ssa_node>(lhs_node);
         if(ssa->bit_values.empty() || ssa->CGetUseStmts().empty())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Skipped because left hand side has empty bit value or no uses");
            continue;
         }
         const auto lhs_type = ir_helper::CGetType(lhs_node);
         const auto& rhs_node = ga->op1;
         const auto rhs_kind = rhs_node->get_kind();
         const auto rhs_type = ir_helper::CGetType(rhs_node);
         const auto loc_info_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
         const auto propagateCurrValue = std::bind(&Bit_Value_opt::propagateValue, this, TM, lhs_node,
                                                   std::placeholders::_1, std::placeholders::_2);
         if(ir_helper::IsRealType(lhs_node))
         {
            if(rhs_kind == select_node_K)
            {
               const auto ce = GetPointerS<select_node>(rhs_node);
               if(ce->op1->index == ce->op2->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with equal operands");
                  propagateCurrValue(ce->op1, DEBUG_CALLSITE);
               }
               else if(ce->op0->get_kind() == constant_int_val_node_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with constant condition");
                  const auto new_val = ir_helper::GetConstValue(ce->op0) ? ce->op1 : ce->op2;
                  propagateCurrValue(new_val, DEBUG_CALLSITE);
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---nothing more can be done");
               }
            }
            else if(ir_helper::IsConstant(rhs_node))
            {
               propagateCurrValue(rhs_node, DEBUG_CALLSITE);
            }
            else if(rhs_kind == ssa_node_K)
            {
               propagateCurrValue(rhs_node, DEBUG_CALLSITE);
            }
            else if(rhs_kind == bitcast_node_K)
            {
               const auto bitcast_expr = GetPointerS<bitcast_node>(rhs_node);
               if(bitcast_expr->op->get_kind() == constant_int_val_node_K)
               {
                  const auto cst_val = ir_helper::GetConstValue(bitcast_expr->op);
                  auto bitwidth_op = ir_helper::TypeSize(bitcast_expr->type);
                  ir_nodeRef val;
                  if(bitwidth_op == 32)
                  {
                     union
                     {
                        float dest;
                        int source;
                     } __conv_union = {};
                     __conv_union.source = static_cast<int>(cst_val);
                     val = TM->CreateUniqueRealCst(static_cast<long double>(__conv_union.dest), bitcast_expr->type);
                  }
                  else if(bitwidth_op == 64)
                  {
                     union
                     {
                        double dest;
                        long long int source;
                     } __conv_union = {};
                     __conv_union.source = static_cast<long long int>(cst_val);
                     val = TM->CreateUniqueRealCst(static_cast<long double>(__conv_union.dest), bitcast_expr->type);
                  }
                  else
                  {
                     THROW_ERROR("not supported floating point bitwidth");
                  }
                  propagateCurrValue(val, DEBUG_CALLSITE);
               }
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---real variables not considered: " + STR(lhs_node));
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
            continue;
         }
         if(ir_helper::IsVectorType(lhs_type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---vector variables not considered: " + STR(lhs_node));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
            continue;
         }
         else if((rhs_kind == constant_int_val_node_K || rhs_kind == constant_fp_val_node_K) &&
                 ir_helper::IsPointerType(rhs_type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---constant pointer value assignments not considered: " + STR(lhs_node));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
            continue;
         }
         else if((rhs_kind == call_node_K) && ga->vdef)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---calls with side effects cannot be optimized: " + STR(rhs_node));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
            continue;
         }
         else if(rhs_kind == addr_node_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "---addr_node cannot be optimized: " + STR(rhs_node));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
            continue;
         }

         const auto condPropageValue = [&](const ir_nodeRef& val, const std::string& debug_callsite) {
            const auto bw_op0 = ir_helper::Size(lhs_node);
            const auto bw_op1 = ir_helper::Size(val);
            if(bw_op1 <= bw_op0)
            {
               propagateCurrValue(val, debug_callsite);
            }
            else
            {
               const auto lhs_type_size = ir_helper::Size(lhs_type);
               const auto shift_offset =
                   TM->CreateUniqueIntegerCst(static_cast<long long>(lhs_type_size - bw_op0), lhs_type);
               const auto shl_expr =
                   IRman->create_binary_operation(lhs_type, val, shift_offset, loc_info_default, shl_node_K);
               const auto shl =
                   IRman->CreateAssignStmt(lhs_type, nullptr, nullptr, shl_expr, function_id, loc_info_default);
               const auto svar = GetPointerS<const assign_stmt>(shl)->op0;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(shl));
               B->PushBefore(shl, stmt, AppM);
               const auto shr_expr =
                   IRman->create_binary_operation(lhs_type, svar, shift_offset, loc_info_default, shr_node_K);
               const auto op0_ga =
                   IRman->CreateAssignStmt(lhs_type, ssa->min, ssa->max, shr_expr, function_id, loc_info_default);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
               B->PushBefore(op0_ga, stmt, AppM);
               auto op0_ga_var = GetPointerS<assign_stmt>(op0_ga)->op0;
               auto op0_ga_varSSA = GetPointerS<ssa_node>(op0_ga_var);
               op0_ga_varSSA->bit_values = ssa->bit_values;
               constrainSSA(op0_ga_varSSA, TM);
               propagateCurrValue(op0_ga_var, DEBUG_CALLSITE);
            }
         };

         const auto& bit_values = ssa->bit_values;
         const auto rel_expr_BVO = [&] {
            const auto me = GetPointerS<binary_node>(rhs_node);
            const auto& op0 = me->op0;
            const auto& op1 = me->op1;

            std::string s0, s1;
            const auto is_op0_ssa = op0->get_kind() == ssa_node_K;
            const auto is_op1_ssa = op1->get_kind() == ssa_node_K;
            if(!is_op0_ssa && !is_op1_ssa)
            {
               return;
            }
            if(is_op0_ssa)
            {
               s0 = GetPointerS<const ssa_node>(op0)->bit_values;
               if(s0.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped because bitvalue of op0 is empty");
                  return;
               }
            }
            else
            {
               THROW_ASSERT(op0->get_kind() == constant_int_val_node_K, "unexpected condition");
               const auto uvalue = ir_helper::GetConstValue(op0, false);
               s0 = convert_to_binary(uvalue, ir_helper::TypeSize(op0));
            }
            if(is_op1_ssa)
            {
               s1 = GetPointerS<const ssa_node>(op1)->bit_values;
               if(s1.empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Skipped because bitvalue of op1 is empty");
                  return;
               }
            }
            else
            {
               THROW_ASSERT(op1->get_kind() == constant_int_val_node_K, "unexpected condition");
               const auto uvalue = ir_helper::GetConstValue(op1, false);
               s1 = convert_to_binary(uvalue, ir_helper::TypeSize(op1));
            }

            unsigned int trailing_zero = 0;
            for(auto s0it = s0.rbegin(), s1it = s1.rbegin(), s0end = s0.rend(), s1end = s1.rend();
                s0it != s0end && s1it != s1end; ++s0it, ++s1it)
            {
               if((*s0it == *s1it && (*s1it == '0' || *s1it == '1')) || *s0it == 'X' || *s1it == 'X')
               {
                  ++trailing_zero;
               }
               else
               {
                  break;
               }
            }
            auto min_size = std::min(s0.size(), s1.size());
            if(trailing_zero < min_size && trailing_zero)
            {
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                              "-->Bit Value Opt: " + rhs_node->get_kind_text() +
                                  " optimized, nbits = " + STR(trailing_zero));
               INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
               modified = true;
               AppM->RegisterTransformation(GetName(), stmt);
               const auto op0_op_type = ir_helper::CGetType(op0);
               const auto op1_op_type = ir_helper::CGetType(op1);

               if(is_op0_ssa)
               {
                  const auto op0_const_node =
                      TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), op0_op_type);
                  const auto op0_expr =
                      IRman->create_binary_operation(op0_op_type, op0, op0_const_node, loc_info_default, shr_node_K);
                  const auto op0_ga =
                      IRman->CreateAssignStmt(op0_op_type, nullptr, nullptr, op0_expr, function_id, loc_info_default);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                  B->PushBefore(op0_ga, stmt, AppM);
                  const auto op0_ga_var = GetPointerS<const assign_stmt>(op0_ga)->op0;
                  TM->ReplaceIRNode(stmt, op0, op0_ga_var);
                  /// set the bit_values to the ssa var
                  auto op0_ssa = GetPointerS<ssa_node>(op0_ga_var);
                  THROW_ASSERT(s0.size() - trailing_zero > 0, "unexpected condition");
                  op0_ssa->bit_values = s0.substr(0, s0.size() - trailing_zero);
                  constrainSSA(op0_ssa, TM);
               }
               else
               {
                  const auto cst_val = ir_helper::GetConstValue(op0, ir_helper::IsSignedIntegerType(op0));
                  TM->ReplaceIRNode(stmt, op0, TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op0_op_type));
               }
               if(is_op1_ssa)
               {
                  const auto op1_const_node =
                      TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), op1_op_type);
                  const auto op1_expr =
                      IRman->create_binary_operation(op1_op_type, op1, op1_const_node, loc_info_default, shr_node_K);
                  const auto op1_ga =
                      IRman->CreateAssignStmt(op1_op_type, nullptr, nullptr, op1_expr, function_id, loc_info_default);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                  B->PushBefore(op1_ga, stmt, AppM);
                  const auto op1_ga_var = GetPointerS<const assign_stmt>(op1_ga)->op0;
                  TM->ReplaceIRNode(stmt, op1, op1_ga_var);
                  /// set the bit_values to the ssa var
                  const auto op1_ssa = GetPointerS<ssa_node>(op1_ga_var);
                  THROW_ASSERT(s1.size() - trailing_zero > 0, "unexpected condition");
                  op1_ssa->bit_values = s1.substr(0, s1.size() - trailing_zero);
                  constrainSSA(op1_ssa, TM);
               }
               else
               {
                  const auto cst_val = ir_helper::GetConstValue(op1, ir_helper::IsSignedIntegerType(op1));
                  TM->ReplaceIRNode(stmt, op1, TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op1_op_type));
               }
            }
         };

         if(is_bit_values_constant(bit_values) && !ir_helper::IsPointerType(rhs_type))
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Left part is constant " + bit_values);
            ir_nodeRef val;
            if(rhs_kind == constant_int_val_node_K && ir_helper::Size(rhs_node) <= bit_values.size())
            {
               val = rhs_node;
            }
            else
            {
               const auto const_value = convert_bitvalue_to_integer_cst(bit_values, lhs_type);
               val = TM->CreateUniqueIntegerCst(const_value, lhs_type);
            }
            if(AppM->ApplyNewTransformation())
            {
               if(ga->predicate && (ga->predicate->get_kind() != constant_int_val_node_K ||
                                    ir_helper::GetConstValue(ga->predicate) != 0))
               {
                  const auto zeroval = TM->CreateUniqueIntegerCst(0, IRman->GetBooleanType());
                  TM->ReplaceIRNode(stmt, ga->predicate, zeroval);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---zero predicated statement: " + stmt->ToString());
                  modified = true;
                  AppM->RegisterTransformation(GetName(), stmt);
               }
            }
            condPropageValue(val, DEBUG_CALLSITE);
         }
         else if(ir_helper::IsConstant(rhs_node))
         {
            propagateCurrValue(rhs_node, DEBUG_CALLSITE);
         }
         else if(rhs_kind == ssa_node_K)
         {
            if(!ssa->bit_values.empty() && ssa->bit_values.at(0) == '0' && ssa->bit_values.size() <= 64)
            {
               const auto bit_mask_constant_node =
                   TM->CreateUniqueIntegerCst((1LL << (ssa->bit_values.size() - 1)) - 1, lhs_type);
               const auto band_expr = IRman->create_binary_operation(lhs_type, rhs_node, bit_mask_constant_node,
                                                                     loc_info_default, and_node_K);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---before: " + stmt->ToString());
               TM->ReplaceIRNode(stmt, rhs_node, band_expr);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---after : " + stmt->ToString());
               modified = true;
               ga->keep = true; /// this prevent an infinite loop with CSE
            }
            else
            {
               const auto bw_op0 = ir_helper::Size(lhs_node);
               const auto bw_op1 = ir_helper::Size(rhs_node);

               if(bw_op1 <= bw_op0)
               {
                  propagateCurrValue(rhs_node, DEBUG_CALLSITE);
               }
            }
         }
         else if(rhs_kind == mul_node_K || rhs_kind == widen_mul_node_K)
         {
            const auto mul_node_BVO = [&] {
               const auto me = GetPointerS<const binary_node>(rhs_node);
               const auto& op0 = me->op0;
               const auto& op1 = me->op1;
               bool squareP = op0->index == op1->index;
               const auto data_bitsize_in0 = ceil_pow2(ir_helper::TypeSize(op0));
               const auto data_bitsize_in1 = ceil_pow2(ir_helper::TypeSize(op1));
               const auto isSigned = ir_helper::IsSignedIntegerType(lhs_type);
               if(!isSigned && rhs_kind == mul_node_K && (data_bitsize_in0 == 1 || data_bitsize_in1 == 1))
               {
                  modified = true;
                  AppM->RegisterTransformation(GetName(), stmt);
                  const auto constNE0 = TM->CreateUniqueIntegerCst(0LL, lhs_type);
                  const auto bt = IRman->GetBooleanType();
                  const auto cond_op0 = IRman->create_binary_operation(bt, data_bitsize_in0 == 1 ? op0 : op1, constNE0,
                                                                       loc_info_default, ne_node_K);
                  const auto op0_ga = IRman->CreateAssignStmt(bt, TM->CreateUniqueIntegerCst(0LL, bt),
                                                              TM->CreateUniqueIntegerCst(1LL, bt), cond_op0,
                                                              function_id, loc_info_default);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                  B->PushBefore(op0_ga, stmt, AppM);
                  const auto op0_ga_var = GetPointerS<const assign_stmt>(op0_ga)->op0;
                  const auto const0 = TM->CreateUniqueIntegerCst(0LL, lhs_type);
                  const auto cond_op = IRman->create_ternary_operation(
                      lhs_type, op0_ga_var, data_bitsize_in1 == 1 ? op0 : op1, const0, loc_info_default, select_node_K);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Replacing " + STR(rhs_node) + " with " + STR(cond_op) + " in " + STR(stmt));
                  TM->ReplaceIRNode(stmt, rhs_node, cond_op);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace expression with a select_node: " + stmt->ToString());
               }
               else
               {
                  unsigned int trailing_zero_op0 = 0;
                  unsigned int trailing_zero_op1 = 0;
                  std::string bit_values_op0;
                  std::string bit_values_op1;
                  const auto is_op0_ssa = op0->get_kind() == ssa_node_K;
                  const auto is_op1_ssa = op1->get_kind() == ssa_node_K;
                  if(is_op0_ssa)
                  {
                     bit_values_op0 = GetPointerS<const ssa_node>(op0)->bit_values;
                     if(bit_values_op0.empty())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Skipped because bitvalue of op0 is empty");
                        return;
                     }
                     for(auto current_el : boost::adaptors::reverse(bit_values_op0))
                     {
                        if(current_el == '0')
                        {
                           ++trailing_zero_op0;
                        }
                        else
                        {
                           break;
                        }
                     }
                  }
                  else
                  {
                     THROW_ASSERT(op0->get_kind() == constant_int_val_node_K, "unexpected case");
                     const auto cst_val = ir_helper::GetConstValue(op0, false);
                     bit_values_op0 = convert_to_binary(cst_val, ir_helper::Size(op0));
                     for(unsigned int index = 0; index < bit_values_op0.size() && cst_val != 0; ++index)
                     {
                        if(cst_val & (integer_cst_t(1) << index))
                        {
                           break;
                        }
                        else
                        {
                           ++trailing_zero_op0;
                        }
                     }
                  }
                  if(is_op1_ssa)
                  {
                     bit_values_op1 = GetPointerS<const ssa_node>(op1)->bit_values;
                     if(bit_values_op1.empty())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Skipped because bitvalue of op1 is empty");
                        return;
                     }
                     for(auto current_el : boost::adaptors::reverse(bit_values_op1))
                     {
                        if(current_el == '0')
                        {
                           ++trailing_zero_op1;
                        }
                        else
                        {
                           break;
                        }
                     }
                  }
                  else
                  {
                     THROW_ASSERT(op1->get_kind() == constant_int_val_node_K, "unexpected case");
                     const auto cst_val = ir_helper::GetConstValue(op1, false);
                     bit_values_op1 = convert_to_binary(cst_val, ir_helper::Size(op1));
                     for(unsigned int index = 0; index < bit_values_op1.size() && cst_val != 0; ++index)
                     {
                        if(cst_val & (integer_cst_t(1) << index))
                        {
                           break;
                        }
                        else
                        {
                           ++trailing_zero_op1;
                        }
                     }
                  }
                  if(trailing_zero_op0 != 0 || trailing_zero_op1 != 0)
                  {
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "-->Bit Value Opt: mul_node/widen_mul_node optimized, nbits = " +
                                        STR(trailing_zero_op0 + trailing_zero_op1));
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                     if(trailing_zero_op0 != 0)
                     {
                        const auto op0_type = ir_helper::CGetType(op0);
                        const auto op0_const_node =
                            TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero_op0), op0_type);
                        const auto op0_expr =
                            IRman->create_binary_operation(op0_type, op0, op0_const_node, loc_info_default, shr_node_K);
                        const auto op0_ga = IRman->CreateAssignStmt(op0_type, nullptr, nullptr, op0_expr, function_id,
                                                                    loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                        B->PushBefore(op0_ga, stmt, AppM);
                        const auto op0_ga_var = GetPointerS<const assign_stmt>(op0_ga)->op0;
                        TM->ReplaceIRNode(stmt, op0, op0_ga_var);
                        /// set the bit_values to the ssa var
                        if(is_op0_ssa)
                        {
                           auto op0_ssa = GetPointerS<ssa_node>(op0_ga_var);
                           THROW_ASSERT(bit_values_op0.size() - trailing_zero_op0 > 0, "unexpected condition");
                           op0_ssa->bit_values = bit_values_op0.substr(0, bit_values_op0.size() - trailing_zero_op0);
                           constrainSSA(op0_ssa, TM);
                        }
                     }
                     if(trailing_zero_op1 != 0 && !squareP)
                     {
                        const auto op1_type = ir_helper::CGetType(op1);
                        const auto op1_const_node =
                            TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero_op1), op1_type);
                        const auto op1_expr = IRman->create_binary_operation(op1_type, me->op1, op1_const_node,
                                                                             loc_info_default, shr_node_K);
                        const auto op1_ga = IRman->CreateAssignStmt(op1_type, ir_nodeRef(), ir_nodeRef(), op1_expr,
                                                                    function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                        B->PushBefore(op1_ga, stmt, AppM);
                        const auto op1_ga_var = GetPointerS<const assign_stmt>(op1_ga)->op0;
                        TM->ReplaceIRNode(stmt, me->op1, op1_ga_var);
                        /// set the bit_values to the ssa var
                        if(is_op1_ssa)
                        {
                           auto op1_ssa = GetPointerS<ssa_node>(op1_ga_var);
                           THROW_ASSERT(bit_values_op1.size() - trailing_zero_op1 > 0, "unexpected condition");
                           op1_ssa->bit_values = bit_values_op1.substr(0, bit_values_op1.size() - trailing_zero_op1);
                           constrainSSA(op1_ssa, TM);
                        }
                     }

                     const auto ssa_vd = IRman->create_ssa_name(nullptr, lhs_type, nullptr, nullptr);
                     auto sn = GetPointerS<ssa_node>(ssa_vd);
                     /// set the bit_values to the ssa var
                     THROW_ASSERT(ssa->bit_values.size() - trailing_zero_op0 - trailing_zero_op1 > 0,
                                  "unexpected condition");
                     sn->bit_values =
                         ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_zero_op0 - trailing_zero_op1);
                     constrainSSA(sn, TM);
                     const auto op_const_node =
                         TM->CreateUniqueIntegerCst((trailing_zero_op0 + trailing_zero_op1), lhs_type);
                     const auto op_expr =
                         IRman->create_binary_operation(lhs_type, ssa_vd, op_const_node, loc_info_default, shl_node_K);
                     const auto curr_ga =
                         IRman->CreateAssignStmt(lhs_type, ssa->min, ssa->max, op_expr, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                     TM->ReplaceIRNode(curr_ga, GetPointerS<const assign_stmt>(curr_ga)->op0, lhs_node);
                     TM->ReplaceIRNode(stmt, lhs_node, ssa_vd);
                     B->PushAfter(curr_ga, stmt, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "pushed");
                  }
               }
            };
            mul_node_BVO();
         }
         else if(rhs_kind == add_node_K || rhs_kind == sub_node_K)
         {
            const auto plus_minus_BVO = [&] {
               const auto me = GetPointerS<const binary_node>(rhs_node);
               bool identicalP = me->op0->index == me->op1->index;
               if(identicalP)
               {
                  if(rhs_kind == sub_node_K)
                  {
                     TM->ReplaceIRNode(stmt, rhs_node, TM->CreateUniqueIntegerCst(0LL, lhs_type));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Statement transformed in " + stmt->ToString());
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                  }
                  else
                  {
                     const auto op_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(1), lhs_type);
                     const auto op_expr =
                         IRman->create_binary_operation(lhs_type, me->op0, op_const_node, loc_info_default, shl_node_K);
                     TM->ReplaceIRNode(stmt, rhs_node, op_expr);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Statement transformed in " + stmt->ToString());
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                  }
                  return;
               }
               const auto& op0 = me->op0;
               const auto& op1 = me->op1;
               const auto isSsaOperand = [](const ir_nodeRef& op) { return op->get_kind() == ssa_node_K; };
               bool is_op0_ssa = isSsaOperand(op0);
               bool is_op1_ssa = isSsaOperand(op1);
               std::string bit_values_op0;
               std::string bit_values_op1;

               PRINT_DBG_MEX(
                   DEBUG_LEVEL_PEDANTIC, debug_level,
                   "Var_uid: " +
                       AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(lhs_node->index) +
                       " bitstring: " + bit_values);
               unsigned int trailing_zero_op0 = 0;
               unsigned int trailing_zero_op1 = 0;
               bool is_op0_null = false;
               bool is_op1_null = false;

               const auto computeTrailingZerosAndNull =
                   [&](const ir_nodeRef& op, const bool is_op_ssa, const std::string& DEBUG_PARAMETER(operand_name),
                       std::string& bit_values_op, unsigned int& trailing_zero_op, bool& is_op_null) -> bool {
                  if(is_op_ssa)
                  {
                     bit_values_op = GetPointerS<const ssa_node>(op)->bit_values;
                     if(bit_values_op.empty())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Skipped because bitvalue of " + operand_name + " is empty");
                        return false;
                     }
                     PRINT_DBG_MEX(
                         DEBUG_LEVEL_PEDANTIC, debug_level,
                         "Var_uid: " +
                             AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(op->index) +
                             " bitstring: " + bit_values_op);
                     for(const auto& current_el : boost::adaptors::reverse(bit_values_op))
                     {
                        if(current_el == '0')
                        {
                           ++trailing_zero_op;
                        }
                        else
                        {
                           break;
                        }
                     }
                     if(trailing_zero_op >= bit_values.size())
                     {
                        is_op_null = true;
                     }
                     return true;
                  }

                  THROW_ASSERT(op->get_kind() == constant_int_val_node_K, "unexpected case");
                  const auto cst_val = ir_helper::GetConstValue(op, false);
                  bit_values_op = convert_to_binary(cst_val, ir_helper::Size(op));
                  if(cst_val == 0)
                  {
                     is_op_null = true;
                     return true;
                  }

                  for(unsigned int index = 0; index < bit_values_op.size(); ++index)
                  {
                     if(cst_val & (integer_cst_t(1) << index))
                     {
                        break;
                     }
                     ++trailing_zero_op;
                  }
                  if(trailing_zero_op >= bit_values.size())
                  {
                     is_op_null = true;
                  }
                  return true;
               };

               if(!computeTrailingZerosAndNull(op0, is_op0_ssa, "op0", bit_values_op0, trailing_zero_op0, is_op0_null))
               {
                  return;
               }
               if(!computeTrailingZerosAndNull(op1, is_op1_ssa, "op1", bit_values_op1, trailing_zero_op1, is_op1_null))
               {
                  return;
               }

               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---Trailing zeros op0=" + STR(trailing_zero_op0) +
                                  ", trailing zeros op1=" + STR(trailing_zero_op1));
               if(is_op0_null)
               {
                  if(rhs_kind == add_node_K)
                  {
                     condPropageValue(me->op1, DEBUG_CALLSITE);
                  }
                  else
                  {
                     const auto res_expr =
                         IRman->create_unary_operation(lhs_type, me->op1, loc_info_default, neg_node_K);
                     TM->ReplaceIRNode(stmt, rhs_node, res_expr);
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                  }
               }
               else if(is_op1_null)
               {
                  condPropageValue(me->op0, DEBUG_CALLSITE);
               }
               else if(trailing_zero_op0 != 0 || trailing_zero_op1 != 0)
               {
                  const auto is_first_max = trailing_zero_op0 > trailing_zero_op1;

                  auto shift_const = rhs_kind == add_node_K ? (is_first_max ? trailing_zero_op0 : trailing_zero_op1) :
                                                              (is_first_max ? trailing_zero_op1 : trailing_zero_op0);

                  if(shift_const > 0)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "---Bit Value Opt: " + ir_node::GetString(rhs_kind) +
                                        " optimized, nbits = " + STR(shift_const));
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                     const auto shift_constant_node =
                         TM->CreateUniqueIntegerCst(static_cast<long long int>(shift_const), lhs_type);
                     const auto op0_type = ir_helper::CGetType(op0);
                     const auto op1_type = ir_helper::CGetType(op1);
                     const auto b_node =
                         rhs_kind == add_node_K ? (is_first_max ? op1 : op0) : (is_first_max ? op0 : op1);
                     const auto b_type = rhs_kind == add_node_K ? (is_first_max ? op1_type : op0_type) :
                                                                  (is_first_max ? op0_type : op1_type);
                     const auto markSsaToSkipIfAddress = [&](const ir_nodeRef& ssa_var) {
#if HAVE_FROM_DISCREPANCY_BUILT
                        /*
                         * for discrepancy analysis, the ssa assigned by this
                         * bitshift must not be checked if it was applied to
                         * a variable marked as address.
                         */
                        if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                        {
                           AppM->RDiscr->ssa_to_skip_if_address.insert(ssa_var);
                        }
#endif
                     };

                     const auto applyShiftToOperand = [&](const ir_nodeRef& operand,
                                                          const ir_nodeConstRef& operand_type,
                                                          const bool is_operand_ssa,
                                                          const std::string& operand_bit_values,
                                                          bool& is_operand_null) {
                        if(!is_operand_ssa)
                        {
                           const auto cst_val =
                               ir_helper::GetConstValue(operand, ir_helper::IsSignedIntegerType(operand_type));
                           if(ssa->bit_values.size() <= shift_const)
                           {
                              is_operand_null = true;
                           }
                           else
                           {
                              if((cst_val >> shift_const) == 0)
                              {
                                 is_operand_null = true;
                              }
                              TM->ReplaceIRNode(stmt, operand,
                                                TM->CreateUniqueIntegerCst(cst_val >> shift_const, operand_type));
                           }
                           return;
                        }

                        std::string resulting_bit_values;
                        if((operand_bit_values.size() - shift_const) > 0)
                        {
                           resulting_bit_values = operand_bit_values.substr(0, operand_bit_values.size() - shift_const);
                        }
                        else if(ir_helper::IsSignedIntegerType(operand_type))
                        {
                           resulting_bit_values = operand_bit_values.substr(0, 1);
                        }
                        else
                        {
                           resulting_bit_values = "0";
                        }

                        if(resulting_bit_values.find_first_not_of("0X") == std::string::npos)
                        {
                           is_operand_null = true;
                           return;
                        }

                        const auto op_expr = IRman->create_binary_operation(operand_type, operand, shift_constant_node,
                                                                            loc_info_default, shr_node_K);
                        const auto op_ga = IRman->CreateAssignStmt(operand_type, nullptr, nullptr, op_expr, function_id,
                                                                   loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op_ga));
                        B->PushBefore(op_ga, stmt, AppM);
                        const auto op_ga_var = GetPointerS<const assign_stmt>(op_ga)->op0;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Replacing " + operand->ToString() + " with " + op_ga_var->ToString() +
                                           " in " + stmt->ToString());
                        TM->ReplaceIRNode(stmt, operand, op_ga_var);
                        markSsaToSkipIfAddress(op_ga_var);
                        /// set the bit_values to the ssa var
                        auto* op_ssa = GetPointerS<ssa_node>(op_ga_var);
                        THROW_ASSERT(resulting_bit_values.size() > 0, "unexpected condition");
                        op_ssa->bit_values = resulting_bit_values;
                        constrainSSA(op_ssa, TM);
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                      "Var_uid: " +
                                          AppM->CGetFunctionBehavior(function_id)
                                              ->CGetBehavioralHelper()
                                              ->PrintVariable(op_ga_var->index) +
                                          " bitstring: " + STR(op_ssa->bit_values));
                     };

                     applyShiftToOperand(me->op0, op0_type, is_op0_ssa, bit_values_op0, is_op0_null);
                     applyShiftToOperand(me->op1, op1_type, is_op1_ssa, bit_values_op1, is_op1_null);

                     ir_nodeRef curr_expr = rhs_node;
                     if(is_op0_null)
                     {
                        curr_expr = rhs_kind == add_node_K ?
                                        me->op1 :
                                        IRman->create_unary_operation(lhs_type, me->op1, loc_info_default, neg_node_K);
                     }
                     else if(is_op1_null)
                     {
                        curr_expr = me->op0;
                     }
                     const auto curr_ga =
                         IRman->CreateAssignStmt(op1_type, nullptr, nullptr, curr_expr, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                     B->PushBefore(curr_ga, stmt, AppM);
                     const auto curr_ga_var = GetPointerS<const assign_stmt>(curr_ga)->op0;
                     markSsaToSkipIfAddress(curr_ga_var);
                     /// set the bit_values to the ssa var
                     auto op_ssa = GetPointerS<ssa_node>(curr_ga_var);
                     THROW_ASSERT(ssa->bit_values.size() - shift_const > 0, "unexpected condition");
                     op_ssa->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - shift_const);
                     constrainSSA(op_ssa, TM);
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "Var_uid: " +
                                       AppM->CGetFunctionBehavior(function_id)
                                           ->CGetBehavioralHelper()
                                           ->PrintVariable(curr_ga_var->index) +
                                       " bitstring: " + STR(op_ssa->bit_values));

                     const auto op_expr = IRman->create_binary_operation(lhs_type, curr_ga_var, shift_constant_node,
                                                                         loc_info_default, shl_node_K);
                     const auto lshift_ga =
                         IRman->CreateAssignStmt(lhs_type, nullptr, nullptr, op_expr, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(lshift_ga));
                     B->PushBefore(lshift_ga, stmt, AppM);
                     const auto lshift_ga_var = GetPointerS<const assign_stmt>(lshift_ga)->op0;
                     /// set the bit_values to the ssa var
                     auto lshift_ssa = GetPointerS<ssa_node>(lshift_ga_var);
                     THROW_ASSERT(ssa->bit_values.size() - shift_const > 0, "unexpected condition");
                     lshift_ssa->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - shift_const);
                     while(lshift_ssa->bit_values.size() < ssa->bit_values.size())
                     {
                        lshift_ssa->bit_values.push_back('0');
                     }
                     constrainSSA(lshift_ssa, TM);
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "Var_uid: " +
                                       AppM->CGetFunctionBehavior(function_id)
                                           ->CGetBehavioralHelper()
                                           ->PrintVariable(lshift_ga_var->index) +
                                       " bitstring: " + STR(lshift_ssa->bit_values));

                     bool do_final_or = false;
                     unsigned int n_iter = 0;
                     for(const auto& cur_bit : boost::adaptors::reverse(ssa->bit_values))
                     {
                        if(cur_bit == '1' || cur_bit == 'U')
                        {
                           do_final_or = true;
                           break;
                        }
                        n_iter++;
                        if(n_iter == shift_const)
                        {
                           break;
                        }
                     }

                     if(do_final_or)
                     {
                        markSsaToSkipIfAddress(lshift_ga_var);
                        if(b_node->get_kind() == constant_int_val_node_K)
                        {
                           const auto cst_val = ir_helper::GetConstValue(b_node, false);
                           const auto b_node_val =
                               TM->CreateUniqueIntegerCst(cst_val & ((integer_cst_t(1) << shift_const) - 1), b_type);
                           TM->ReplaceIRNode(stmt, rhs_node,
                                             IRman->create_ternary_operation(lhs_type, lshift_ga_var, b_node_val,
                                                                             shift_constant_node, loc_info_default,
                                                                             concat_bit_node_K));
                        }
                        else
                        {
                           const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(
                               static_cast<long long int>((1ULL << shift_const) - 1), b_type);
                           const auto band_expr = IRman->create_binary_operation(b_type, b_node, bit_mask_constant_node,
                                                                                 loc_info_default, and_node_K);
                           const auto band_ga = IRman->CreateAssignStmt(b_type, nullptr, nullptr, band_expr,
                                                                        function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(band_ga));
                           B->PushBefore(band_ga, stmt, AppM);
                           const auto band_ga_var = GetPointerS<const assign_stmt>(band_ga)->op0;
                           markSsaToSkipIfAddress(band_ga_var);
                           /// set the bit_values to the ssa var
                           auto band_ssa = GetPointerS<ssa_node>(band_ga_var);
                           for(const auto& cur_bit : boost::adaptors::reverse(ssa->bit_values))
                           {
                              band_ssa->bit_values = cur_bit + band_ssa->bit_values;
                              if(band_ssa->bit_values.size() == shift_const)
                              {
                                 break;
                              }
                           }
                           band_ssa->bit_values = "0" + band_ssa->bit_values;
                           constrainSSA(band_ssa, TM);
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                         "Var_uid: " +
                                             AppM->CGetFunctionBehavior(function_id)
                                                 ->CGetBehavioralHelper()
                                                 ->PrintVariable(band_ga_var->index) +
                                             " bitstring: " + STR(band_ssa->bit_values));

                           const auto res_expr = IRman->create_ternary_operation(lhs_type, lshift_ga_var, band_ga_var,
                                                                                 shift_constant_node, loc_info_default,
                                                                                 concat_bit_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, res_expr);
                        }
                     }
                     else
                     {
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Final or not performed: ");
                        TM->ReplaceIRNode(stmt, rhs_node, lshift_ga_var);
                     }
                  }
               }
            };
            plus_minus_BVO();
         }
         else if(rhs_kind == eq_node_K || rhs_kind == ne_node_K)
         {
            const auto eq_ne_node_BVO = [&] {
               const auto me = GetPointerS<const binary_node>(rhs_node);
               const auto& op0 = me->op0;
               const auto& op1 = me->op1;
               if(ir_helper::IsRealType(op0) && ir_helper::IsRealType(op1))
               {
                  // TODO: adapt existing operations to real type (zero sign bug to be considered)
                  return;
               }
               const auto op0_size = ir_helper::TypeSize(op0);
               bool is_op1_zero = false;
               if(op1->get_kind() == constant_int_val_node_K)
               {
                  if(ir_helper::GetConstValue(op1) == 0)
                  {
                     is_op1_zero = true;
                  }
               }

               if(op0->index == op1->index)
               {
                  const auto const_value = rhs_kind == eq_node_K ? 1LL : 0LL;
                  const auto val = TM->CreateUniqueIntegerCst(const_value, lhs_type);
                  propagateCurrValue(val, DEBUG_CALLSITE);
               }
               else if(is_op1_zero && rhs_kind == ne_node_K && op0_size == 1)
               {
                  const auto op0_op_type = ir_helper::CGetType(op0);
                  const auto data_bitsize = ir_helper::TypeSize(op0_op_type);
                  if(data_bitsize == 1)
                  {
                     propagateCurrValue(op0, DEBUG_CALLSITE);
                     if(!ssa->CGetUseStmts().empty())
                     {
                        if(AppM->ApplyNewTransformation())
                        {
                           const auto nop_node_expr =
                               IRman->create_unary_operation(lhs_type, op0, loc_info_default, nop_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, nop_node_expr);
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                     }
                  }
                  else
                  {
                     const auto one_const_node = TM->CreateUniqueIntegerCst(1, op0_op_type);
                     const auto bitwise_masked =
                         IRman->create_binary_operation(op0_op_type, op0, one_const_node, loc_info_default, and_node_K);
                     const auto op0_ga = IRman->CreateAssignStmt(
                         op0_op_type, TM->CreateUniqueIntegerCst(0, op0_op_type),
                         TM->CreateUniqueIntegerCst(1LL, op0_op_type), bitwise_masked, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                     B->PushBefore(op0_ga, stmt, AppM);
                     const auto op0_ga_var = GetPointerS<const assign_stmt>(op0_ga)->op0;

                     const auto ga_nop =
                         IRman->CreateNopExpr(op0_ga_var, lhs_type, ir_nodeRef(), ir_nodeRef(), function_id);
                     B->PushBefore(ga_nop, stmt, AppM);
                     modified = true;
                     AppM->RegisterTransformation(GetName(), ga_nop);
                     const auto nop_ga_var = GetPointerS<const assign_stmt>(ga_nop)->op0;
                     TM->ReplaceIRNode(stmt, rhs_node, nop_ga_var);
                  }
               }
               else
               {
                  rel_expr_BVO();
               }
            };
            eq_ne_node_BVO();
         }
         else if(rhs_kind == lt_node_K || rhs_kind == gt_node_K || rhs_kind == le_node_K || rhs_kind == ge_node_K)
         {
            rel_expr_BVO();
         }
         else if(rhs_kind == and_node_K || rhs_kind == xor_node_K)
         {
            const auto bit_expr_BVO = [&] {
               const auto me = GetPointerS<const binary_node>(rhs_node);
               const auto& op0 = me->op0;
               const auto& op1 = me->op1;

               std::string s0, s1;
               bool is_zero0 = false, is_zero1 = false;
               const auto isSsaOperand = [](const ir_nodeRef& op) { return op->get_kind() == ssa_node_K; };
               const bool is_op0_ssa = isSsaOperand(op0);
               const bool is_op1_ssa = isSsaOperand(op1);
               const auto computeBitExprOperandInfo = [&](const ir_nodeRef& op, const bool is_op_ssa,
                                                          const std::string& DEBUG_PARAMETER(operand_name),
                                                          std::string& operand_bits, bool& is_zero) -> bool {
                  if(is_op_ssa)
                  {
                     operand_bits = GetPointerS<const ssa_node>(op)->bit_values;
                     if(operand_bits.empty())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Skipped because bitvalue of " + operand_name + " is empty");
                        return false;
                     }
                     is_zero = operand_bits.find_first_not_of("0X") == std::string::npos;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + operand_name + ": " + operand_bits);
                     return true;
                  }

                  THROW_ASSERT(op->get_kind() == constant_int_val_node_K, "unexpected case");
                  const auto uvalue = ir_helper::GetConstValue(op, false);
                  operand_bits = convert_to_binary(uvalue, ir_helper::TypeSize(op));
                  is_zero = uvalue == 0;
                  return true;
               };

               if(!computeBitExprOperandInfo(op0, is_op0_ssa, "s0", s0, is_zero0))
               {
                  return;
               }
               if(!computeBitExprOperandInfo(op1, is_op1_ssa, "s1", s1, is_zero1))
               {
                  return;
               }

               if(is_zero0 || is_zero1)
               {
                  if(rhs_kind == and_node_K)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---replace and_node usage before: " + stmt->ToString());
                     const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ir_helper::CGetType(op0));
                     propagateCurrValue(zero_node, DEBUG_CALLSITE);
                  }
                  else
                  {
                     // xor_node_K
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---replace xor_node usage before: " + stmt->ToString());
                     const auto val = is_zero0 ? op1 : op0;
                     condPropageValue(val, DEBUG_CALLSITE);
                  }
               }
               else
               {
                  unsigned int trailing_zero = 0;
                  for(auto s0it = s0.rbegin(), s1it = s1.rbegin(), s0end = s0.rend(), s1end = s1.rend();
                      s0it != s0end && s1it != s1end; ++s0it, ++s1it)
                  {
                     if((rhs_kind == and_node_K && (*s0it == '0' || *s1it == '0')) || *s0it == 'X' || *s1it == 'X')
                     {
                        ++trailing_zero;
                     }
                     else
                     {
                        break;
                     }
                  }
                  auto min_size = std::min(s0.size(), s1.size());
                  if(trailing_zero < min_size && trailing_zero)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "---Bit Value Opt: " + std::string(rhs_node->get_kind_text()) +
                                        " optimized, nbits = " + STR(trailing_zero));
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                     const auto op0_op_type = ir_helper::CGetType(op0);
                     const auto op1_op_type = ir_helper::CGetType(op1);

                     if(is_op0_ssa)
                     {
                        const auto op0_const_node = TM->CreateUniqueIntegerCst(trailing_zero, op0_op_type);
                        const auto op0_expr = IRman->create_binary_operation(op0_op_type, op0, op0_const_node,
                                                                             loc_info_default, shr_node_K);
                        const auto op0_ga = IRman->CreateAssignStmt(op0_op_type, nullptr, nullptr, op0_expr,
                                                                    function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                        B->PushBefore(op0_ga, stmt, AppM);
                        const auto op0_ga_var = GetPointerS<const assign_stmt>(op0_ga)->op0;
                        TM->ReplaceIRNode(stmt, op0, op0_ga_var);
                        /// set the bit_values to the ssa var
                        auto op0_ssa = GetPointerS<ssa_node>(op0_ga_var);
                        THROW_ASSERT(s0.size() - trailing_zero > 0, "unexpected condition");
                        op0_ssa->bit_values = s0.substr(0, s0.size() - trailing_zero);
                        constrainSSA(op0_ssa, TM);
                     }
                     else
                     {
                        const auto cst_val = ir_helper::GetConstValue(op0, ir_helper::IsSignedIntegerType(op0));
                        TM->ReplaceIRNode(stmt, op0, TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op0_op_type));
                     }
                     if(is_op1_ssa)
                     {
                        const auto op1_const_node =
                            TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), op1_op_type);
                        const auto op1_expr = IRman->create_binary_operation(op1_op_type, op1, op1_const_node,
                                                                             loc_info_default, shr_node_K);
                        const auto op1_ga = IRman->CreateAssignStmt(op1_op_type, nullptr, nullptr, op1_expr,
                                                                    function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                        B->PushBefore(op1_ga, stmt, AppM);
                        const auto op1_ga_var = GetPointerS<const assign_stmt>(op1_ga)->op0;
                        TM->ReplaceIRNode(stmt, op1, op1_ga_var);
                        /// set the bit_values to the ssa var
                        auto op1_ssa = GetPointerS<ssa_node>(op1_ga_var);
                        THROW_ASSERT(s1.size() - trailing_zero > 0, "unexpected condition");
                        op1_ssa->bit_values = s1.substr(0, s1.size() - trailing_zero);
                        constrainSSA(op1_ssa, TM);
                     }
                     else
                     {
                        const auto cst_val = ir_helper::GetConstValue(op1, ir_helper::IsSignedIntegerType(op1));
                        TM->ReplaceIRNode(stmt, op1, TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op1_op_type));
                     }

                     const auto ssa_vd = IRman->create_ssa_name(nullptr, lhs_type, nullptr, nullptr);
                     auto sn = GetPointerS<ssa_node>(ssa_vd);
                     /// set the bit_values to the ssa var
                     THROW_ASSERT(ssa->bit_values.size() - trailing_zero > 0, "unexpected condition");
                     sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_zero);
                     constrainSSA(sn, TM);
                     const auto op_const_node =
                         TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), lhs_type);
                     const auto op_expr =
                         IRman->create_binary_operation(lhs_type, ssa_vd, op_const_node, loc_info_default, shl_node_K);
                     const auto curr_ga =
                         IRman->CreateAssignStmt(lhs_type, ssa->min, ssa->max, op_expr, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                     TM->ReplaceIRNode(curr_ga, GetPointerS<const assign_stmt>(curr_ga)->op0, lhs_node);
                     TM->ReplaceIRNode(stmt, lhs_node, ssa_vd);
                     B->PushAfter(curr_ga, stmt, AppM);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "pushed");
                  }
               }
            };
            bit_expr_BVO();
         }
         else if(rhs_kind == select_node_K)
         {
            const auto select_node_BVO = [&] {
               const auto me = GetPointerS<const select_node>(rhs_node);
               if(!ir_helper::IsBooleanType(me->op0))
               {
                  /// try to fix select_node condition
                  if(me->op0->get_kind() == ssa_node_K)
                  {
                     const auto cond_op0_ssa = GetPointerS<const ssa_node>(me->op0);
                     const auto defStmt = cond_op0_ssa->GetDefStmt();
                     if(defStmt->get_kind() == assign_stmt_K)
                     {
                        const auto prev_ga = GetPointerS<const assign_stmt>(defStmt);
                        if(prev_ga->op1->get_kind() == nop_node_K)
                        {
                           const auto ne = GetPointerS<const nop_node>(prev_ga->op1);
                           if(ir_helper::IsBooleanType(ne->op))
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace select_node condition before: " + stmt->ToString());
                              TM->ReplaceIRNode(stmt, me->op0, ne->op);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace select_node condition after: " + stmt->ToString());
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                        }
                     }
                  }
               }
               if(!AppM->ApplyNewTransformation())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Skipped because reached limit of cfg transformations");
                  return;
               }
               const auto& op0 = me->op1;
               const auto& op1 = me->op2;
               const auto& condition = me->op0;
               if(op0->index == op1->index)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with equal operands");
                  condPropageValue(op0, DEBUG_CALLSITE);
               }
               else if(condition->get_kind() == constant_int_val_node_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with constant condition");
                  const auto val = ir_helper::GetConstValue(me->op0, false) ? op0 : op1;
                  condPropageValue(val, DEBUG_CALLSITE);
               }
               else
               {
                  std::string s0, s1;
                  const auto isSsaOperand = [](const ir_nodeRef& op) { return op->get_kind() == ssa_node_K; };
                  bool is_op0_ssa = isSsaOperand(op0);
                  bool is_op1_ssa = isSsaOperand(op1);
                  unsigned long long s0_precision = 0;
                  bool is_s0_null = false;
                  bool is_s0_one = false;
                  unsigned long long s1_precision = 0;
                  bool is_s1_null = false;
                  bool is_s1_one = false;
                  const auto computeCondOperandInfo = [&](const ir_nodeRef& op, const bool is_op_ssa,
                                                          const std::string& DEBUG_PARAMETER(operand_name),
                                                          std::string& operand_bits, unsigned long long& precision,
                                                          bool& is_null, bool& is_one) -> bool {
                     if(is_op_ssa)
                     {
                        operand_bits = GetPointerS<const ssa_node>(op)->bit_values;
                        if(operand_bits.empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Skipped because " + operand_name + " is empty");
                           return false;
                        }
                        precision = static_cast<unsigned int>(operand_bits.size());
                        is_null = operand_bits.find_first_not_of("0X") == std::string::npos;
                        is_one = operand_bits == "1" || operand_bits == "01";
                        return true;
                     }

                     THROW_ASSERT(op->get_kind() == constant_int_val_node_K, "unexpected condition");
                     const auto uvalue = ir_helper::GetConstValue(op, false);
                     precision = uvalue ? ir_helper::Size(op) : ir_helper::TypeSize(op);
                     operand_bits = convert_to_binary(uvalue, precision);
                     is_null = uvalue == 0;
                     is_one = uvalue == 1;
                     return true;
                  };

                  if(!computeCondOperandInfo(op0, is_op0_ssa, "s0", s0, s0_precision, is_s0_null, is_s0_one))
                  {
                     return;
                  }
                  if(!computeCondOperandInfo(op1, is_op1_ssa, "s1", s1, s1_precision, is_s1_null, is_s1_one))
                  {
                     return;
                  }

                  unsigned long long minimum_precision = std::min(s0_precision, s1_precision);
                  unsigned int trailing_eq = 0;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---Bit_value strings are " + s0 + " and " + s1);
                  for(unsigned int index = 0; index < (minimum_precision - 1); ++index)
                  {
                     if((s0[s0.size() - index - 1] == '0' || s0[s0.size() - index - 1] == 'X') &&
                        (s1[s1.size() - index - 1] == '0' || s1[s1.size() - index - 1] == 'X'))
                     {
                        ++trailing_eq;
                     }
                     else
                     {
                        break;
                     }
                  }
                  if(trailing_eq)
                  {
                     INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                    "---Bit Value Opt: select_node optimized, nbits = " + STR(trailing_eq));
                     modified = true;
                     const auto op0_op_type = ir_helper::CGetType(op0);
                     const auto op1_op_type = ir_helper::CGetType(op1);

                     if(is_op0_ssa)
                     {
                        const auto op0_const_node =
                            TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), op0_op_type);
                        const auto op0_expr = IRman->create_binary_operation(op0_op_type, op0, op0_const_node,
                                                                             loc_info_default, shr_node_K);
                        const auto op0_ga = IRman->CreateAssignStmt(op0_op_type, nullptr, nullptr, op0_expr,
                                                                    function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                        B->PushBefore(op0_ga, stmt, AppM);
                        const auto op0_ga_var = GetPointerS<const assign_stmt>(op0_ga)->op0;
                        TM->ReplaceIRNode(stmt, op0, op0_ga_var);
                        /// set the bit_values to the ssa var
                        auto op0_ssa = GetPointerS<ssa_node>(op0_ga_var);
                        THROW_ASSERT(s0.size() - trailing_eq > 0, "unexpected condition");
                        op0_ssa->bit_values = s0.substr(0, s0.size() - trailing_eq);
                        constrainSSA(op0_ssa, TM);
                     }
                     else
                     {
                        const auto cst_val = ir_helper::GetConstValue(op0, ir_helper::IsSignedIntegerType(op0));
                        TM->ReplaceIRNode(stmt, op0, TM->CreateUniqueIntegerCst(cst_val >> trailing_eq, op0_op_type));
                     }
                     if(is_op1_ssa)
                     {
                        const auto op1_const_node =
                            TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), op1_op_type);
                        const auto op1_expr = IRman->create_binary_operation(op1_op_type, op1, op1_const_node,
                                                                             loc_info_default, shr_node_K);
                        const auto op1_ga = IRman->CreateAssignStmt(op1_op_type, ir_nodeRef(), ir_nodeRef(), op1_expr,
                                                                    function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                        B->PushBefore(op1_ga, stmt, AppM);
                        const auto op1_ga_var = GetPointerS<const assign_stmt>(op1_ga)->op0;
                        TM->ReplaceIRNode(stmt, op1, op1_ga_var);
                        /// set the bit_values to the ssa var
                        auto op1_ssa = GetPointerS<ssa_node>(op1_ga_var);
                        THROW_ASSERT(s1.size() - trailing_eq > 0, "unexpected condition");
                        op1_ssa->bit_values = s1.substr(0, s1.size() - trailing_eq);
                        constrainSSA(op1_ssa, TM);
                     }
                     else
                     {
                        const auto cst_val = ir_helper::GetConstValue(op1, ir_helper::IsSignedIntegerType(op1));
                        TM->ReplaceIRNode(stmt, op1, TM->CreateUniqueIntegerCst(cst_val >> trailing_eq, op1_op_type));
                     }
                     const auto ssa_vd = IRman->create_ssa_name(nullptr, lhs_type, nullptr, nullptr);
                     auto sn = GetPointerS<ssa_node>(ssa_vd);
                     /// set the bit_values to the ssa var
                     if(ssa->bit_values.size())
                     {
                        THROW_ASSERT(ssa->bit_values.size() - trailing_eq > 0, "unexpected condition");
                        sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_eq);
                        constrainSSA(sn, TM);
                     }
                     const auto op_const_node =
                         TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), lhs_type);
                     const auto op_expr =
                         IRman->create_binary_operation(lhs_type, ssa_vd, op_const_node, loc_info_default, shl_node_K);
                     const auto curr_ga =
                         IRman->CreateAssignStmt(lhs_type, ssa->min, ssa->max, op_expr, function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                     TM->ReplaceIRNode(curr_ga, GetPointerS<const assign_stmt>(curr_ga)->op0, lhs_node);
                     TM->ReplaceIRNode(stmt, lhs_node, ssa_vd);
                     B->PushAfter(curr_ga, stmt, AppM);
                     modified = true;
                     AppM->RegisterTransformation(GetName(), stmt);
                  }
                  else if(is_s0_one && is_s1_null)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with true and false");
                     ir_nodeRef cond_var;
                     if(ir_helper::IsBooleanType(lhs_type))
                     {
                        cond_var = me->op0;
                     }
                     else
                     {
                        const auto ga_nop = IRman->CreateNopExpr(me->op0, lhs_type, ssa->min, ssa->max, function_id);
                        B->PushBefore(ga_nop, stmt, AppM);
                        modified = true;
                        AppM->RegisterTransformation(GetName(), ga_nop);
                        cond_var = GetPointerS<const assign_stmt>(ga_nop)->op0;
                     }
                     condPropageValue(cond_var, DEBUG_CALLSITE);
                  }
                  else if(is_s0_null && is_s1_one)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with false and true");
                     /// second argument is null since we cannot add the new statement at the end of the
                     /// current BB
                     const auto new_ssa = IRman->CreateNotExpr(me->op0, blocRef(), function_id);
                     const auto new_stmt = GetPointerS<const ssa_node>(new_ssa)->GetDefStmt();
                     B->PushBefore(new_stmt, stmt, AppM);
                     ir_nodeRef cond_var;
                     if(ir_helper::IsBooleanType(lhs_type))
                     {
                        cond_var = new_ssa;
                     }
                     else
                     {
                        const auto ga_nop = IRman->CreateNopExpr(new_ssa, lhs_type, ssa->min, ssa->max, function_id);
                        B->PushBefore(ga_nop, stmt, AppM);
                        cond_var = GetPointerS<const assign_stmt>(ga_nop)->op0;
                     }
                     condPropageValue(cond_var, DEBUG_CALLSITE);
                  }
               }
            };
            select_node_BVO();
         }
         else if(rhs_kind == or_node_K)
         {
            const auto bie = GetPointerS<const or_node>(rhs_node);
            if(bie->op0->get_kind() == constant_int_val_node_K || bie->op1->get_kind() == constant_int_val_node_K)
            {
               ir_nodeRef val;
               if(bie->op0->get_kind() == constant_int_val_node_K)
               {
                  const auto cst_val = ir_helper::GetConstValue(bie->op0);
                  if(cst_val == 0)
                  {
                     val = bie->op1;
                  }
               }
               else
               {
                  const auto cst_val = ir_helper::GetConstValue(bie->op1);
                  if(cst_val == 0)
                  {
                     val = bie->op0;
                  }
               }
               if(val)
               {
                  condPropageValue(val, DEBUG_CALLSITE);
               }
            }
         }
         else if(rhs_kind == gep_node_K)
         {
            const auto ppe = GetPointerS<const gep_node>(rhs_node);
            if(ppe->op1->get_kind() == constant_int_val_node_K)
            {
               const auto cst_val = ir_helper::GetConstValue(ppe->op1);
               if(cst_val == 0)
               {
                  condPropageValue(ppe->op0, DEBUG_CALLSITE);
               }
               else if(ppe->op0->get_kind() == ssa_node_K)
               {
                  const auto temp_def = GetPointerS<const ssa_node>(ppe->op0)->GetDefStmt();
                  if(temp_def->get_kind() == assign_stmt_K)
                  {
                     const auto prev_ga = GetPointerS<const assign_stmt>(temp_def);
                     if(prev_ga->op1->get_kind() == gep_node_K)
                     {
                        const auto prev_ppe = GetPointerS<const gep_node>(prev_ga->op1);
                        if(prev_ppe->op0->get_kind() == ssa_node_K &&
                           prev_ppe->op1->get_kind() == constant_int_val_node_K)
                        {
                           const auto prev_val = ir_helper::GetConstValue(prev_ppe->op1);
                           const auto new_offset =
                               TM->CreateUniqueIntegerCst((prev_val + cst_val), ir_helper::CGetType(ppe->op1));
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace constant usage before: " + stmt->ToString());
                           TM->ReplaceIRNode(stmt, ppe->op1, new_offset);
                           TM->ReplaceIRNode(stmt, ppe->op0, prev_ppe->op0);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace constant usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                     }
                  }
               }
            }
         }
         else if(rhs_kind == addr_node_K)
         {
            const auto ae = GetPointerS<const addr_node>(rhs_node);
            if(ae->op->get_kind() == mem_access_node_K)
            {
               const auto mr = GetPointerS<const mem_access_node>(ae->op);
               if(mr->op->get_kind() == ssa_node_K)
               {
                  const auto temp_def = GetPointerS<const ssa_node>(mr->op)->GetDefStmt();
                  if(temp_def->get_kind() == assign_stmt_K)
                  {
                     const auto prev_ga = GetPointerS<const assign_stmt>(temp_def);
                     if(prev_ga->op1->get_kind() == addr_node_K)
                     {
                        condPropageValue(prev_ga->op0, DEBUG_CALLSITE);
                     }
                  }
               }
               else if(mr->op->get_kind() == constant_int_val_node_K)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace constant usage before: " + stmt->ToString());
                  TM->ReplaceIRNode(stmt, rhs_node, mr->op);
                  modified = true;
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---replace constant usage after: " + stmt->ToString());
               }
            }
         }
         else if(rhs_kind == extract_bit_node_K)
         {
            const auto extract_bit_node_BVO = [&] {
               const auto ebe = GetPointerS<const extract_bit_node>(rhs_node);
               THROW_ASSERT(ebe->op1->get_kind() == constant_int_val_node_K, "unexpected condition");
               const auto pos_value = ir_helper::GetConstValue(ebe->op1);
               THROW_ASSERT(pos_value >= 0, "");
               if(ebe->op0->get_kind() == ssa_node_K)
               {
                  const auto ebe_op0_ssa = GetPointerS<const ssa_node>(ebe->op0);
                  const auto ebe_op0_size = static_cast<integer_cst_t>(ir_helper::TypeSize(ebe->op0));
                  if(ebe_op0_size <= pos_value)
                  {
                     const bool right_signed = ir_helper::IsSignedIntegerType(ebe->op0);
                     if(right_signed)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---replace extract_bit_node usage before: " + stmt->ToString());
                        const auto new_pos =
                            TM->CreateUniqueIntegerCst(ebe_op0_size - 1, ir_helper::CGetType(ebe->op1));
                        const auto eb_op = IRman->create_extract_bit_node(ebe->op0, new_pos, loc_info_default);
                        const auto eb_ga = IRman->CreateAssignStmt(ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                                   TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op,
                                                                   function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                        B->PushBefore(eb_ga, stmt, AppM);
                        const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                        const auto masking =
                            IRman->create_binary_operation(ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0,
                                                           bit_mask_constant_node, loc_info_default, and_node_K);
                        TM->ReplaceIRNode(stmt, rhs_node,
                                          masking); /// replaced with redundant code to restart bit-value analysis
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---replace extract_bit_node usage after: " + stmt->ToString());
                        modified = true;
                        AppM->RegisterTransformation(GetName(), stmt);
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---replace extract_bit_node usage before: " + stmt->ToString());
                        const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                        propagateCurrValue(zero_node, DEBUG_CALLSITE);
                     }
                  }
                  else if(ir_helper::IsBooleanType(ebe->op0))
                  {
                     propagateCurrValue(ebe->op0, DEBUG_CALLSITE);
                  }
                  else
                  {
                     const auto defStmt = ebe_op0_ssa->GetDefStmt();
                     if(defStmt->get_kind() == assign_stmt_K)
                     {
                        const auto prev_ga = GetPointerS<const assign_stmt>(defStmt);
                        const auto prev_code1 = prev_ga->op1->get_kind();
                        if(prev_code1 == nop_node_K)
                        {
                           const auto ne = GetPointerS<const unary_node>(prev_ga->op1);
                           if(ir_helper::IsBooleanType(ne->op))
                           {
                              if(pos_value == 0)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1LL, ebe->type);
                                 const auto masking = IRman->create_binary_operation(
                                     ebe->type, ne->op, bit_mask_constant_node, loc_info_default, and_node_K);
                                 TM->ReplaceIRNode(
                                     stmt, rhs_node,
                                     masking); /// replaced with redundant code to restart bit-value analysis
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage after: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                              else
                              {
                                 const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                 propagateCurrValue(zero_node, DEBUG_CALLSITE);
                              }
                           }
                           else
                           {
                              const auto neType_node = ir_helper::CGetType(ne->op);
                              if(neType_node->get_kind() == integer_ty_node_K)
                              {
                                 if(static_cast<integer_cst_t>(ir_helper::TypeSize(ne->op)) > pos_value)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_node usage before: " + stmt->ToString());
                                    const auto eb_op =
                                        IRman->create_extract_bit_node(ne->op, ebe->op1, loc_info_default);
                                    const auto eb_ga =
                                        IRman->CreateAssignStmt(ebe->type, TM->CreateUniqueIntegerCst(0LL, ebe->type),
                                                                TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op,
                                                                function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt, AppM);
                                    const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1LL, ebe->type);
                                    const auto masking = IRman->create_binary_operation(
                                        ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0, bit_mask_constant_node,
                                        loc_info_default, and_node_K);
                                    TM->ReplaceIRNode(
                                        stmt, rhs_node,
                                        masking); /// replaced with redundant code to restart bit-value analysis
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_node usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else
                                 {
                                    const bool right_signed = ir_helper::IsSignedIntegerType(ne->op);
                                    if(right_signed)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_node usage before: " + stmt->ToString());
                                       const auto new_pos = TM->CreateUniqueIntegerCst(
                                           static_cast<integer_cst_t>(ir_helper::TypeSize(ne->op) - 1),
                                           ir_helper::CGetType(ebe->op1));
                                       const auto eb_op =
                                           IRman->create_extract_bit_node(ne->op, new_pos, loc_info_default);
                                       const auto eb_ga = IRman->CreateAssignStmt(
                                           ebe->type, TM->CreateUniqueIntegerCst(0LL, ebe->type),
                                           TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op, function_id,
                                           loc_info_default);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---Created " + STR(eb_ga));
                                       B->PushBefore(eb_ga, stmt, AppM);
                                       const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1LL, ebe->type);
                                       const auto masking = IRman->create_binary_operation(
                                           ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0,
                                           bit_mask_constant_node, loc_info_default, and_node_K);
                                       TM->ReplaceIRNode(stmt, rhs_node,
                                                         masking); /// replaced with redundant code to
                                                                   /// restart bit-value analysis
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_node usage after: " + stmt->ToString());
                                       modified = true;
                                       AppM->RegisterTransformation(GetName(), stmt);
                                    }
                                    else
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_node usage before: " + stmt->ToString());
                                       const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                       propagateCurrValue(zero_node, DEBUG_CALLSITE);
                                    }
                                 }
                              }
                           }
                        }
                        else if(prev_code1 == and_node_K)
                        {
                           const auto bae = GetPointerS<const and_node>(prev_ga->op1);
                           if(bae->op0->get_kind() == constant_int_val_node_K ||
                              bae->op1->get_kind() == constant_int_val_node_K)
                           {
                              auto bae_op0 = bae->op0;
                              auto bae_op1 = bae->op1;
                              if(bae->op0->get_kind() == constant_int_val_node_K)
                              {
                                 std::swap(bae_op0, bae_op1);
                              }
                              const auto bae_mask_value = ir_helper::GetConstValue(bae_op1);
                              const auto masked_value = (bae_mask_value & (integer_cst_t(1) << pos_value));
                              if(masked_value && bae_op0->get_kind() != constant_int_val_node_K)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 const auto eb_op = IRman->create_extract_bit_node(bae_op0, ebe->op1, loc_info_default);
                                 const auto eb_ga = IRman->CreateAssignStmt(
                                     ebe->type, TM->CreateUniqueIntegerCst(0LL, ebe->type),
                                     TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op, function_id, loc_info_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                 B->PushBefore(eb_ga, stmt, AppM);
                                 const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                 const auto masking = IRman->create_binary_operation(
                                     ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0, bit_mask_constant_node,
                                     loc_info_default, and_node_K);
                                 TM->ReplaceIRNode(
                                     stmt, rhs_node,
                                     masking); /// replaced with redundant code to restart bit-value analysis
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage after: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 const auto zero_node = TM->CreateUniqueIntegerCst(masked_value ? 1 : 0, ebe->type);
                                 propagateCurrValue(zero_node, DEBUG_CALLSITE);
                              }
                           }
                        }
                        else if(prev_code1 == concat_bit_node_K)
                        {
                           const auto bice = GetPointerS<const concat_bit_node>(prev_ga->op1);
                           THROW_ASSERT(bice->op2->get_kind() == constant_int_val_node_K, "unexpected condition");
                           const auto nbit_value = ir_helper::GetConstValue(bice->op2);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage before: " + stmt->ToString());
                           const auto eb_op = IRman->create_extract_bit_node(
                               nbit_value > pos_value ? bice->op1 : bice->op0, ebe->op1, loc_info_default);
                           const auto eb_ga = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                           B->PushBefore(eb_ga, stmt, AppM);
                           const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                           const auto masking =
                               IRman->create_binary_operation(ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0,
                                                              bit_mask_constant_node, loc_info_default, and_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node,
                                             masking); /// replaced with redundant code to restart bit-value analysis
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                        else if(prev_code1 == shl_node_K)
                        {
                           const auto lse = GetPointerS<const shl_node>(prev_ga->op1);
                           if(lse->op1->get_kind() == constant_int_val_node_K)
                           {
                              const auto lsbit_value = ir_helper::GetConstValue(lse->op1);
                              if((pos_value - lsbit_value) >= 0)
                              {
                                 const auto new_pos =
                                     TM->CreateUniqueIntegerCst(pos_value - lsbit_value, ir_helper::CGetType(ebe->op1));
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 const auto eb_op = IRman->create_extract_bit_node(lse->op0, new_pos, loc_info_default);
                                 const auto eb_ga = IRman->CreateAssignStmt(
                                     ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                     TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, loc_info_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                 B->PushBefore(eb_ga, stmt, AppM);
                                 const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                 const auto masking = IRman->create_binary_operation(
                                     ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0, bit_mask_constant_node,
                                     loc_info_default, and_node_K);
                                 TM->ReplaceIRNode(
                                     stmt, rhs_node,
                                     masking); /// replaced with redundant code to restart bit-value analysis
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage after: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                 propagateCurrValue(zero_node, DEBUG_CALLSITE);
                              }
                           }
                        }
                        else if(prev_code1 == shr_node_K)
                        {
                           const auto rse = GetPointerS<const shr_node>(prev_ga->op1);
                           if(rse->op1->get_kind() == constant_int_val_node_K)
                           {
                              const auto rsbit_value = ir_helper::GetConstValue(rse->op1);
                              THROW_ASSERT((pos_value + rsbit_value) >= 0, "unexpected condition");
                              const auto new_pos =
                                  TM->CreateUniqueIntegerCst(pos_value + rsbit_value, ir_helper::CGetType(ebe->op1));
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace extract_bit_node usage before: " + stmt->ToString());
                              const auto eb_op = IRman->create_extract_bit_node(rse->op0, new_pos, loc_info_default);
                              const auto eb_ga = IRman->CreateAssignStmt(
                                  ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                  TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, loc_info_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                              B->PushBefore(eb_ga, stmt, AppM);
                              const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                              const auto masking =
                                  IRman->create_binary_operation(ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0,
                                                                 bit_mask_constant_node, loc_info_default, and_node_K);
                              TM->ReplaceIRNode(stmt, rhs_node,
                                                masking); /// replaced with redundant code to restart bit-value analysis
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace extract_bit_node usage after: " + stmt->ToString());
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                           else if(rse->op0->get_kind() == constant_int_val_node_K)
                           {
                              const auto res_value =
                                  ir_helper::GetConstValue(rse->op0, ir_helper::IsSignedIntegerType(rse->op0)) >>
                                  pos_value;
                              if(res_value)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 const auto precision = ir_helper::TypeSize(ebe_op0_ssa->type);
                                 unsigned int log2;
                                 for(log2 = 1; precision > (1u << log2); ++log2)
                                 {
                                    ;
                                 }
                                 ir_nodeRef op1, op2, op3, op4, op5, op6, op7, op8;
                                 for(auto i = 0u; i < log2; ++i)
                                 {
                                    const auto new_pos = TM->CreateUniqueIntegerCst(i, ir_helper::CGetType(ebe->op1));
                                    const auto eb_op =
                                        IRman->create_extract_bit_node(rse->op1, new_pos, loc_info_default);
                                    const auto eb_ga = IRman->CreateAssignStmt(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt, AppM);
                                    const auto eb_ga_ssa_var = GetPointerS<const assign_stmt>(eb_ga)->op0;
                                    if(i == 0)
                                    {
                                       op1 = eb_ga_ssa_var;
                                    }
                                    else if(i == 1)
                                    {
                                       op2 = eb_ga_ssa_var;
                                    }
                                    else if(i == 2)
                                    {
                                       op3 = eb_ga_ssa_var;
                                    }
                                    else if(i == 3)
                                    {
                                       op4 = eb_ga_ssa_var;
                                    }
                                    else if(i == 4)
                                    {
                                       op5 = eb_ga_ssa_var;
                                    }
                                    else if(i == 5)
                                    {
                                       op6 = eb_ga_ssa_var;
                                    }
                                    else
                                    {
                                       THROW_ERROR("unexpected condition");
                                    }
                                 }
                                 const auto LutConstType = IRman->GetUnsignedLongLongType();

                                 const auto lut_constant_node = TM->CreateUniqueIntegerCst(res_value, LutConstType);
                                 const auto eb_op = IRman->create_lut_node(ebe->type, lut_constant_node, op1, op2, op3,
                                                                           op4, op5, op6, op7, op8, loc_info_default);
                                 const auto eb_ga = IRman->CreateAssignStmt(
                                     ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                     TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, loc_info_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                 B->PushBefore(eb_ga, stmt, AppM);
                                 const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                 const auto masking = IRman->create_binary_operation(
                                     ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0, bit_mask_constant_node,
                                     loc_info_default, and_node_K);
                                 TM->ReplaceIRNode(stmt, rhs_node, masking); /// replaced with redundant code to
                                                                             /// restart bit-value analysis
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage after: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                              else
                              {
                                 const auto zero_node = TM->CreateUniqueIntegerCst(0, ebe->type);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage before: " + stmt->ToString());
                                 TM->ReplaceIRNode(stmt, rhs_node, zero_node);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_node usage after: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                           }
                        }
                        else if(prev_code1 == not_node_K)
                        {
                           const auto bne = GetPointerS<const not_node>(prev_ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage before: " + stmt->ToString());
                           const auto eb_op = IRman->create_extract_bit_node(bne->op, ebe->op1, loc_info_default);
                           const auto eb_ga = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                           B->PushBefore(eb_ga, stmt, AppM);
                           const auto negating = IRman->create_unary_operation(
                               ebe->type, GetPointerS<const assign_stmt>(eb_ga)->op0, loc_info_default, not_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, negating);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                        else if(prev_code1 == and_node_K)
                        {
                           const auto bae = GetPointerS<const and_node>(prev_ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage before: " + stmt->ToString());
                           const auto eb_op0 = IRman->create_extract_bit_node(bae->op0, ebe->op1, loc_info_default);
                           const auto eb_ga0 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op0, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                           B->PushBefore(eb_ga0, stmt, AppM);
                           const auto eb_op1 = IRman->create_extract_bit_node(bae->op1, ebe->op1, loc_info_default);
                           const auto eb_ga1 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                           B->PushBefore(eb_ga1, stmt, AppM);
                           const auto anding = IRman->create_binary_operation(
                               ebe->type, GetPointerS<const assign_stmt>(eb_ga0)->op0,
                               GetPointerS<const assign_stmt>(eb_ga1)->op0, loc_info_default, and_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, anding);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                        else if(prev_code1 == or_node_K)
                        {
                           const auto bie = GetPointerS<const or_node>(prev_ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage before: " + stmt->ToString());
                           const auto eb_op0 = IRman->create_extract_bit_node(bie->op0, ebe->op1, loc_info_default);
                           const auto eb_ga0 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op0, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                           B->PushBefore(eb_ga0, stmt, AppM);
                           const auto eb_op1 = IRman->create_extract_bit_node(bie->op1, ebe->op1, loc_info_default);
                           const auto eb_ga1 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                           B->PushBefore(eb_ga1, stmt, AppM);
                           const auto anding = IRman->create_binary_operation(
                               ebe->type, GetPointerS<const assign_stmt>(eb_ga0)->op0,
                               GetPointerS<const assign_stmt>(eb_ga1)->op0, loc_info_default, or_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, anding);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                        else if(prev_code1 == xor_node_K)
                        {
                           const auto bxe = GetPointerS<const xor_node>(prev_ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage before: " + stmt->ToString());
                           const auto eb_op0 = IRman->create_extract_bit_node(bxe->op0, ebe->op1, loc_info_default);
                           const auto eb_ga0 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op0, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                           B->PushBefore(eb_ga0, stmt, AppM);
                           const auto eb_op1 = IRman->create_extract_bit_node(bxe->op1, ebe->op1, loc_info_default);
                           const auto eb_ga1 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                           B->PushBefore(eb_ga1, stmt, AppM);
                           const auto anding = IRman->create_binary_operation(
                               ebe->type, GetPointerS<const assign_stmt>(eb_ga0)->op0,
                               GetPointerS<const assign_stmt>(eb_ga1)->op0, loc_info_default, xor_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, anding);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                        else if(prev_code1 == select_node_K)
                        {
                           const auto ce = GetPointerS<const select_node>(prev_ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage before: " + stmt->ToString());
                           const auto eb_op1 = IRman->create_extract_bit_node(ce->op1, ebe->op1, loc_info_default);
                           const auto eb_ga1 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                           B->PushBefore(eb_ga1, stmt, AppM);
                           const auto eb_op2 = IRman->create_extract_bit_node(ce->op2, ebe->op1, loc_info_default);
                           const auto eb_ga2 = IRman->CreateAssignStmt(
                               ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                               TM->CreateUniqueIntegerCst(1, ebe->type), eb_op2, function_id, loc_info_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga2));
                           B->PushBefore(eb_ga2, stmt, AppM);
                           const auto ceRes = IRman->create_ternary_operation(
                               ebe->type, ce->op0, GetPointerS<const assign_stmt>(eb_ga1)->op0,
                               GetPointerS<const assign_stmt>(eb_ga2)->op0, loc_info_default, select_node_K);
                           TM->ReplaceIRNode(stmt, rhs_node, ceRes);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace extract_bit_node usage after: " + stmt->ToString());
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                        }
                        else if(prev_code1 == add_node_K)
                        {
                           const auto pe = GetPointerS<const add_node>(prev_ga->op1);
                           if(pe->op1->get_kind() == constant_int_val_node_K &&
                              ir_helper::TypeSize(ebe->op0) <= max_lut_size)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace extract_bit_node usage before: " + stmt->ToString());
                              auto btype = IRman->GetBooleanType();
                              auto carry = TM->CreateUniqueIntegerCst(0, btype);
                              ir_nodeRef sum;
                              for(integer_cst_t bitIndex = 0; bitIndex <= pos_value; ++bitIndex)
                              {
                                 const auto bitIndex_node =
                                     TM->CreateUniqueIntegerCst(bitIndex, ir_helper::CGetType(ebe->op1));
                                 const auto eb_op1 =
                                     IRman->create_extract_bit_node(pe->op0, bitIndex_node, loc_info_default);
                                 const auto eb_ga1 = IRman->CreateAssignStmt(
                                     btype, TM->CreateUniqueIntegerCst(0, btype), TM->CreateUniqueIntegerCst(1, btype),
                                     eb_op1, function_id, loc_info_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                 B->PushBefore(eb_ga1, stmt, AppM);
                                 const auto eb_op2 =
                                     IRman->create_extract_bit_node(pe->op1, bitIndex_node, loc_info_default);
                                 const auto eb_ga2 = IRman->CreateAssignStmt(
                                     btype, TM->CreateUniqueIntegerCst(0, btype), TM->CreateUniqueIntegerCst(1, btype),
                                     eb_op2, function_id, loc_info_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga2));
                                 B->PushBefore(eb_ga2, stmt, AppM);
                                 const auto sum0 = IRman->create_binary_operation(
                                     btype, GetPointerS<const assign_stmt>(eb_ga1)->op0,
                                     GetPointerS<const assign_stmt>(eb_ga2)->op0, loc_info_default, xor_node_K);
                                 const auto sum0_ga1 = IRman->CreateAssignStmt(
                                     btype, TM->CreateUniqueIntegerCst(0, btype), TM->CreateUniqueIntegerCst(1, btype),
                                     sum0, function_id, loc_info_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(sum0_ga1));
                                 B->PushBefore(sum0_ga1, stmt, AppM);
                                 sum = IRman->create_binary_operation(btype,
                                                                      GetPointerS<const assign_stmt>(sum0_ga1)->op0,
                                                                      carry, loc_info_default, xor_node_K);
                                 if(bitIndex < pos_value)
                                 {
                                    const auto sum_ga1 = IRman->CreateAssignStmt(
                                        btype, TM->CreateUniqueIntegerCst(0, btype),
                                        TM->CreateUniqueIntegerCst(1, btype), sum, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---Created " + STR(sum_ga1));
                                    B->PushBefore(sum_ga1, stmt, AppM);

                                    const auto and1 = IRman->create_binary_operation(
                                        btype, GetPointerS<const assign_stmt>(eb_ga1)->op0, carry, loc_info_default,
                                        and_node_K);
                                    const auto and1_ga = IRman->CreateAssignStmt(
                                        btype, TM->CreateUniqueIntegerCst(0, btype),
                                        TM->CreateUniqueIntegerCst(1, btype), and1, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---Created " + STR(and1_ga));
                                    B->PushBefore(and1_ga, stmt, AppM);

                                    const auto and2 = IRman->create_binary_operation(
                                        btype, GetPointerS<const assign_stmt>(eb_ga2)->op0, carry, loc_info_default,
                                        and_node_K);
                                    const auto and2_ga = IRman->CreateAssignStmt(
                                        btype, TM->CreateUniqueIntegerCst(0, btype),
                                        TM->CreateUniqueIntegerCst(1, btype), and2, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---Created " + STR(and2_ga));
                                    B->PushBefore(and2_ga, stmt, AppM);

                                    const auto and3 = IRman->create_binary_operation(
                                        btype, GetPointerS<const assign_stmt>(eb_ga1)->op0,
                                        GetPointerS<const assign_stmt>(eb_ga2)->op0, loc_info_default, and_node_K);
                                    const auto and3_ga = IRman->CreateAssignStmt(
                                        btype, TM->CreateUniqueIntegerCst(0, btype),
                                        TM->CreateUniqueIntegerCst(1, btype), and3, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---Created " + STR(and3_ga));
                                    B->PushBefore(and3_ga, stmt, AppM);

                                    const auto or1 = IRman->create_binary_operation(
                                        btype, GetPointerS<const assign_stmt>(and1_ga)->op0,
                                        GetPointerS<const assign_stmt>(and2_ga)->op0, loc_info_default, or_node_K);
                                    const auto or1_ga = IRman->CreateAssignStmt(
                                        btype, TM->CreateUniqueIntegerCst(0, btype),
                                        TM->CreateUniqueIntegerCst(1, btype), or1, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(or1_ga));
                                    B->PushBefore(or1_ga, stmt, AppM);

                                    const auto carry1 = IRman->create_binary_operation(
                                        btype, GetPointerS<const assign_stmt>(or1_ga)->op0,
                                        GetPointerS<const assign_stmt>(and3_ga)->op0, loc_info_default, or_node_K);
                                    const auto carry1_ga = IRman->CreateAssignStmt(
                                        btype, TM->CreateUniqueIntegerCst(0, btype),
                                        TM->CreateUniqueIntegerCst(1, btype), carry1, function_id, loc_info_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---Created " + STR(carry1_ga));
                                    B->PushBefore(carry1_ga, stmt, AppM);
                                    carry = GetPointerS<const assign_stmt>(carry1_ga)->op0;
                                 }
                              }
                              TM->ReplaceIRNode(stmt, rhs_node, sum);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace extract_bit_node usage after: " + stmt->ToString());
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                        }
                     }
                  }
               }
               else if(ebe->op0->get_kind() == constant_int_val_node_K)
               {
                  const auto res_value =
                      ((ir_helper::GetConstValue(ebe->op0, ir_helper::IsSignedIntegerType(ebe->op0)) >> pos_value) &
                       1) != 0;
                  const auto res_node = TM->CreateUniqueIntegerCst(res_value, ebe->type);
                  propagateCurrValue(res_node, DEBUG_CALLSITE);
               }
            };
            extract_bit_node_BVO();
         }
         else if(rhs_kind == nop_node_K)
         {
            const auto ne = GetPointerS<const nop_node>(rhs_node);
            if(ir_helper::IsBooleanType(lhs_type))
            {
               if(ir_helper::IsBooleanType(ne->op))
               {
                  propagateCurrValue(ne->op, DEBUG_CALLSITE);
               }
               else
               {
                  if(ne->op->get_kind() == ssa_node_K)
                  {
                     const auto ne_op_ssa = GetPointerS<const ssa_node>(ne->op);
                     if(ne_op_ssa->type->get_kind() == integer_ty_node_K)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---replace extract_bit_node usage before: " + stmt->ToString());
                        const auto indexType = IRman->GetUnsignedLongLongType();
                        const auto zero_node = TM->CreateUniqueIntegerCst(0, indexType);
                        const auto eb_op = IRman->create_extract_bit_node(ne->op, zero_node, loc_info_default);
                        const auto eb_ga = IRman->CreateAssignStmt(ne->type, TM->CreateUniqueIntegerCst(0, ne->type),
                                                                   TM->CreateUniqueIntegerCst(1, ne->type), eb_op,
                                                                   function_id, loc_info_default);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                        B->PushBefore(eb_ga, stmt, AppM);
                        const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ne->type);
                        const auto masking =
                            IRman->create_binary_operation(ne->type, GetPointerS<const assign_stmt>(eb_ga)->op0,
                                                           bit_mask_constant_node, loc_info_default, and_node_K);
                        TM->ReplaceIRNode(stmt, rhs_node,
                                          masking); /// replaced with redundant code to restart bit-value analysis
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---replace extract_bit_node usage after: " + stmt->ToString());
                        modified = true;
                        AppM->RegisterTransformation(GetName(), stmt);
                     }
                  }
               }
            }
            else if(ir_helper::IsSameType(lhs_node, ne->op))
            {
               const auto bw_op1 = ir_helper::Size(ne->op);
               const auto bw_op0 = ir_helper::Size(lhs_node);

               if(bw_op1 <= bw_op0)
               {
                  propagateCurrValue(ne->op, DEBUG_CALLSITE);
               }
            }
         }

         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed statement " + stmt->ToString());
      }
      for(const auto& phi : B->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Analyzing phi " + phi->ToString());
         const auto pn = GetPointerS<phi_stmt>(phi);
         if(!pn->virtual_flag)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "LHS: " + STR(pn->res->index));
            if(pn->res->get_kind() == ssa_node_K)
            {
               const auto ssa = GetPointerS<const ssa_node>(pn->res);
               if(!ssa->bit_values.empty())
               {
                  if(is_bit_values_constant(ssa->bit_values))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---Left part is constant " + ssa->bit_values);
                     const auto const_value = convert_bitvalue_to_integer_cst(ssa->bit_values, pn->res);
                     const auto val = TM->CreateUniqueIntegerCst(const_value, ir_helper::CGetType(pn->res));

                     propagateValue(TM, pn->res, val, DEBUG_CALLSITE);
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Analyzed phi " + phi->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed BB" + STR(B->number));
   }
}
