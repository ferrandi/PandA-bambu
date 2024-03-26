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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file Bit_Value_opt.cpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

// Header include
#include "Bit_Value_opt.hpp"

#include "BitLatticeManipulator.hpp"
#include "Range.hpp"

/// Autoheader include
#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"

// Behavior include
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// HLS/vcd include
#include "Discrepancy.hpp"

// Parameter include
#include "Parameter.hpp"

/// design_flows includes
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"
#include "frontend_flow_step_factory.hpp"

// STD include
#include <boost/range/adaptor/reversed.hpp>
#include <cmath>
#include <fstream>
#include <string>

// Tree include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "math_function.hpp"       // for ceil_pow2
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "utility.hpp"

#define DEBUG_CALLSITE (__FILE__ + std::string(":") + STR(__LINE__))

Bit_Value_opt::Bit_Value_opt(const ParameterConstRef _parameters, const application_managerRef _AppM,
                             unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BIT_VALUE_OPT, _design_flow_manager, _parameters), modified(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

Bit_Value_opt::~Bit_Value_opt() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
Bit_Value_opt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
         {
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
            if(parameters->isOption(OPT_bitvalue_ipa) && parameters->getOption<bool>(OPT_bitvalue_ipa))
            {
               relationships.insert(std::make_pair(BIT_VALUE_IPA, WHOLE_APPLICATION));
            }
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
            if(!parameters->getOption<int>(OPT_gcc_openmp_simd))
            {
               relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
            }
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
   const auto TM = AppM->get_tree_manager();
   const auto IRman = tree_manipulationRef(new tree_manipulation(TM, parameters, AppM));

   const auto tn = TM->CGetTreeNode(function_id);
   // tree_nodeRef Scpe = TM->GetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   modified = false;
   optimize(fd, TM, IRman);
   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void Bit_Value_opt::constrainSSA(ssa_name* op_ssa, tree_managerRef TM)
{
   if(tree_helper::IsRealType(op_ssa->type))
   {
      return;
   }
   const auto nbit = op_ssa->bit_values.size();
   THROW_ASSERT(!op_ssa->bit_values.empty(), "unexpected condition");
   const auto nbitType = tree_helper::TypeSize(op_ssa->type);
   if(nbit != nbitType)
   {
      const bool isSigned = tree_helper::IsSignedIntegerType(op_ssa->type);
      if(isSigned)
      {
         RangeRef constraintRange(
             new Range(Regular, static_cast<Range::bw_t>(nbitType), -(1ll << (nbit - 1)), (1ll << (nbit - 1)) - 1));
         if(op_ssa->range)
         {
            if(op_ssa->range->getSpan() < constraintRange->getSpan())
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
         RangeRef constraintRange(new Range(Regular, static_cast<Range::bw_t>(nbitType), 0, (1ll << nbit) - 1));
         if(op_ssa->range)
         {
            if(op_ssa->range->getSpan() < constraintRange->getSpan())
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

static integer_cst_t convert_bitvalue_to_integer_cst(const std::string& bit_values, tree_managerRef TM,
                                                     unsigned int var_id)
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
   const auto is_signed = tree_helper::is_int(TM, var_id);
   if(is_signed && bit_values[0] == '1')
   {
      const_value |= integer_cst_t(-1) << index_val;
      THROW_ASSERT(const_value < 0, "");
   }
   return const_value;
}

static bool is_bit_values_constant(const std::string& bit_values)
{
   bool is_constant = bit_values.size() != 0;
   for(auto current_el : bit_values)
   {
      if(current_el == 'U')
      {
         is_constant = false;
         break;
      }
   }
   return is_constant;
}

void Bit_Value_opt::propagateValue(const ssa_name* ssa, tree_managerRef TM, tree_nodeRef old_val, tree_nodeRef new_val,
                                   const std::string
#if HAVE_ASSERTS
                                       callSiteString
#endif
)
{
   THROW_ASSERT(tree_helper::Size(old_val) >= tree_helper::Size(new_val),
                "unexpected case " + STR(old_val) + " " + STR(new_val) + " old-bw=" + STR(tree_helper::Size(old_val)) +
                    " new-bw=" + STR(tree_helper::Size(new_val)) + " from " + callSiteString);
   const auto StmtUses = ssa->CGetUseStmts();
   for(const auto& use : StmtUses)
   {
      if(!AppM->ApplyNewTransformation())
      {
         break;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
      TM->ReplaceTreeNode(use.first, old_val, new_val);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
      modified = true;
      AppM->RegisterTransformation(GetName(), use.first);
   }
}

void Bit_Value_opt::optimize(const function_decl* fd, tree_managerRef TM, tree_manipulationRef IRman)
{
   THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_device(), "unexpected condition");
   const auto hls_d = GetPointerS<const HLS_manager>(AppM)->get_HLS_device();
   THROW_ASSERT(hls_d->has_parameter("max_lut_size"), "unexpected condition");
   const auto max_lut_size = hls_d->get_parameter<size_t>("max_lut_size");

   /// in case propagate constants from parameters
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      const unsigned int p_decl_id = AppM->getSSAFromParm(function_id, GET_INDEX_CONST_NODE(parm_decl_node));
      if(tree_helper::is_real(TM, p_decl_id) || tree_helper::is_a_complex(TM, p_decl_id))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Parameter not supported " + TM->CGetTreeNode(p_decl_id)->ToString());
         continue;
      }
      if(AppM->ApplyNewTransformation())
      {
         const auto parm_type = tree_helper::CGetType(parm_decl_node);
         const auto parmssa = TM->CGetTreeNode(p_decl_id);
         const auto p = GetPointer<const ssa_name>(parmssa);
         THROW_ASSERT(!p->bit_values.empty(), "unexpected condition");
         const auto is_constant = is_bit_values_constant(p->bit_values);
         if(is_constant)
         {
            const auto ull_value = convert_bitvalue_to_integer_cst(p->bit_values, TM, p_decl_id);
            const auto val = TM->CreateUniqueIntegerCst(ull_value, parm_type);
            propagateValue(p, TM, TM->CGetTreeNode(p_decl_id), val, DEBUG_CALLSITE);
         }
      }
   }
   auto sl = GetPointerS<statement_list>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list");
   for(const auto& bb_pair : sl->list_of_bloc)
   {
      const auto B = bb_pair.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B->number));
      const auto list_of_stmt = B->CGetStmtList();
      for(const auto& stmt : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + stmt->ToString());
         if(!AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Skipped because reached limit of CFG transformations");
            break;
         }
         if(GetPointerS<gimple_node>(stmt)->keep)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                           "<--Skipped because the statement has been annotated with the keep tag");
            continue;
         }
         if(stmt->get_kind() == gimple_assign_K)
         {
            auto ga = GetPointerS<gimple_assign>(stmt);
            unsigned int output_uid = GET_INDEX_CONST_NODE(ga->op0);
            auto ssa = GetPointer<ssa_name>(ga->op0);
            if(ssa && !ssa->bit_values.empty() && !ssa->CGetUseStmts().empty())
            {
               auto condPropageValue = [&](tree_nodeRef val, std::string debug_callsite) {
                  auto bw_op1 = tree_helper::Size(val);
                  auto bw_op0 = tree_helper::Size(ga->op0);
                  if(bw_op1 <= bw_op0)
                  {
                     propagateValue(ssa, TM, ga->op0, val, debug_callsite);
                  }
                  else
                  {
                     const auto srcp = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                     const auto& val_type = ssa->type;
                     const auto val_type_bw = tree_helper::Size(ssa->type);
                     const auto shift_offset =
                         TM->CreateUniqueIntegerCst(static_cast<long long>(val_type_bw - bw_op0), val_type);
                     const auto shl_expr =
                         IRman->create_binary_operation(val_type, val, shift_offset, srcp, lshift_expr_K);
                     const auto shl =
                         IRman->CreateGimpleAssign(val_type, nullptr, nullptr, shl_expr, function_id, srcp);
                     const auto svar = GetPointerS<const gimple_assign>(GET_CONST_NODE(shl))->op0;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(shl));
                     B->PushBefore(shl, stmt, AppM);
                     const auto shr_expr =
                         IRman->create_binary_operation(val_type, svar, shift_offset, srcp, rshift_expr_K);
                     const auto op0_ga =
                         IRman->CreateGimpleAssign(ssa->type, ssa->min, ssa->max, shr_expr, function_id, srcp);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                     B->PushBefore(op0_ga, stmt, AppM);
                     auto op0_ga_var = GetPointerS<gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                     auto op0_ga_varSSA = GetPointerS<ssa_name>(GET_CONST_NODE(op0_ga_var));
                     op0_ga_varSSA->bit_values = ssa->bit_values;
                     constrainSSA(op0_ga_varSSA, TM);
                     propagateValue(ssa, TM, ga->op0, op0_ga_var, DEBUG_CALLSITE);
                  }
               };
               if(tree_helper::is_real(TM, output_uid))
               {
                  auto real_BVO = [&] {
                     if(ga->op1->get_kind() == cond_expr_K)
                     {
                        const auto me = GetPointerS<cond_expr>(ga->op1);
                        if(me->op1 == me->op2)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with equal operands");
                           propagateValue(ssa, TM, ga->op0, me->op1, DEBUG_CALLSITE);
                        }
                        else if(GET_CONST_NODE(me->op0)->get_kind() == integer_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Cond expr with constant condition");
                           const auto new_val = tree_helper::GetConstValue(me->op0) ? me->op1 : me->op2;
                           propagateValue(ssa, TM, ga->op0, new_val, DEBUG_CALLSITE);
                        }
                        else
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---nothing more can be done");
                        }
                     }
                     else if(GetPointer<cst_node>(ga->op1))
                     {
                        propagateValue(ssa, TM, ga->op0, ga->op1, DEBUG_CALLSITE);
                     }
                     else if(GetPointer<ssa_name>(ga->op1))
                     {
                        propagateValue(ssa, TM, ga->op0, ga->op1, DEBUG_CALLSITE);
                     }
                     else if(ga->op1->get_kind() == view_convert_expr_K)
                     {
                        auto vce = GetPointerS<view_convert_expr>(ga->op1);
                        if(GetPointer<cst_node>(vce->op))
                        {
                           if(vce->op->get_kind() == integer_cst_K)
                           {
                              const auto cst_val = tree_helper::GetConstValue(vce->op);
                              auto bitwidth_op = tree_helper::TypeSize(vce->type);
                              tree_nodeRef val;
                              if(bitwidth_op == 32)
                              {
                                 union
                                 {
                                    float dest;
                                    int source;
                                 } __conv_union = {};
                                 __conv_union.source = static_cast<int>(cst_val);
                                 val = TM->CreateUniqueRealCst(static_cast<long double>(__conv_union.dest), vce->type);
                              }
                              else if(bitwidth_op == 64)
                              {
                                 union
                                 {
                                    double dest;
                                    long long int source;
                                 } __conv_union = {};
                                 __conv_union.source = static_cast<long long int>(cst_val);
                                 val = TM->CreateUniqueRealCst(static_cast<long double>(__conv_union.dest), vce->type);
                              }
                              else
                              {
                                 THROW_ERROR("not supported floating point bitwidth");
                              }
                              propagateValue(ssa, TM, ga->op0, val, DEBUG_CALLSITE);
                           }
                        }
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---real variables not considered: " + STR(GET_INDEX_CONST_NODE(ga->op0)));
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "<--Examined statement " + stmt->ToString());
                  };
                  real_BVO();
                  continue;
               }
               if(tree_helper::is_a_complex(TM, output_uid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---complex variables not considered: " + STR(GET_INDEX_CONST_NODE(ga->op0)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
                  continue;
               }
               if(tree_helper::is_a_vector(TM, output_uid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---vector variables not considered: " + STR(GET_INDEX_CONST_NODE(ga->op0)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
                  continue;
               }
               if((GetPointer<integer_cst>(ga->op1) || GetPointer<real_cst>(ga->op1)) &&
                  tree_helper::IsPointerType(ga->op1))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---constant pointer value assignments not considered: " +
                                     STR(GET_INDEX_CONST_NODE(ga->op0)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
                  continue;
               }
               if(GetPointer<call_expr>(ga->op1) and ga->vdef)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---calls with side effects cannot be optimized" + STR(GET_INDEX_CONST_NODE(ga->op1)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
                  continue;
               }
               if(GetPointer<addr_expr>(ga->op1))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---addr_expr cannot be optimized" + STR(GET_INDEX_CONST_NODE(ga->op1)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + stmt->ToString());
                  continue;
               }
               auto ga_BVO = [&] {
                  const auto ga_op_type = tree_helper::CGetType(ga->op0);
                  const auto Scpe = TM->GetTreeNode(function_id);
                  const auto& bit_values = ssa->bit_values;
                  auto is_constant = is_bit_values_constant(bit_values) && !tree_helper::IsPointerType(ga->op1);
                  auto rel_expr_BVO = [&] {
                     auto* me = GetPointer<binary_expr>(ga->op1);
                     const auto op0 = GET_CONST_NODE(me->op0);
                     const auto op1 = GET_CONST_NODE(me->op1);

                     std::string s0, s1;
                     const auto is_op0_ssa = GetPointer<const ssa_name>(op0);
                     const auto is_op1_ssa = GetPointer<const ssa_name>(op1);
                     if(is_op0_ssa)
                     {
                        s0 = GetPointerS<const ssa_name>(op0)->bit_values;
                        if(s0.empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Skipped because bitvalue of op0 is empty");
                           return;
                        }
                     }
                     else
                     {
                        THROW_ASSERT(op0->get_kind() == integer_cst_K, "unexpected condition");
                     }
                     if(is_op1_ssa)
                     {
                        s1 = GetPointerS<const ssa_name>(op1)->bit_values;
                        if(s1.empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Skipped because bitvalue of op1 is empty");
                           return;
                        }
                     }
                     else
                     {
                        THROW_ASSERT(op1->get_kind() == integer_cst_K, "unexpected condition");
                     }
                     if(!is_op0_ssa && !is_op1_ssa)
                     {
                        return;
                     }

                     unsigned int trailing_zero = 0;
                     if(!is_op0_ssa)
                     {
                        const auto uvalue = tree_helper::GetConstValue(me->op0, false);
                        s0 = convert_to_binary(uvalue, tree_helper::TypeSize(me->op0));
                     }
                     if(!is_op1_ssa)
                     {
                        const auto uvalue = tree_helper::GetConstValue(me->op1, false);
                        s1 = convert_to_binary(uvalue, tree_helper::TypeSize(me->op1));
                     }

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
                                       "-->Bit Value Opt: " + std::string(ga->op1->get_kind_text()) +
                                           " optimized, nbits = " + STR(trailing_zero));
                        INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                        modified = true;
                        AppM->RegisterTransformation(GetName(), stmt);
                        const auto srcp_default =
                            ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                        const auto op0_op_type = tree_helper::CGetType(op0);
                        const auto op1_op_type = tree_helper::CGetType(op1);

                        if(is_op0_ssa)
                        {
                           const auto op0_const_node =
                               TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), op0_op_type);
                           const auto op0_expr = IRman->create_binary_operation(op0_op_type, me->op0, op0_const_node,
                                                                                srcp_default, rshift_expr_K);
                           const auto op0_ga = IRman->CreateGimpleAssign(op0_op_type, nullptr, nullptr, op0_expr,
                                                                         function_id, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                           B->PushBefore(op0_ga, stmt, AppM);
                           const auto op0_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                           TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
                           /// set the bit_values to the ssa var
                           auto op0_ssa = GetPointer<ssa_name>(op0_ga_var);
                           THROW_ASSERT(s0.size() - trailing_zero > 0, "unexpected condition");
                           op0_ssa->bit_values = s0.substr(0, s0.size() - trailing_zero);
                           constrainSSA(op0_ssa, TM);
                        }
                        else
                        {
                           const auto cst_val =
                               tree_helper::GetConstValue(me->op0, tree_helper::IsSignedIntegerType(op0));
                           TM->ReplaceTreeNode(stmt, me->op0,
                                               TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op0_op_type));
                        }
                        if(is_op1_ssa)
                        {
                           const auto op1_const_node =
                               TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), op1_op_type);
                           const auto op1_expr = IRman->create_binary_operation(op1_op_type, me->op1, op1_const_node,
                                                                                srcp_default, rshift_expr_K);
                           const auto op1_ga = IRman->CreateGimpleAssign(op1_op_type, nullptr, nullptr, op1_expr,
                                                                         function_id, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                           B->PushBefore(op1_ga, stmt, AppM);
                           const auto op1_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op1_ga))->op0;
                           TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
                           /// set the bit_values to the ssa var
                           const auto op1_ssa = GetPointer<ssa_name>(op1_ga_var);
                           THROW_ASSERT(s1.size() - trailing_zero > 0, "unexpected condition");
                           op1_ssa->bit_values = s1.substr(0, s1.size() - trailing_zero);
                           constrainSSA(op1_ssa, TM);
                        }
                        else
                        {
                           const auto cst_val =
                               tree_helper::GetConstValue(me->op1, tree_helper::IsSignedIntegerType(op1));
                           TM->ReplaceTreeNode(stmt, me->op1,
                                               TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op1_op_type));
                        }
                     }
                  };

                  if(is_constant)
                  {
                     auto c_BVO = [&] {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Left part is constant " + bit_values);
                        tree_nodeRef val;
                        if(GET_CONST_NODE(ga->op1)->get_kind() == integer_cst_K &&
                           tree_helper::Size(ga->op1) <= bit_values.size())
                        {
                           val = ga->op1;
                        }
                        else
                        {
                           const auto const_value = convert_bitvalue_to_integer_cst(bit_values, TM, output_uid);
                           val = TM->CreateUniqueIntegerCst(const_value, ga_op_type);
                        }
                        if(AppM->ApplyNewTransformation())
                        {
                           if(GET_CONST_NODE(ga->op0)->get_kind() == ssa_name_K && ga->predicate)
                           {
                              if(GET_CONST_NODE(ga->predicate)->get_kind() != integer_cst_K ||
                                 tree_helper::GetConstValue(ga->predicate) != 0)
                              {
                                 const auto bt = IRman->GetBooleanType();
                                 const auto zeroval = TM->CreateUniqueIntegerCst(static_cast<long long int>(0), bt);
                                 TM->ReplaceTreeNode(stmt, ga->predicate, zeroval);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---zero predicated statement: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                           }
                        }
                        condPropageValue(val, DEBUG_CALLSITE);
                     };
                     c_BVO();
                  }
                  else if(GetPointer<const cst_node>(GET_CONST_NODE(ga->op1)))
                  {
                     propagateValue(ssa, TM, ga->op0, ga->op1, DEBUG_CALLSITE);
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == ssa_name_K)
                  {
                     auto ssa_name_BVO = [&] {
                        if(!ssa->bit_values.empty() && ssa->bit_values.at(0) == '0' && ssa->bit_values.size() <= 64)
                        {
                           const auto srcp_default =
                               ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           const auto bit_mask_constant_node =
                               TM->CreateUniqueIntegerCst((1LL << (ssa->bit_values.size() - 1)) - 1, ssa->type);
                           const auto band_expr = IRman->create_binary_operation(
                               ssa->type, ga->op1, bit_mask_constant_node, srcp_default, bit_and_expr_K);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace ssa usage before: " + stmt->ToString());
                           TM->ReplaceTreeNode(stmt, ga->op1, band_expr);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace ssa usage after: " + stmt->ToString());
                           modified = true;
                           ga->keep = true; /// this prevent an infinite loop with CSE
                        }
                        else
                        {
                           auto bw_op1 = tree_helper::Size(ga->op1);
                           auto bw_op0 = tree_helper::Size(ga->op0);

                           if(bw_op1 <= bw_op0)
                           {
                              propagateValue(ssa, TM, ga->op0, ga->op1, DEBUG_CALLSITE);
                           }
                        }
                     };
                     ssa_name_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == mult_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == widen_mult_expr_K)
                  {
                     auto mult_expr_BVO = [&] {
                        const auto me = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
                        const auto op0 = GET_CONST_NODE(me->op0);
                        const auto op1 = GET_CONST_NODE(me->op1);
                        bool squareP = GET_INDEX_CONST_NODE(me->op0) == GET_INDEX_CONST_NODE(me->op1);
                        const auto data_bitsize_in0 = ceil_pow2(tree_helper::TypeSize(op0));
                        const auto data_bitsize_in1 = ceil_pow2(tree_helper::TypeSize(op1));
                        const auto isSigned = tree_helper::is_int(TM, GET_INDEX_CONST_NODE(ga_op_type));
                        if(!isSigned && GET_CONST_NODE(ga->op1)->get_kind() == mult_expr_K &&
                           (data_bitsize_in0 == 1 || data_bitsize_in1 == 1))
                        {
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                           const auto constNE0 = TM->CreateUniqueIntegerCst(0LL, ga_op_type);
                           const auto srcp_default =
                               ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           const auto bt = IRman->GetBooleanType();
                           const auto cond_op0 = IRman->create_binary_operation(
                               bt, data_bitsize_in0 == 1 ? me->op0 : me->op1, constNE0, srcp_default, ne_expr_K);
                           const auto op0_ga = IRman->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0LL, bt),
                                                                         TM->CreateUniqueIntegerCst(1LL, bt), cond_op0,
                                                                         function_id, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                           B->PushBefore(op0_ga, stmt, AppM);
                           const auto op0_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                           const auto const0 = TM->CreateUniqueIntegerCst(0LL, ga_op_type);
                           const auto cond_op = IRman->create_ternary_operation(
                               ga_op_type, op0_ga_var, data_bitsize_in1 == 1 ? me->op0 : me->op1, const0, srcp_default,
                               cond_expr_K);
                           THROW_ASSERT(tree_helper::CGetType(GetPointer<const gimple_assign>(stmt)->op0)->index ==
                                            GET_INDEX_CONST_NODE(ga_op_type),
                                        "");
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Replacing " + STR(ga->op1) + " with " + STR(cond_op) + " in " +
                                              STR(stmt));
                           TM->ReplaceTreeNode(stmt, ga->op1, cond_op);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---replace expression with a cond_expr: " + stmt->ToString());
                        }
                        else
                        {
                           unsigned int trailing_zero_op0 = 0;
                           unsigned int trailing_zero_op1 = 0;
                           std::string bit_values_op0;
                           std::string bit_values_op1;
                           bool is_op0_ssa = op0->get_kind() == ssa_name_K;
                           bool is_op1_ssa = op1->get_kind() == ssa_name_K;
                           if(is_op0_ssa)
                           {
                              bit_values_op0 = GetPointer<const ssa_name>(op0)->bit_values;
                              if(bit_values_op0.empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Skipped because bitvalue of op0 is empty");
                                 return;
                              }
                              for(auto current_el : boost::adaptors::reverse(bit_values_op0))
                              {
                                 if(current_el == '0' || current_el == 'X')
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
                              THROW_ASSERT(op0->get_kind() == integer_cst_K, "unexpected case");
                              const auto cst_val = tree_helper::GetConstValue(op0, false);
                              bit_values_op0 = convert_to_binary(cst_val, tree_helper::Size(op0));
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
                              bit_values_op1 = GetPointer<const ssa_name>(op1)->bit_values;
                              if(bit_values_op1.empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Skipped because bitvalue of op1 is empty");
                                 return;
                              }
                              for(auto current_el : boost::adaptors::reverse(bit_values_op1))
                              {
                                 if(current_el == '0' || current_el == 'X')
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
                              THROW_ASSERT(op1->get_kind() == integer_cst_K, "unexpected case");
                              const auto cst_val = tree_helper::GetConstValue(op1, false);
                              bit_values_op1 = convert_to_binary(cst_val, tree_helper::Size(op1));
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
                                             "-->Bit Value Opt: mult_expr/widen_mult_expr optimized, nbits = " +
                                                 STR(trailing_zero_op0 + trailing_zero_op1));
                              INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                              const auto srcp_default =
                                  ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              if(trailing_zero_op0 != 0)
                              {
                                 const auto op0_type = tree_helper::CGetType(op0);
                                 const auto op0_const_node = TM->CreateUniqueIntegerCst(
                                     static_cast<long long int>(trailing_zero_op0), op0_type);
                                 const auto op0_expr = IRman->create_binary_operation(op0_type, me->op0, op0_const_node,
                                                                                      srcp_default, rshift_expr_K);
                                 const auto op0_ga = IRman->CreateGimpleAssign(op0_type, nullptr, nullptr, op0_expr,
                                                                               function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt, AppM);
                                 const auto op0_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
                                 /// set the bit_values to the ssa var
                                 if(is_op0_ssa)
                                 {
                                    auto op0_ssa = GetPointer<ssa_name>(op0_ga_var);
                                    THROW_ASSERT(bit_values_op0.size() - trailing_zero_op0 > 0, "unexpected condition");
                                    op0_ssa->bit_values =
                                        bit_values_op0.substr(0, bit_values_op0.size() - trailing_zero_op0);
                                    constrainSSA(op0_ssa, TM);
                                 }
                              }
                              if(trailing_zero_op1 != 0 && !squareP)
                              {
                                 const auto op1_type = tree_helper::CGetType(op1);
                                 const auto op1_const_node = TM->CreateUniqueIntegerCst(
                                     static_cast<long long int>(trailing_zero_op1), op1_type);
                                 const auto op1_expr = IRman->create_binary_operation(op1_type, me->op1, op1_const_node,
                                                                                      srcp_default, rshift_expr_K);
                                 const auto op1_ga = IRman->CreateGimpleAssign(op1_type, tree_nodeRef(), tree_nodeRef(),
                                                                               op1_expr, function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt, AppM);
                                 const auto op1_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op1_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
                                 /// set the bit_values to the ssa var
                                 if(is_op1_ssa)
                                 {
                                    auto* op1_ssa = GetPointer<ssa_name>(op1_ga_var);
                                    THROW_ASSERT(bit_values_op1.size() - trailing_zero_op1 > 0, "unexpected condition");
                                    op1_ssa->bit_values =
                                        bit_values_op1.substr(0, bit_values_op1.size() - trailing_zero_op1);
                                    constrainSSA(op1_ssa, TM);
                                 }
                              }

                              const auto ssa_vd = IRman->create_ssa_name(nullptr, ga_op_type, nullptr, nullptr);
                              auto sn = GetPointer<ssa_name>(ssa_vd);
                              /// set the bit_values to the ssa var
                              THROW_ASSERT(ssa->bit_values.size() - trailing_zero_op0 - trailing_zero_op1 > 0,
                                           "unexpected condition");
                              sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_zero_op0 -
                                                                             trailing_zero_op1);
                              constrainSSA(sn, TM);
                              const auto op_const_node =
                                  TM->CreateUniqueIntegerCst((trailing_zero_op0 + trailing_zero_op1), ga_op_type);
                              const auto op_expr = IRman->create_binary_operation(ga_op_type, ssa_vd, op_const_node,
                                                                                  srcp_default, lshift_expr_K);
                              const auto curr_ga = IRman->CreateGimpleAssign(ga_op_type, ssa->min, ssa->max, op_expr,
                                                                             function_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                              TM->ReplaceTreeNode(
                                  curr_ga, GetPointer<const gimple_assign>(GET_CONST_NODE(curr_ga))->op0, ga->op0);
                              TM->ReplaceTreeNode(stmt, ga->op0, ssa_vd);
                              B->PushAfter(curr_ga, stmt, AppM);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "pushed");
                           }
                        }
                     };
                     mult_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == plus_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == minus_expr_K)
                  {
                     auto plus_minus_BVO = [&] {
                        const auto me = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
                        bool identicalP = GET_INDEX_CONST_NODE(me->op0) == GET_INDEX_CONST_NODE(me->op1);
                        if(identicalP)
                        {
                           if(GET_CONST_NODE(ga->op1)->get_kind() == minus_expr_K)
                           {
                              TM->ReplaceTreeNode(stmt, ga->op1, TM->CreateUniqueIntegerCst(0LL, ga_op_type));
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Statement transformed in " + stmt->ToString());
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                           else
                           {
                              const auto op_const_node =
                                  TM->CreateUniqueIntegerCst(static_cast<long long int>(1), ga_op_type);
                              const auto srcp_default =
                                  ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              const auto op_expr = IRman->create_binary_operation(ga_op_type, me->op0, op_const_node,
                                                                                  srcp_default, lshift_expr_K);
                              TM->ReplaceTreeNode(stmt, ga->op1, op_expr);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Statement transformed in " + stmt->ToString());
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                           return;
                        }
                        const auto op0 = GET_CONST_NODE(me->op0);
                        const auto op1 = GET_CONST_NODE(me->op1);
                        bool is_op0_ssa = op0->get_kind() == ssa_name_K;
                        bool is_op1_ssa = op1->get_kind() == ssa_name_K;
                        std::string bit_values_op0;
                        std::string bit_values_op1;

                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                      "Var_uid: " +
                                          AppM->CGetFunctionBehavior(function_id)
                                              ->CGetBehavioralHelper()
                                              ->PrintVariable(output_uid) +
                                          " bitstring: " + bit_values);
                        unsigned int trailing_zero_op0 = 0;
                        unsigned int trailing_zero_op1 = 0;
                        bool is_op0_null = false;
                        bool is_op1_null = false;

                        if(GET_CONST_NODE(ga->op1)->get_kind() == plus_expr_K)
                        {
                           if(is_op0_ssa)
                           {
                              bit_values_op0 = GetPointerS<const ssa_name>(op0)->bit_values;
                              if(bit_values_op0.empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Skipped because bitvalue of op0 is empty");
                                 return;
                              }
                              PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                            "Var_uid: " +
                                                AppM->CGetFunctionBehavior(function_id)
                                                    ->CGetBehavioralHelper()
                                                    ->PrintVariable(GET_INDEX_CONST_NODE(me->op0)) +
                                                " bitstring: " + bit_values_op0);
                              for(const auto& current_el : boost::adaptors::reverse(bit_values_op0))
                              {
                                 if(current_el == '0' || current_el == 'X')
                                 {
                                    ++trailing_zero_op0;
                                 }
                                 else
                                 {
                                    break;
                                 }
                              }
                              if(bit_values_op0.find_first_not_of("0X") == std::string::npos ||
                                 trailing_zero_op0 >= bit_values.size())
                              {
                                 is_op0_null = true;
                              }
                           }
                           else
                           {
                              THROW_ASSERT(op0->get_kind() == integer_cst_K, "unexpected case");
                              const auto cst_val = tree_helper::GetConstValue(op0, false);
                              bit_values_op0 = convert_to_binary(cst_val, tree_helper::Size(op0));
                              if(cst_val == 0)
                              {
                                 is_op0_null = true;
                              }
                              else
                              {
                                 for(unsigned int index = 0; index < bit_values_op0.size(); ++index)
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
                                 if(trailing_zero_op0 >= bit_values.size())
                                 {
                                    is_op1_null = true;
                                 }
                              }
                           }
                        }
                        else
                        {
                           if(is_op0_ssa)
                           {
                              bit_values_op0 = GetPointerS<const ssa_name>(op0)->bit_values;
                              if(bit_values_op0.empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Skipped because bitvalue of op0 is empty");
                                 return;
                              }
                           }
                        }

                        if(is_op1_ssa)
                        {
                           bit_values_op1 = GetPointerS<const ssa_name>(op1)->bit_values;
                           if(bit_values_op1.empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Skipped because bitvalue of op1 is empty");
                              return;
                           }
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                         "Var_uid: " +
                                             AppM->CGetFunctionBehavior(function_id)
                                                 ->CGetBehavioralHelper()
                                                 ->PrintVariable(GET_INDEX_CONST_NODE(me->op1)) +
                                             " bitstring: " + bit_values_op1);
                           for(const auto& current_el : boost::adaptors::reverse(bit_values_op1))
                           {
                              if(current_el == '0' || current_el == 'X')
                              {
                                 ++trailing_zero_op1;
                              }
                              else
                              {
                                 break;
                              }
                           }
                           if(bit_values_op1.find_first_not_of("0X") == std::string::npos ||
                              trailing_zero_op1 >= bit_values.size())
                           {
                              is_op1_null = true;
                           }
                        }
                        else
                        {
                           THROW_ASSERT(op1->get_kind() == integer_cst_K, "unexpected case");
                           const auto cst_val = tree_helper::GetConstValue(op1, false);
                           bit_values_op1 = convert_to_binary(cst_val, tree_helper::Size(op1));
                           if(cst_val == 0)
                           {
                              is_op1_null = true;
                           }
                           else
                           {
                              for(unsigned int index = 0; index < bit_values_op1.size(); ++index)
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
                              if(trailing_zero_op1 >= bit_values.size())
                              {
                                 is_op1_null = true;
                              }
                           }
                        }

                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---Trailing zeros op0=" + STR(trailing_zero_op0) +
                                           ", trailing zeros op1=" + STR(trailing_zero_op1));
                        if(is_op0_null)
                        {
                           condPropageValue(me->op1, DEBUG_CALLSITE);
                        }
                        else if(is_op1_null)
                        {
                           condPropageValue(me->op0, DEBUG_CALLSITE);
                        }
                        else if(trailing_zero_op0 != 0 || trailing_zero_op1 != 0)
                        {
                           modified = true;
                           AppM->RegisterTransformation(GetName(), stmt);
                           const auto srcp_default =
                               ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           const auto is_first_max = trailing_zero_op0 > trailing_zero_op1;

                           auto shift_const = is_first_max ? trailing_zero_op0 : trailing_zero_op1;

                           INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level,
                                          "---Bit Value Opt: " +
                                              (GET_CONST_NODE(ga->op1)->get_kind() == plus_expr_K ?
                                                   std::string("plus_expr") :
                                                   std::string("minus_expr")) +
                                              " optimized, nbits = " + STR(shift_const));
                           const auto shift_constant_node =
                               TM->CreateUniqueIntegerCst(static_cast<long long int>(shift_const), ga_op_type);
                           const auto op0_type = tree_helper::CGetType(op0);
                           const auto op1_type = tree_helper::CGetType(op1);
                           const auto b_node = is_first_max ? me->op1 : me->op0;
                           const auto b_type = is_first_max ? op1_type : op0_type;

                           if(!is_op0_ssa)
                           {
                              const auto cst_val =
                                  tree_helper::GetConstValue(op0, tree_helper::IsSignedIntegerType(op0_type));
                              if(ssa->bit_values.size() <= shift_const)
                              {
                                 is_op0_null = true;
                              }
                              else
                              {
                                 if((cst_val >> shift_const) == 0)
                                 {
                                    is_op0_null = GET_CONST_NODE(ga->op1)->get_kind() == plus_expr_K; // TODO: true?
                                 }
                                 TM->ReplaceTreeNode(stmt, me->op0,
                                                     TM->CreateUniqueIntegerCst(cst_val >> shift_const, op0_type));
                              }
                           }
                           else
                           {
                              std::string resulting_bit_values;

                              if((bit_values_op0.size() - shift_const) > 0)
                              {
                                 resulting_bit_values = bit_values_op0.substr(0, bit_values_op0.size() - shift_const);
                              }
                              else if(tree_helper::IsSignedIntegerType(op0_type))
                              {
                                 resulting_bit_values = bit_values_op0.substr(0, 1);
                              }
                              else
                              {
                                 resulting_bit_values = "0";
                              }

                              if(resulting_bit_values.find_first_not_of("0X") == std::string::npos &&
                                 GET_CONST_NODE(ga->op1)->get_kind() == plus_expr_K)
                              {
                                 is_op0_null = true;
                              }
                              else
                              {
                                 const auto op0_expr = IRman->create_binary_operation(
                                     op0_type, me->op0, shift_constant_node, srcp_default, rshift_expr_K);
                                 const auto op0_ga = IRman->CreateGimpleAssign(op0_type, nullptr, nullptr, op0_expr,
                                                                               function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt, AppM);
                                 const auto op0_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Replacing " + me->op0->ToString() + " with " +
                                                    op0_ga_var->ToString() + " in " + stmt->ToString());
                                 TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned by this
                                  * bitshift must not be checked if it was applied to
                                  * a variable marked as address.
                                  */
                                 if(parameters->isOption(OPT_discrepancy) and
                                    parameters->getOption<bool>(OPT_discrepancy))
                                 {
                                    AppM->RDiscr->ssa_to_skip_if_address.insert(op0_ga_var);
                                 }
#endif
                                 /// set the bit_values to the ssa var
                                 auto op0_ssa = GetPointer<ssa_name>(op0_ga_var);
                                 THROW_ASSERT(resulting_bit_values.size() > 0, "unexpected condition");
                                 op0_ssa->bit_values = resulting_bit_values;
                                 constrainSSA(op0_ssa, TM);
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "Var_uid: " +
                                                   AppM->CGetFunctionBehavior(function_id)
                                                       ->CGetBehavioralHelper()
                                                       ->PrintVariable(GET_INDEX_CONST_NODE(op0_ga_var)) +
                                                   " bitstring: " + STR(op0_ssa->bit_values));
                              }
                           }

                           if(!is_op1_ssa)
                           {
                              const auto cst_val =
                                  tree_helper::GetConstValue(op1, tree_helper::IsSignedIntegerType(op1_type));
                              if(ssa->bit_values.size() <= shift_const)
                              {
                                 is_op1_null = true;
                              }
                              else
                              {
                                 if((cst_val >> shift_const) == 0)
                                 {
                                    is_op1_null = true;
                                 }
                                 TM->ReplaceTreeNode(stmt, me->op1,
                                                     TM->CreateUniqueIntegerCst(cst_val >> shift_const, op1_type));
                              }
                           }
                           else
                           {
                              std::string resulting_bit_values;

                              if((bit_values_op1.size() - shift_const) > 0)
                              {
                                 resulting_bit_values = bit_values_op1.substr(0, bit_values_op1.size() - shift_const);
                              }
                              else if(tree_helper::IsSignedIntegerType(op1_type))
                              {
                                 resulting_bit_values = bit_values_op1.substr(0, 1);
                              }
                              else
                              {
                                 resulting_bit_values = "0";
                              }

                              if(resulting_bit_values.find_first_not_of("0X") == std::string::npos)
                              {
                                 is_op1_null = true;
                              }
                              else
                              {
                                 const auto op1_expr = IRman->create_binary_operation(
                                     op1_type, me->op1, shift_constant_node, srcp_default, rshift_expr_K);
                                 const auto op1_ga = IRman->CreateGimpleAssign(op1_type, nullptr, nullptr, op1_expr,
                                                                               function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt, AppM);
                                 const auto op1_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op1_ga))->op0;
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Replacing " + me->op1->ToString() + " with " +
                                                    op1_ga_var->ToString() + " in " + stmt->ToString());
                                 TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned by this
                                  * bitshift must not be checked if it was applied to
                                  * a variable marked as address.
                                  */
                                 if(parameters->isOption(OPT_discrepancy) and
                                    parameters->getOption<bool>(OPT_discrepancy))
                                 {
                                    AppM->RDiscr->ssa_to_skip_if_address.insert(op1_ga_var);
                                 }
#endif
                                 /// set the bit_values to the ssa var
                                 auto* op1_ssa = GetPointer<ssa_name>(op1_ga_var);
                                 THROW_ASSERT(resulting_bit_values.size() > 0, "unexpected condition");
                                 op1_ssa->bit_values = resulting_bit_values;
                                 constrainSSA(op1_ssa, TM);
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "Var_uid: " +
                                                   AppM->CGetFunctionBehavior(function_id)
                                                       ->CGetBehavioralHelper()
                                                       ->PrintVariable(GET_INDEX_CONST_NODE(op1_ga_var)) +
                                                   " bitstring: " + STR(op1_ssa->bit_values));
                              }
                           }

                           tree_nodeRef curr_ga;
                           if(is_op0_null)
                           {
                              curr_ga = IRman->CreateGimpleAssign(op1_type, nullptr, nullptr, me->op1, function_id,
                                                                  srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                           }
                           else if(is_op1_null)
                           {
                              curr_ga = IRman->CreateGimpleAssign(op1_type, nullptr, nullptr, me->op0, function_id,
                                                                  srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                           }
                           else
                           {
                              curr_ga = IRman->CreateGimpleAssign(op1_type, nullptr, nullptr, ga->op1, function_id,
                                                                  srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                           }
                           B->PushBefore(curr_ga, stmt, AppM);
                           const auto curr_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(curr_ga))->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                           /*
                            * for discrepancy analysis, the ssa assigned by this
                            * bitshift must not be checked if it was applied to
                            * a variable marked as address.
                            */
                           if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                           {
                              AppM->RDiscr->ssa_to_skip_if_address.insert(curr_ga_var);
                           }
#endif
                           /// set the bit_values to the ssa var
                           auto op_ssa = GetPointer<ssa_name>(curr_ga_var);
                           THROW_ASSERT(ssa->bit_values.size() - shift_const > 0, "unexpected condition");
                           op_ssa->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - shift_const);
                           constrainSSA(op_ssa, TM);
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                         "Var_uid: " +
                                             AppM->CGetFunctionBehavior(function_id)
                                                 ->CGetBehavioralHelper()
                                                 ->PrintVariable(GET_INDEX_CONST_NODE(curr_ga_var)) +
                                             " bitstring: " + STR(op_ssa->bit_values));

                           const auto op_expr = IRman->create_binary_operation(
                               ga_op_type, curr_ga_var, shift_constant_node, srcp_default, lshift_expr_K);
                           const auto lshift_ga = IRman->CreateGimpleAssign(ga_op_type, nullptr, nullptr, op_expr,
                                                                            function_id, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(lshift_ga));
                           B->PushBefore(lshift_ga, stmt, AppM);
                           const auto lshift_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(lshift_ga))->op0;
                           /// set the bit_values to the ssa var
                           auto lshift_ssa = GetPointer<ssa_name>(lshift_ga_var);
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
                                                 ->PrintVariable(GET_INDEX_CONST_NODE(lshift_ga_var)) +
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
#if HAVE_FROM_DISCREPANCY_BUILT
                              /*
                               * for discrepancy analysis, the ssa assigned by this
                               * bitshift must not be checked if it was applied to
                               * a variable marked as address.
                               */
                              if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                              {
                                 AppM->RDiscr->ssa_to_skip_if_address.insert(lshift_ga_var);
                              }
#endif
                              if(GetPointer<const integer_cst>(GET_CONST_NODE(b_node)))
                              {
                                 const auto cst_val = tree_helper::GetConstValue(b_node, false);
                                 const auto b_node_val = TM->CreateUniqueIntegerCst(
                                     cst_val & ((integer_cst_t(1) << shift_const) - 1), b_type);
                                 TM->ReplaceTreeNode(stmt, ga->op1,
                                                     IRman->create_ternary_operation(
                                                         ga_op_type, lshift_ga_var, b_node_val, shift_constant_node,
                                                         srcp_default, bit_ior_concat_expr_K));
                              }
                              else
                              {
                                 const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(
                                     static_cast<long long int>((1ULL << shift_const) - 1), b_type);
                                 const auto band_expr = IRman->create_binary_operation(
                                     b_type, b_node, bit_mask_constant_node, srcp_default, bit_and_expr_K);
                                 const auto band_ga = IRman->CreateGimpleAssign(b_type, nullptr, nullptr, band_expr,
                                                                                function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(band_ga));
                                 B->PushBefore(band_ga, stmt, AppM);
                                 const auto band_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(band_ga))->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned by this
                                  * bitshift must not be checked if it was applied to
                                  * a variable marked as address.
                                  */
                                 if(parameters->isOption(OPT_discrepancy) and
                                    parameters->getOption<bool>(OPT_discrepancy))
                                 {
                                    AppM->RDiscr->ssa_to_skip_if_address.insert(band_ga_var);
                                 }
#endif
                                 /// set the bit_values to the ssa var
                                 auto band_ssa = GetPointer<ssa_name>(band_ga_var);
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
                                                       ->PrintVariable(GET_INDEX_CONST_NODE(band_ga_var)) +
                                                   " bitstring: " + STR(band_ssa->bit_values));

                                 const auto res_expr = IRman->create_ternary_operation(
                                     ga_op_type, lshift_ga_var, band_ga_var, shift_constant_node, srcp_default,
                                     bit_ior_concat_expr_K);
                                 TM->ReplaceTreeNode(stmt, ga->op1, res_expr);
                              }
                           }
                           else
                           {
                              PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Final or not performed: ");
                              TM->ReplaceTreeNode(stmt, ga->op1, lshift_ga_var);
                           }
                           /// set uses of stmt
                        }
                        else if(GET_CONST_NODE(ga->op1)->get_kind() == minus_expr_K &&
                                op0->get_kind() == integer_cst_K && tree_helper::GetConstValue(op0) == 0)
                        {
                           if(!parameters->isOption(OPT_use_ALUs) || !parameters->getOption<bool>(OPT_use_ALUs))
                           {
                              const auto srcp_default =
                                  ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              const auto res_expr =
                                  IRman->create_unary_operation(ga_op_type, me->op1, srcp_default, negate_expr_K);
                              TM->ReplaceTreeNode(stmt, ga->op1, res_expr);
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                        }
                     };
                     plus_minus_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == eq_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == ne_expr_K)
                  {
                     auto eq_ne_expr_BVO = [&] {
                        const auto me = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
                        const auto& op0 = me->op0;
                        const auto& op1 = me->op1;
                        if(tree_helper::IsRealType(op0) && tree_helper::IsRealType(op1))
                        {
                           // TODO: adapt existing operations to real type (zero sign bug to be considered)
                           return;
                        }
                        const auto op0_size = tree_helper::TypeSize(op0);
                        bool is_op1_zero = false;
                        if(GetPointer<const integer_cst>(GET_CONST_NODE(op1)))
                        {
                           if(tree_helper::GetConstValue(op1) == 0)
                           {
                              is_op1_zero = true;
                           }
                        }

                        if(op0->index == op1->index)
                        {
                           const auto const_value = GET_CONST_NODE(ga->op1)->get_kind() == eq_expr_K ? 1LL : 0LL;
                           const auto val = TM->CreateUniqueIntegerCst(const_value, ga_op_type);
                           propagateValue(ssa, TM, ga->op0, val, DEBUG_CALLSITE);
                        }
                        else if(is_op1_zero && GET_CONST_NODE(ga->op1)->get_kind() == ne_expr_K && op0_size == 1)
                        {
                           const auto op0_op_type = tree_helper::CGetType(op0);
                           const auto data_bitsize = tree_helper::TypeSize(op0_op_type);
                           if(data_bitsize == 1)
                           {
                              propagateValue(ssa, TM, ga->op0, me->op0, DEBUG_CALLSITE);
                              if(!ssa->CGetUseStmts().empty())
                              {
                                 if(AppM->ApplyNewTransformation())
                                 {
                                    const auto srcp_default =
                                        ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                                    const auto nop_expr_node =
                                        IRman->create_unary_operation(ga_op_type, me->op0, srcp_default, nop_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, nop_expr_node);
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                              }
                           }
                           else
                           {
                              const auto srcp_default =
                                  ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              const auto one_const_node = TM->CreateUniqueIntegerCst(1, op0_op_type);
                              const auto bitwise_masked = IRman->create_binary_operation(
                                  op0_op_type, me->op0, one_const_node, srcp_default, bit_and_expr_K);
                              const auto op0_ga =
                                  IRman->CreateGimpleAssign(op0_op_type, TM->CreateUniqueIntegerCst(0, op0_op_type),
                                                            TM->CreateUniqueIntegerCst(1LL, op0_op_type),
                                                            bitwise_masked, function_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                              B->PushBefore(op0_ga, stmt, AppM);
                              const auto op0_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;

                              const auto ga_nop = IRman->CreateNopExpr(op0_ga_var, ga_op_type, tree_nodeRef(),
                                                                       tree_nodeRef(), function_id);
                              B->PushBefore(ga_nop, stmt, AppM);
                              modified = true;
                              AppM->RegisterTransformation(GetName(), ga_nop);
                              const auto nop_ga_var = GetPointer<const gimple_assign>(GET_CONST_NODE(ga_nop))->op0;
                              TM->ReplaceTreeNode(stmt, ga->op1, nop_ga_var);
                           }
                        }
                        else
                        {
                           rel_expr_BVO();
                        }
                     };
                     eq_ne_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == lt_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == gt_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == le_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == ge_expr_K)
                  {
                     rel_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == bit_and_expr_K ||
                          GET_CONST_NODE(ga->op1)->get_kind() == bit_xor_expr_K)
                  {
                     auto bit_expr_BVO = [&] {
                        const auto me = GetPointer<const binary_expr>(GET_CONST_NODE(ga->op1));
                        const auto& op0 = me->op0;
                        const auto& op1 = me->op1;
                        const auto expr_kind = GET_CONST_NODE(ga->op1)->get_kind();

                        std::string s0, s1;
                        bool is_op0_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(op0));
                        bool is_op1_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(op1));
                        if(is_op0_ssa)
                        {
                           s0 = GetPointer<const ssa_name>(GET_CONST_NODE(op0))->bit_values;
                           if(s0.empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Skipped because bitvalue of s0 is empty");
                              return;
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---s0: " + s0);
                        }
                        else
                        {
                           THROW_ASSERT(GET_CONST_NODE(op0)->get_kind() == integer_cst_K, "unexpected case");
                        }
                        if(is_op1_ssa)
                        {
                           s1 = GetPointer<const ssa_name>(GET_CONST_NODE(op1))->bit_values;
                           if(s1.empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Skipped because bitvalue of s1 is empty");
                              return;
                           }
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---s1: " + s1);
                        }
                        else
                        {
                           THROW_ASSERT(GET_CONST_NODE(op1)->get_kind() == integer_cst_K, "unexpected case");
                        }

                        unsigned int trailing_zero = 0;
                        bool is_zero0 = s0.find_first_not_of("0X") == std::string::npos;
                        if(!is_op0_ssa)
                        {
                           const auto uvalue = tree_helper::GetConstValue(op0, false);
                           is_zero0 = uvalue == 0;
                           s0 = convert_to_binary(uvalue, tree_helper::TypeSize(op0));
                        }
                        bool is_zero1 = s1.find_first_not_of("0X") == std::string::npos;
                        if(!is_op1_ssa)
                        {
                           const auto uvalue = tree_helper::GetConstValue(op1, false);
                           is_zero1 = uvalue == 0;
                           s1 = convert_to_binary(uvalue, tree_helper::TypeSize(op1));
                        }
                        if(is_zero0 || is_zero1)
                        {
                           if(GET_CONST_NODE(ga->op1)->get_kind() == bit_and_expr_K)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace bit_and_expr usage before: " + stmt->ToString());
                              const auto zero_node = TM->CreateUniqueIntegerCst(0LL, tree_helper::CGetType(op0));
                              propagateValue(ssa, TM, ga->op0, zero_node, DEBUG_CALLSITE);
                           }
                           else
                           {
                              // bit_xor_expr_K
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace bit_xor_expr usage before: " + stmt->ToString());
                              const auto val = is_zero0 ? op1 : op0;
                              condPropageValue(val, DEBUG_CALLSITE);
                           }
                        }
                        else
                        {
                           for(auto s0it = s0.rbegin(), s1it = s1.rbegin(), s0end = s0.rend(), s1end = s1.rend();
                               s0it != s0end && s1it != s1end; ++s0it, ++s1it)
                           {
                              if((expr_kind == bit_and_expr_K && (*s0it == '0' || *s1it == '0')) || *s0it == 'X' ||
                                 *s1it == 'X')
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
                                             "---Bit Value Opt: " + std::string(ga->op1->get_kind_text()) +
                                                 " optimized, nbits = " + STR(trailing_zero));
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                              const auto srcp_default =
                                  ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              const auto op0_op_type = tree_helper::CGetType(op0);
                              const auto op1_op_type = tree_helper::CGetType(op1);

                              if(is_op0_ssa)
                              {
                                 const auto op0_const_node = TM->CreateUniqueIntegerCst(trailing_zero, op0_op_type);
                                 const auto op0_expr = IRman->create_binary_operation(op0_op_type, op0, op0_const_node,
                                                                                      srcp_default, rshift_expr_K);
                                 const auto op0_ga = IRman->CreateGimpleAssign(op0_op_type, nullptr, nullptr, op0_expr,
                                                                               function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt, AppM);
                                 const auto op0_ga_var = GetPointerS<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, op0, op0_ga_var);
                                 /// set the bit_values to the ssa var
                                 auto op0_ssa = GetPointerS<ssa_name>(op0_ga_var);
                                 THROW_ASSERT(s0.size() - trailing_zero > 0, "unexpected condition");
                                 op0_ssa->bit_values = s0.substr(0, s0.size() - trailing_zero);
                                 constrainSSA(op0_ssa, TM);
                              }
                              else
                              {
                                 const auto cst_val =
                                     tree_helper::GetConstValue(op0, tree_helper::IsSignedIntegerType(op0));
                                 TM->ReplaceTreeNode(stmt, op0,
                                                     TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op0_op_type));
                              }
                              if(is_op1_ssa)
                              {
                                 const auto op1_const_node =
                                     TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), op1_op_type);
                                 const auto op1_expr = IRman->create_binary_operation(op1_op_type, op1, op1_const_node,
                                                                                      srcp_default, rshift_expr_K);
                                 const auto op1_ga = IRman->CreateGimpleAssign(op1_op_type, nullptr, nullptr, op1_expr,
                                                                               function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt, AppM);
                                 const auto op1_ga_var = GetPointerS<const gimple_assign>(GET_CONST_NODE(op1_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, op1, op1_ga_var);
                                 /// set the bit_values to the ssa var
                                 auto op1_ssa = GetPointerS<ssa_name>(op1_ga_var);
                                 THROW_ASSERT(s1.size() - trailing_zero > 0, "unexpected condition");
                                 op1_ssa->bit_values = s1.substr(0, s1.size() - trailing_zero);
                                 constrainSSA(op1_ssa, TM);
                              }
                              else
                              {
                                 const auto cst_val =
                                     tree_helper::GetConstValue(op1, tree_helper::IsSignedIntegerType(op1));
                                 TM->ReplaceTreeNode(stmt, op1,
                                                     TM->CreateUniqueIntegerCst(cst_val >> trailing_zero, op1_op_type));
                              }

                              const auto ssa_vd = IRman->create_ssa_name(nullptr, ga_op_type, nullptr, nullptr);
                              auto sn = GetPointerS<ssa_name>(ssa_vd);
                              /// set the bit_values to the ssa var
                              THROW_ASSERT(ssa->bit_values.size() - trailing_zero > 0, "unexpected condition");
                              sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_zero);
                              constrainSSA(sn, TM);
                              const auto op_const_node =
                                  TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), ga_op_type);
                              const auto op_expr = IRman->create_binary_operation(ga_op_type, ssa_vd, op_const_node,
                                                                                  srcp_default, lshift_expr_K);
                              const auto curr_ga = IRman->CreateGimpleAssign(ga_op_type, ssa->min, ssa->max, op_expr,
                                                                             function_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                              TM->ReplaceTreeNode(
                                  curr_ga, GetPointer<const gimple_assign>(GET_CONST_NODE(curr_ga))->op0, ga->op0);
                              TM->ReplaceTreeNode(stmt, ga->op0, ssa_vd);
                              B->PushAfter(curr_ga, stmt, AppM);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "pushed");
                           }
                        }
                     };
                     bit_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == cond_expr_K)
                  {
                     auto cond_expr_BVO = [&] {
                        const auto me = GetPointer<const cond_expr>(GET_CONST_NODE(ga->op1));
                        if(!tree_helper::IsBooleanType(me->op0))
                        {
                           /// try to fix cond_expr condition
                           const auto cond_op0_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(me->op0));
                           if(cond_op0_ssa)
                           {
                              const auto defStmt = GET_CONST_NODE(cond_op0_ssa->CGetDefStmt());
                              if(defStmt->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(defStmt);
                                 const auto prev_code1 = GET_CONST_NODE(prev_ga->op1)->get_kind();
                                 if(prev_code1 == nop_expr_K)
                                 {
                                    const auto ne = GetPointer<const nop_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(tree_helper::IsBooleanType(ne->op))
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace cond_expr condition before: " + stmt->ToString());
                                       TM->ReplaceTreeNode(stmt, me->op0, ne->op);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace cond_expr condition after: " + stmt->ToString());
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
                        const auto condition = GET_CONST_NODE(me->op0);
                        if(GET_INDEX_CONST_NODE(op0) == GET_INDEX_CONST_NODE(op1))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with equal operands");
                           condPropageValue(me->op1, DEBUG_CALLSITE);
                        }
                        else if(condition->get_kind() == integer_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---Cond expr with constant condition");
                           const auto val = tree_helper::GetConstValue(me->op0, false) ? me->op1 : me->op2;

                           condPropageValue(val, DEBUG_CALLSITE);
                        }
                        else
                        {
                           THROW_ASSERT(op0 != op1, "unexpected condition");
                           std::string s0, s1;
                           bool is_op0_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(op0));
                           bool is_op1_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(op1));
                           unsigned long long s0_precision = 0;
                           bool is_s0_null = false;
                           bool is_s0_one = false;
                           if(is_op0_ssa)
                           {
                              s0 = GetPointerS<const ssa_name>(GET_CONST_NODE(op0))->bit_values;
                              if(s0.empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Skipped because so is empty");
                                 return;
                              }
                              s0_precision = static_cast<unsigned int>(s0.size());
                              is_s0_null = s0.find_first_not_of("0X") == std::string::npos;
                              is_s0_one = s0 == "1" || s0 == "01";
                           }
                           else
                           {
                              THROW_ASSERT(GET_CONST_NODE(op0)->get_kind() == integer_cst_K, "unexpected condition");
                              const auto uvalue = tree_helper::GetConstValue(op0, false);
                              s0_precision =
                                  uvalue ? tree_helper::Size(GET_CONST_NODE(op0)) : tree_helper::TypeSize(op0);
                              s0 = convert_to_binary(uvalue, s0_precision);
                              is_s0_null = uvalue == 0;
                              is_s0_one = uvalue == 1;
                           }
                           unsigned long long s1_precision = 0;
                           bool is_s1_null = false;
                           bool is_s1_one = false;
                           if(is_op1_ssa)
                           {
                              s1 = GetPointerS<const ssa_name>(GET_CONST_NODE(op1))->bit_values;
                              if(s1.empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---Skipped because s1 is empty");
                                 return;
                              }
                              s1_precision = static_cast<unsigned int>(s1.size());
                              is_s1_null = s1.find_first_not_of("0X") == std::string::npos;
                              is_s1_one = s1 == "1" || s1 == "01";
                           }
                           else
                           {
                              THROW_ASSERT(GET_CONST_NODE(op1)->get_kind() == integer_cst_K, "unexpected condition");
                              const auto uvalue = tree_helper::GetConstValue(op1, false);
                              s1_precision =
                                  uvalue ? tree_helper::Size(GET_CONST_NODE(op1)) : tree_helper::TypeSize(op1);
                              s1 = convert_to_binary(uvalue, s1_precision);
                              is_s1_null = uvalue == 0;
                              is_s1_one = uvalue == 1;
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
                                             "---Bit Value Opt: cond_expr optimized, nbits = " + STR(trailing_eq));
                              modified = true;
                              const auto srcp_default =
                                  ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              const auto op0_op_type = tree_helper::CGetType(op0);
                              const auto op1_op_type = tree_helper::CGetType(op1);

                              if(is_op0_ssa)
                              {
                                 const auto op0_const_node =
                                     TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), op0_op_type);
                                 const auto op0_expr = IRman->create_binary_operation(
                                     op0_op_type, me->op1, op0_const_node, srcp_default, rshift_expr_K);
                                 const auto op0_ga = IRman->CreateGimpleAssign(op0_op_type, nullptr, nullptr, op0_expr,
                                                                               function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt, AppM);
                                 const auto op0_ga_var = GetPointerS<const gimple_assign>(GET_CONST_NODE(op0_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op1, op0_ga_var);
                                 /// set the bit_values to the ssa var
                                 auto op0_ssa = GetPointerS<ssa_name>(op0_ga_var);
                                 THROW_ASSERT(s0.size() - trailing_eq > 0, "unexpected condition");
                                 op0_ssa->bit_values = s0.substr(0, s0.size() - trailing_eq);
                                 constrainSSA(op0_ssa, TM);
                              }
                              else
                              {
                                 const auto cst_val =
                                     tree_helper::GetConstValue(op0, tree_helper::IsSignedIntegerType(op0));
                                 TM->ReplaceTreeNode(stmt, me->op1,
                                                     TM->CreateUniqueIntegerCst(cst_val >> trailing_eq, op0_op_type));
                              }
                              if(is_op1_ssa)
                              {
                                 const auto op1_const_node =
                                     TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), op1_op_type);
                                 const auto op1_expr = IRman->create_binary_operation(
                                     op1_op_type, me->op2, op1_const_node, srcp_default, rshift_expr_K);
                                 const auto op1_ga = IRman->CreateGimpleAssign(
                                     op1_op_type, tree_nodeRef(), tree_nodeRef(), op1_expr, function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt, AppM);
                                 const auto op1_ga_var = GetPointerS<const gimple_assign>(GET_CONST_NODE(op1_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op2, op1_ga_var);
                                 /// set the bit_values to the ssa var
                                 auto op1_ssa = GetPointerS<ssa_name>(op1_ga_var);
                                 THROW_ASSERT(s1.size() - trailing_eq > 0, "unexpected condition");
                                 op1_ssa->bit_values = s1.substr(0, s1.size() - trailing_eq);
                                 constrainSSA(op1_ssa, TM);
                              }
                              else
                              {
                                 const auto cst_val =
                                     tree_helper::GetConstValue(op1, tree_helper::IsSignedIntegerType(op1));
                                 TM->ReplaceTreeNode(stmt, me->op2,
                                                     TM->CreateUniqueIntegerCst(cst_val >> trailing_eq, op1_op_type));
                              }
                              const auto ssa_vd = IRman->create_ssa_name(nullptr, ga_op_type, nullptr, nullptr);
                              auto* sn = GetPointer<ssa_name>(ssa_vd);
                              /// set the bit_values to the ssa var
                              if(ssa->bit_values.size())
                              {
                                 THROW_ASSERT(ssa->bit_values.size() - trailing_eq > 0, "unexpected condition");
                                 sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_eq);
                                 constrainSSA(sn, TM);
                              }
                              const auto op_const_node =
                                  TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), ga_op_type);
                              const auto op_expr = IRman->create_binary_operation(ga_op_type, ssa_vd, op_const_node,
                                                                                  srcp_default, lshift_expr_K);
                              const auto curr_ga = IRman->CreateGimpleAssign(ga_op_type, ssa->min, ssa->max, op_expr,
                                                                             function_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                              TM->ReplaceTreeNode(
                                  curr_ga, GetPointer<const gimple_assign>(GET_CONST_NODE(curr_ga))->op0, ga->op0);
                              TM->ReplaceTreeNode(stmt, ga->op0, ssa_vd);
                              B->PushAfter(curr_ga, stmt, AppM);
                              modified = true;
                              AppM->RegisterTransformation(GetName(), stmt);
                           }
                           else if(is_s0_one && is_s1_null)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Cond expr with true and false");
                              tree_nodeRef cond_var;
                              if(tree_helper::IsBooleanType(ssa->type))
                              {
                                 cond_var = me->op0;
                              }
                              else
                              {
                                 const auto ga_nop =
                                     IRman->CreateNopExpr(me->op0, ssa->type, ssa->min, ssa->max, function_id);
                                 B->PushBefore(ga_nop, stmt, AppM);
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), ga_nop);
                                 cond_var = GetPointer<const gimple_assign>(GET_CONST_NODE(ga_nop))->op0;
                              }
                              condPropageValue(cond_var, DEBUG_CALLSITE);
                           }
                           else if(is_s0_null && is_s1_one)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---Cond expr with false and true");
                              /// second argument is null since we cannot add the new statement at the end of the
                              /// current BB
                              const auto new_ssa = IRman->CreateNotExpr(me->op0, blocRef(), function_id);
                              const auto new_stmt = GetPointer<const ssa_name>(GET_CONST_NODE(new_ssa))->CGetDefStmt();
                              B->PushBefore(new_stmt, stmt, AppM);
                              tree_nodeRef cond_var;
                              if(tree_helper::IsBooleanType(ssa->type))
                              {
                                 cond_var = new_ssa;
                              }
                              else
                              {
                                 const auto ga_nop =
                                     IRman->CreateNopExpr(new_ssa, ssa->type, ssa->min, ssa->max, function_id);
                                 B->PushBefore(ga_nop, stmt, AppM);
                                 cond_var = GetPointer<const gimple_assign>(GET_CONST_NODE(ga_nop))->op0;
                              }
                              condPropageValue(cond_var, DEBUG_CALLSITE);
                           }
                        }
                     };
                     cond_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == bit_ior_expr_K)
                  {
                     auto bit_ior_expr_BVO = [&] {
                        const auto bie = GetPointer<const bit_ior_expr>(GET_CONST_NODE(ga->op1));
                        if(GET_CONST_NODE(bie->op0)->get_kind() == integer_cst_K ||
                           GET_CONST_NODE(bie->op1)->get_kind() == integer_cst_K)
                        {
                           tree_nodeRef val;
                           if(GET_CONST_NODE(bie->op0)->get_kind() == integer_cst_K)
                           {
                              const auto cst_val = tree_helper::GetConstValue(bie->op0);
                              if(cst_val == 0)
                              {
                                 val = bie->op1;
                              }
                           }
                           else
                           {
                              const auto cst_val = tree_helper::GetConstValue(bie->op1);
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
                     };
                     bit_ior_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == pointer_plus_expr_K)
                  {
                     auto pointer_plus_expr_BVO = [&] {
                        const auto ppe = GetPointer<const pointer_plus_expr>(GET_CONST_NODE(ga->op1));
                        if(GET_CONST_NODE(ppe->op1)->get_kind() == integer_cst_K)
                        {
                           const auto cst_val = tree_helper::GetConstValue(ppe->op1);
                           if(cst_val == 0)
                           {
                              condPropageValue(ppe->op0, DEBUG_CALLSITE);
                           }
                           else if(GetPointer<const ssa_name>(GET_CONST_NODE(ppe->op0)))
                           {
                              const auto temp_def =
                                  GET_CONST_NODE(GetPointer<const ssa_name>(GET_CONST_NODE(ppe->op0))->CGetDefStmt());
                              if(temp_def->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(temp_def);
                                 if(GET_CONST_NODE(prev_ga->op1)->get_kind() == pointer_plus_expr_K)
                                 {
                                    const auto prev_ppe =
                                        GetPointer<const pointer_plus_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(GetPointer<const ssa_name>(GET_CONST_NODE(prev_ppe->op0)) &&
                                       GetPointer<const integer_cst>(GET_CONST_NODE(prev_ppe->op1)))
                                    {
                                       const auto prev_val = tree_helper::GetConstValue(prev_ppe->op1);
                                       const auto new_offset = TM->CreateUniqueIntegerCst(
                                           (prev_val + cst_val), tree_helper::CGetType(ppe->op1));
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace constant usage before: " + stmt->ToString());
                                       TM->ReplaceTreeNode(stmt, ppe->op1, new_offset);
                                       TM->ReplaceTreeNode(stmt, ppe->op0, prev_ppe->op0);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace constant usage after: " + stmt->ToString());
                                       modified = true;
                                       AppM->RegisterTransformation(GetName(), stmt);
                                    }
                                 }
                              }
                           }
                        }
                     };
                     pointer_plus_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == addr_expr_K)
                  {
                     auto addr_expr_BVO = [&] {
                        const auto ae = GetPointer<const addr_expr>(GET_CONST_NODE(ga->op1));
                        const auto ae_code = GET_CONST_NODE(ae->op)->get_kind();
                        if(ae_code == mem_ref_K)
                        {
                           const auto MR = GetPointer<const mem_ref>(GET_CONST_NODE(ae->op));
                           const auto op1_val = tree_helper::GetConstValue(MR->op1);
                           if(op1_val == 0 && GET_CONST_NODE(MR->op0)->get_kind() == ssa_name_K)
                           {
                              const auto temp_def = GetPointer<const ssa_name>(MR->op0)->CGetDefStmt();
                              if(temp_def->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(temp_def);
                                 if(GET_CONST_NODE(prev_ga->op1)->get_kind() == addr_expr_K)
                                 {
                                    condPropageValue(prev_ga->op0, DEBUG_CALLSITE);
                                 }
                              }
                           }
                           else if(op1_val == 0 && GET_CONST_NODE(MR->op0)->get_kind() == integer_cst_K)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace constant usage before: " + stmt->ToString());
                              TM->ReplaceTreeNode(stmt, ga->op1, MR->op0);
                              modified = true;
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---replace constant usage after: " + stmt->ToString());
                           }
                        }
                     };
                     addr_expr_BVO();
                  }
                  else if(ga->op1->get_kind() == extract_bit_expr_K)
                  {
                     auto extract_bit_expr_BVO = [&] {
                        const auto srcp_default =
                            ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                        const auto ebe = GetPointer<const extract_bit_expr>(GET_CONST_NODE(ga->op1));
                        THROW_ASSERT(GET_CONST_NODE(ebe->op1)->get_kind() == integer_cst_K, "unexpected condition");
                        const auto pos_value = tree_helper::GetConstValue(ebe->op1);
                        THROW_ASSERT(pos_value >= 0, "");
                        const auto ebe_op0_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(ebe->op0));
                        if(ebe_op0_ssa)
                        {
                           if(static_cast<integer_cst_t>(tree_helper::TypeSize(ebe->op0)) <= pos_value)
                           {
                              const auto right_id = GET_INDEX_CONST_NODE(ebe->op0);
                              const bool right_signed = tree_helper::is_int(TM, right_id);
                              if(right_signed)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_expr usage before: " + stmt->ToString());
                                 const auto new_pos = TM->CreateUniqueIntegerCst(
                                     static_cast<integer_cst_t>(tree_helper::TypeSize(ebe->op0) - 1),
                                     tree_helper::CGetType(ebe->op1));
                                 const auto eb_op = IRman->create_extract_bit_expr(ebe->op0, new_pos, srcp_default);
                                 const auto eb_ga = IRman->CreateGimpleAssign(
                                     ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                     TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op, function_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                 B->PushBefore(eb_ga, stmt, AppM);
                                 const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                 const auto masking = IRman->create_binary_operation(
                                     ebe->type, GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                     bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                 TM->ReplaceTreeNode(
                                     stmt, ga->op1,
                                     masking); /// replaced with redundant code to restart lut_transformation
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_expr usage after: " + stmt->ToString());
                                 modified = true;
                                 AppM->RegisterTransformation(GetName(), stmt);
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---replace extract_bit_expr usage before: " + stmt->ToString());
                                 const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                 propagateValue(ssa, TM, ga->op0, zero_node, DEBUG_CALLSITE);
                              }
                           }
                           else if(tree_helper::IsBooleanType(ebe->op0))
                           {
                              propagateValue(ssa, TM, ga->op0, ebe->op0, DEBUG_CALLSITE);
                           }
                           else
                           {
                              auto defStmt = GET_CONST_NODE(ebe_op0_ssa->CGetDefStmt());
                              if(defStmt->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(defStmt);
                                 auto prev_code1 = GET_CONST_NODE(prev_ga->op1)->get_kind();
                                 if(prev_code1 == nop_expr_K || prev_code1 == convert_expr_K)
                                 {
                                    auto ne = GetPointer<const unary_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(tree_helper::IsBooleanType(ne->op))
                                    {
                                       if(pos_value == 0)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage before: " +
                                                             stmt->ToString());
                                          const auto bit_mask_constant_node =
                                              TM->CreateUniqueIntegerCst(1LL, ebe->type);
                                          const auto masking =
                                              IRman->create_binary_operation(ebe->type, ne->op, bit_mask_constant_node,
                                                                             srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(
                                              stmt, ga->op1,
                                              masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage after: " +
                                                             stmt->ToString());
                                          modified = true;
                                          AppM->RegisterTransformation(GetName(), stmt);
                                       }
                                       else
                                       {
                                          const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                          propagateValue(ssa, TM, ga->op0, zero_node, DEBUG_CALLSITE);
                                       }
                                    }
                                    else
                                    {
                                       const auto neType_node = tree_helper::CGetType(ne->op);
                                       if(GET_CONST_NODE(neType_node)->get_kind() == integer_type_K)
                                       {
                                          if(static_cast<integer_cst_t>(tree_helper::TypeSize(ne->op)) > pos_value)
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---replace extract_bit_expr usage before: " +
                                                                stmt->ToString());
                                             const auto eb_op =
                                                 IRman->create_extract_bit_expr(ne->op, ebe->op1, srcp_default);
                                             const auto eb_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0LL, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(eb_ga));
                                             B->PushBefore(eb_ga, stmt, AppM);
                                             const auto bit_mask_constant_node =
                                                 TM->CreateUniqueIntegerCst(1LL, ebe->type);
                                             const auto masking = IRman->create_binary_operation(
                                                 ebe->type, GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                                 bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                             TM->ReplaceTreeNode(stmt, ga->op1,
                                                                 masking); /// replaced with redundant code to restart
                                                                           /// lut_transformation
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---replace extract_bit_expr usage after: " +
                                                                stmt->ToString());
                                             modified = true;
                                             AppM->RegisterTransformation(GetName(), stmt);
                                          }
                                          else
                                          {
                                             const bool right_signed = tree_helper::IsSignedIntegerType(ne->op);
                                             if(right_signed)
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---replace extract_bit_expr usage before: " +
                                                                   stmt->ToString());
                                                const auto new_pos = TM->CreateUniqueIntegerCst(
                                                    static_cast<integer_cst_t>(tree_helper::TypeSize(ne->op) - 1),
                                                    tree_helper::CGetType(ebe->op1));
                                                const auto eb_op =
                                                    IRman->create_extract_bit_expr(ne->op, new_pos, srcp_default);
                                                const auto eb_ga = IRman->CreateGimpleAssign(
                                                    ebe->type, TM->CreateUniqueIntegerCst(0LL, ebe->type),
                                                    TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op, function_id,
                                                    srcp_default);
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---Created " + STR(eb_ga));
                                                B->PushBefore(eb_ga, stmt, AppM);
                                                const auto bit_mask_constant_node =
                                                    TM->CreateUniqueIntegerCst(1LL, ebe->type);
                                                const auto masking = IRman->create_binary_operation(
                                                    ebe->type,
                                                    GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                                    bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                                TM->ReplaceTreeNode(stmt, ga->op1,
                                                                    masking); /// replaced with redundant code to
                                                                              /// restart lut_transformation
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---replace extract_bit_expr usage after: " +
                                                                   stmt->ToString());
                                                modified = true;
                                                AppM->RegisterTransformation(GetName(), stmt);
                                             }
                                             else
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---replace extract_bit_expr usage before: " +
                                                                   stmt->ToString());
                                                const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                                propagateValue(ssa, TM, ga->op0, zero_node, DEBUG_CALLSITE);
                                             }
                                          }
                                       }
                                    }
                                 }
                                 else if(prev_code1 == bit_and_expr_K)
                                 {
                                    const auto bae = GetPointerS<const bit_and_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(GET_CONST_NODE(bae->op0)->get_kind() == integer_cst_K ||
                                       GET_CONST_NODE(bae->op1)->get_kind() == integer_cst_K)
                                    {
                                       auto bae_op0 = bae->op0;
                                       auto bae_op1 = bae->op1;
                                       if(GET_CONST_NODE(bae->op0)->get_kind() == integer_cst_K)
                                       {
                                          std::swap(bae_op0, bae_op1);
                                       }
                                       const auto bae_mask_value = tree_helper::GetConstValue(bae_op1);
                                       const auto masked_value = (bae_mask_value & (integer_cst_t(1) << pos_value));
                                       if(masked_value && GET_CONST_NODE(bae_op0)->get_kind() != integer_cst_K)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage before: " +
                                                             stmt->ToString());
                                          const auto eb_op =
                                              IRman->create_extract_bit_expr(bae_op0, ebe->op1, srcp_default);
                                          const auto eb_ga = IRman->CreateGimpleAssign(
                                              ebe->type, TM->CreateUniqueIntegerCst(0LL, ebe->type),
                                              TM->CreateUniqueIntegerCst(1LL, ebe->type), eb_op, function_id,
                                              srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---Created " + STR(eb_ga));
                                          B->PushBefore(eb_ga, stmt, AppM);
                                          const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                          const auto masking = IRman->create_binary_operation(
                                              ebe->type, GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                              bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(
                                              stmt, ga->op1,
                                              masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage after: " +
                                                             stmt->ToString());
                                          modified = true;
                                          AppM->RegisterTransformation(GetName(), stmt);
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage before: " +
                                                             stmt->ToString());
                                          const auto zero_node =
                                              TM->CreateUniqueIntegerCst(masked_value ? 1 : 0, ebe->type);
                                          propagateValue(ssa, TM, ga->op0, zero_node, DEBUG_CALLSITE);
                                       }
                                    }
                                 }
                                 else if(prev_code1 == bit_ior_concat_expr_K)
                                 {
                                    const auto bice =
                                        GetPointerS<const bit_ior_concat_expr>(GET_CONST_NODE(prev_ga->op1));
                                    THROW_ASSERT(bice->op2->get_kind() == integer_cst_K, "unexpected condition");
                                    const auto nbit_value = tree_helper::GetConstValue(bice->op2);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto eb_op = IRman->create_extract_bit_expr(
                                        nbit_value > pos_value ? bice->op1 : bice->op0, ebe->op1, srcp_default);
                                    const auto eb_ga = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt, AppM);
                                    const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                    const auto masking = IRman->create_binary_operation(
                                        ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                        bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                    TM->ReplaceTreeNode(
                                        stmt, ga->op1,
                                        masking); /// replaced with redundant code to restart lut_transformation
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else if(prev_code1 == lshift_expr_K)
                                 {
                                    const auto lse = GetPointerS<const lshift_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(GET_CONST_NODE(lse->op1)->get_kind() == integer_cst_K)
                                    {
                                       const auto lsbit_value = tree_helper::GetConstValue(lse->op1);
                                       if((pos_value - lsbit_value) >= 0)
                                       {
                                          const auto new_pos = TM->CreateUniqueIntegerCst(
                                              pos_value - lsbit_value, tree_helper::CGetType(ebe->op1));
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage before: " +
                                                             stmt->ToString());
                                          const auto eb_op =
                                              IRman->create_extract_bit_expr(lse->op0, new_pos, srcp_default);
                                          const auto eb_ga = IRman->CreateGimpleAssign(
                                              ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                              TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id,
                                              srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---Created " + STR(eb_ga));
                                          B->PushBefore(eb_ga, stmt, AppM);
                                          const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                          const auto masking = IRman->create_binary_operation(
                                              ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                              bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(
                                              stmt, ga->op1,
                                              masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage after: " +
                                                             stmt->ToString());
                                          modified = true;
                                          AppM->RegisterTransformation(GetName(), stmt);
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage before: " +
                                                             stmt->ToString());
                                          const auto zero_node = TM->CreateUniqueIntegerCst(0LL, ebe->type);
                                          propagateValue(ssa, TM, ga->op0, zero_node, DEBUG_CALLSITE);
                                       }
                                    }
                                 }
                                 else if(prev_code1 == rshift_expr_K)
                                 {
                                    const auto rse = GetPointerS<const rshift_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(GET_CONST_NODE(rse->op1)->get_kind() == integer_cst_K)
                                    {
                                       const auto rsbit_value = tree_helper::GetConstValue(rse->op1);
                                       THROW_ASSERT((pos_value + rsbit_value) >= 0, "unexpected condition");
                                       const auto new_pos = TM->CreateUniqueIntegerCst(pos_value + rsbit_value,
                                                                                       tree_helper::CGetType(ebe->op1));
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_expr usage before: " + stmt->ToString());
                                       const auto eb_op =
                                           IRman->create_extract_bit_expr(rse->op0, new_pos, srcp_default);
                                       const auto eb_ga = IRman->CreateGimpleAssign(
                                           ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                           TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, srcp_default);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---Created " + STR(eb_ga));
                                       B->PushBefore(eb_ga, stmt, AppM);
                                       const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ebe->type);
                                       const auto masking = IRman->create_binary_operation(
                                           ebe->type, GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                           bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                       TM->ReplaceTreeNode(
                                           stmt, ga->op1,
                                           masking); /// replaced with redundant code to restart lut_transformation
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_expr usage after: " + stmt->ToString());
                                       modified = true;
                                       AppM->RegisterTransformation(GetName(), stmt);
                                    }
                                    else if(GET_CONST_NODE(rse->op0)->get_kind() == integer_cst_K)
                                    {
                                       const auto res_value =
                                           tree_helper::GetConstValue(rse->op0,
                                                                      tree_helper::IsSignedIntegerType(rse->op0)) >>
                                           pos_value;
                                       if(res_value)
                                       {
                                          if(max_lut_size > 0)
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---replace extract_bit_expr usage before: " +
                                                                stmt->ToString());
                                             const auto precision = tree_helper::TypeSize(ebe_op0_ssa->type);
                                             unsigned int log2;
                                             for(log2 = 1; precision > (1u << log2); ++log2)
                                             {
                                                ;
                                             }
                                             tree_nodeRef op1, op2, op3, op4, op5, op6, op7, op8;
                                             for(auto i = 0u; i < log2; ++i)
                                             {
                                                const auto new_pos =
                                                    TM->CreateUniqueIntegerCst(i, tree_helper::CGetType(ebe->op1));
                                                const auto eb_op =
                                                    IRman->create_extract_bit_expr(rse->op1, new_pos, srcp_default);
                                                const auto eb_ga = IRman->CreateGimpleAssign(
                                                    ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                    TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id,
                                                    srcp_default);
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---Created " + STR(eb_ga));
                                                B->PushBefore(eb_ga, stmt, AppM);
                                                const auto eb_ga_ssa_var =
                                                    GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0;
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

                                             const auto lut_constant_node =
                                                 TM->CreateUniqueIntegerCst(res_value, LutConstType);
                                             const auto eb_op =
                                                 IRman->create_lut_expr(ebe->type, lut_constant_node, op1, op2, op3,
                                                                        op4, op5, op6, op7, op8, srcp_default);
                                             const auto eb_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(eb_ga));
                                             B->PushBefore(eb_ga, stmt, AppM);
                                             const auto bit_mask_constant_node =
                                                 TM->CreateUniqueIntegerCst(1, ebe->type);
                                             const auto masking = IRman->create_binary_operation(
                                                 ebe->type,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                                 bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                             TM->ReplaceTreeNode(stmt, ga->op1,
                                                                 masking); /// replaced with redundant code to restart
                                                                           /// lut_transformation
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---replace extract_bit_expr usage after: " +
                                                                stmt->ToString());
                                             modified = true;
                                             AppM->RegisterTransformation(GetName(), stmt);
                                          }
                                          else
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---No lut allowed, lut replacement skipped...");
                                          }
                                       }
                                       else
                                       {
                                          const auto zero_node = TM->CreateUniqueIntegerCst(0, ebe->type);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage before: " +
                                                             stmt->ToString());
                                          TM->ReplaceTreeNode(stmt, ga->op1, zero_node);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---replace extract_bit_expr usage after: " +
                                                             stmt->ToString());
                                          modified = true;
                                          AppM->RegisterTransformation(GetName(), stmt);
                                       }
                                    }
                                 }
                                 else if(prev_code1 == bit_not_expr_K)
                                 {
                                    const auto bne = GetPointerS<const bit_not_expr>(GET_CONST_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto eb_op = IRman->create_extract_bit_expr(bne->op, ebe->op1, srcp_default);
                                    const auto eb_ga = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt, AppM);
                                    const auto negating = IRman->create_unary_operation(
                                        ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                        srcp_default, truth_not_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, negating);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else if(prev_code1 == bit_and_expr_K)
                                 {
                                    const auto bae = GetPointerS<const bit_and_expr>(GET_CONST_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto eb_op0 =
                                        IRman->create_extract_bit_expr(bae->op0, ebe->op1, srcp_default);
                                    const auto eb_ga0 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op0, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                                    B->PushBefore(eb_ga0, stmt, AppM);
                                    const auto eb_op1 =
                                        IRman->create_extract_bit_expr(bae->op1, ebe->op1, srcp_default);
                                    const auto eb_ga1 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt, AppM);
                                    const auto anding = IRman->create_binary_operation(
                                        ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga0))->op0,
                                        GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0, srcp_default,
                                        truth_and_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, anding);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else if(prev_code1 == bit_ior_expr_K)
                                 {
                                    const auto bie = GetPointerS<const bit_ior_expr>(GET_CONST_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto eb_op0 =
                                        IRman->create_extract_bit_expr(bie->op0, ebe->op1, srcp_default);
                                    const auto eb_ga0 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op0, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                                    B->PushBefore(eb_ga0, stmt, AppM);
                                    const auto eb_op1 =
                                        IRman->create_extract_bit_expr(bie->op1, ebe->op1, srcp_default);
                                    const auto eb_ga1 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt, AppM);
                                    const auto anding = IRman->create_binary_operation(
                                        ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga0))->op0,
                                        GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0, srcp_default,
                                        truth_or_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, anding);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else if(prev_code1 == bit_xor_expr_K)
                                 {
                                    const auto bxe = GetPointerS<const bit_xor_expr>(GET_CONST_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto eb_op0 =
                                        IRman->create_extract_bit_expr(bxe->op0, ebe->op1, srcp_default);
                                    const auto eb_ga0 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op0, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                                    B->PushBefore(eb_ga0, stmt, AppM);
                                    const auto eb_op1 =
                                        IRman->create_extract_bit_expr(bxe->op1, ebe->op1, srcp_default);
                                    const auto eb_ga1 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt, AppM);
                                    const auto anding = IRman->create_binary_operation(
                                        ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga0))->op0,
                                        GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0, srcp_default,
                                        truth_xor_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, anding);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else if(prev_code1 == cond_expr_K)
                                 {
                                    const auto ce = GetPointerS<const cond_expr>(GET_CONST_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto eb_op1 = IRman->create_extract_bit_expr(ce->op1, ebe->op1, srcp_default);
                                    const auto eb_ga1 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt, AppM);
                                    const auto eb_op2 = IRman->create_extract_bit_expr(ce->op2, ebe->op1, srcp_default);
                                    const auto eb_ga2 = IRman->CreateGimpleAssign(
                                        ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                        TM->CreateUniqueIntegerCst(1, ebe->type), eb_op2, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga2));
                                    B->PushBefore(eb_ga2, stmt, AppM);
                                    const auto ceRes = IRman->create_ternary_operation(
                                        ebe->type, ce->op0,
                                        GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0,
                                        GetPointer<const gimple_assign>(GET_CONST_NODE(eb_ga2))->op0, srcp_default,
                                        cond_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, ceRes);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                                 else if(prev_code1 == plus_expr_K)
                                 {
                                    const auto pe = GetPointerS<const plus_expr>(GET_CONST_NODE(prev_ga->op1));
                                    if(GET_CONST_NODE(pe->op1)->get_kind() == integer_cst_K &&
                                       tree_helper::TypeSize(ebe->op0) <= max_lut_size)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_expr usage before: " + stmt->ToString());
                                       auto carry = TM->CreateUniqueIntegerCst(0, ebe->type);
                                       tree_nodeRef sum;
                                       for(integer_cst_t bitIndex = 0; bitIndex <= pos_value; ++bitIndex)
                                       {
                                          const auto bitIndex_node =
                                              TM->CreateUniqueIntegerCst(bitIndex, tree_helper::CGetType(ebe->op1));
                                          const auto eb_op1 =
                                              IRman->create_extract_bit_expr(pe->op0, bitIndex_node, srcp_default);
                                          const auto eb_ga1 = IRman->CreateGimpleAssign(
                                              ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                              TM->CreateUniqueIntegerCst(1, ebe->type), eb_op1, function_id,
                                              srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---Created " + STR(eb_ga1));
                                          B->PushBefore(eb_ga1, stmt, AppM);
                                          const auto eb_op2 =
                                              IRman->create_extract_bit_expr(pe->op1, bitIndex_node, srcp_default);
                                          const auto eb_ga2 = IRman->CreateGimpleAssign(
                                              ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                              TM->CreateUniqueIntegerCst(1, ebe->type), eb_op2, function_id,
                                              srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---Created " + STR(eb_ga2));
                                          B->PushBefore(eb_ga2, stmt, AppM);
                                          const auto sum0 = IRman->create_binary_operation(
                                              ebe->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0,
                                              GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga2))->op0,
                                              srcp_default, truth_xor_expr_K);
                                          const auto sum0_ga1 = IRman->CreateGimpleAssign(
                                              ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                              TM->CreateUniqueIntegerCst(1, ebe->type), sum0, function_id,
                                              srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---Created " + STR(sum0_ga1));
                                          B->PushBefore(sum0_ga1, stmt, AppM);
                                          sum = IRman->create_binary_operation(
                                              ebe->type,
                                              GetPointerS<const gimple_assign>(GET_CONST_NODE(sum0_ga1))->op0, carry,
                                              srcp_default, truth_xor_expr_K);
                                          if(bitIndex < pos_value)
                                          {
                                             const auto sum_ga1 = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), sum, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(sum_ga1));
                                             B->PushBefore(sum_ga1, stmt, AppM);

                                             const auto and1 = IRman->create_binary_operation(
                                                 ebe->type,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0, carry,
                                                 srcp_default, truth_and_expr_K);
                                             const auto and1_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), and1, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(and1_ga));
                                             B->PushBefore(and1_ga, stmt, AppM);

                                             const auto and2 = IRman->create_binary_operation(
                                                 ebe->type,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga2))->op0, carry,
                                                 srcp_default, truth_and_expr_K);
                                             const auto and2_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), and2, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(and2_ga));
                                             B->PushBefore(and2_ga, stmt, AppM);

                                             const auto and3 = IRman->create_binary_operation(
                                                 ebe->type,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga1))->op0,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga2))->op0,
                                                 srcp_default, truth_and_expr_K);
                                             const auto and3_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), and3, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(and3_ga));
                                             B->PushBefore(and3_ga, stmt, AppM);

                                             const auto or1 = IRman->create_binary_operation(
                                                 ebe->type,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(and1_ga))->op0,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(and2_ga))->op0,
                                                 srcp_default, truth_or_expr_K);
                                             const auto or1_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), or1, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(or1_ga));
                                             B->PushBefore(or1_ga, stmt, AppM);

                                             const auto carry1 = IRman->create_binary_operation(
                                                 ebe->type,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(or1_ga))->op0,
                                                 GetPointerS<const gimple_assign>(GET_CONST_NODE(and3_ga))->op0,
                                                 srcp_default, truth_or_expr_K);
                                             const auto carry1_ga = IRman->CreateGimpleAssign(
                                                 ebe->type, TM->CreateUniqueIntegerCst(0, ebe->type),
                                                 TM->CreateUniqueIntegerCst(1, ebe->type), carry1, function_id,
                                                 srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                            "---Created " + STR(carry1_ga));
                                             B->PushBefore(carry1_ga, stmt, AppM);
                                             carry = GetPointerS<const gimple_assign>(GET_CONST_NODE(carry1_ga))->op0;
                                          }
                                       }
                                       TM->ReplaceTreeNode(stmt, ga->op1, sum);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---replace extract_bit_expr usage after: " + stmt->ToString());
                                       modified = true;
                                       AppM->RegisterTransformation(GetName(), stmt);
                                    }
                                 }
                              }
                           }
                        }
                        else if(GET_CONST_NODE(ebe->op0)->get_kind() == integer_cst_K)
                        {
                           const auto res_value =
                               ((tree_helper::GetConstValue(ebe->op0, tree_helper::IsSignedIntegerType(ebe->op0)) >>
                                 pos_value) &
                                1) != 0;
                           const auto res_node = TM->CreateUniqueIntegerCst(res_value, ebe->type);
                           propagateValue(ssa, TM, ga->op0, res_node, DEBUG_CALLSITE);
                        }
                     };
                     extract_bit_expr_BVO();
                  }
                  else if(GET_CONST_NODE(ga->op1)->get_kind() == nop_expr_K)
                  {
                     auto nop_expr_BVO = [&] {
                        const auto ne = GetPointerS<const nop_expr>(GET_CONST_NODE(ga->op1));
                        if(tree_helper::IsBooleanType(ga->op0))
                        {
                           if(tree_helper::IsBooleanType(ne->op))
                           {
                              propagateValue(ssa, TM, ga->op0, ne->op, DEBUG_CALLSITE);
                           }
                           else
                           {
                              const auto ne_op_ssa = GetPointer<const ssa_name>(GET_CONST_NODE(ne->op));
                              if(ne_op_ssa)
                              {
                                 if(GET_CONST_NODE(ne_op_ssa->type)->get_kind() == integer_type_K)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto indexType = IRman->GetUnsignedLongLongType();
                                    const auto zero_node = TM->CreateUniqueIntegerCst(0, indexType);
                                    const auto srcp_default =
                                        ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                                    const auto eb_op = IRman->create_extract_bit_expr(ne->op, zero_node, srcp_default);
                                    const auto eb_ga = IRman->CreateGimpleAssign(
                                        ne->type, TM->CreateUniqueIntegerCst(0, ne->type),
                                        TM->CreateUniqueIntegerCst(1, ne->type), eb_op, function_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt, AppM);
                                    const auto bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, ne->type);
                                    const auto masking = IRman->create_binary_operation(
                                        ne->type, GetPointerS<const gimple_assign>(GET_CONST_NODE(eb_ga))->op0,
                                        bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                    TM->ReplaceTreeNode(
                                        stmt, ga->op1,
                                        masking); /// replaced with redundant code to restart lut_transformation
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                   "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
                                    AppM->RegisterTransformation(GetName(), stmt);
                                 }
                              }
                           }
                        }
                        else if(tree_helper::IsSameType(ga->op0, ne->op))
                        {
                           auto bw_op1 = tree_helper::Size(ne->op);
                           auto bw_op0 = tree_helper::Size(ga->op0);

                           if(bw_op1 <= bw_op0)
                           {
                              propagateValue(ssa, TM, ga->op0, ne->op, DEBUG_CALLSITE);
                           }
                        }
                     };
                     nop_expr_BVO();
                  }
               };
               ga_BVO();
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement analyzed " + stmt->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB analyzed " + STR(B->number));
      for(const auto& phi : B->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Phi operation " + phi->ToString());
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Phi index: " + STR(GET_INDEX_CONST_NODE(phi)));
         auto pn = GetPointerS<gimple_phi>(GET_CONST_NODE(phi));
         bool is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "LHS: " + STR(GET_INDEX_CONST_NODE(pn->res)));
            const auto ssa = GetPointer<const ssa_name>(GET_CONST_NODE(pn->res));
            if(ssa && !ssa->bit_values.empty())
            {
               const auto& bit_values = ssa->bit_values;
               const auto is_constant = is_bit_values_constant(bit_values);
               if(is_constant)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Left part is constant " + bit_values);
                  const auto const_value =
                      convert_bitvalue_to_integer_cst(bit_values, TM, GET_INDEX_CONST_NODE(pn->res));
                  const auto val = TM->CreateUniqueIntegerCst(const_value, tree_helper::CGetType(pn->res));

                  propagateValue(ssa, TM, pn->res, val, DEBUG_CALLSITE);
                  if(AppM->ApplyNewTransformation())
                  {
                     pn->res = TM->GetTreeNode(ssa->index);
                     THROW_ASSERT(ssa->CGetUseStmts().empty(), "unexpected case");
                     AppM->RegisterTransformation(GetName(), phi);
                  }
               }
            }
         }
      }
   }
}
