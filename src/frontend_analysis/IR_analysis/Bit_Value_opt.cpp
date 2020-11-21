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

#include "Range.hpp"
#include "bit_lattice.hpp"

/// Autoheader include
#include "config_HAVE_FROM_DISCREPANCY_BUILT.hpp"
#include "config_HAVE_STDCXX_17.hpp"

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

// STD include
#include <boost/range/adaptor/reversed.hpp>
#include <cmath>
#include <fstream>
#include <string>

// Tree include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "ext_tree_node.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "math_function.hpp"       // for resize_to_1_8_16_32_64_128_256_512
#include "string_manipulation.hpp" // for GET_CLASS
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_manipulation.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include "utility.hpp"

Bit_Value_opt::Bit_Value_opt(const ParameterConstRef _parameters, const application_managerRef _AppM, unsigned int _function_id, const DesignFlowManagerConstRef _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, BIT_VALUE_OPT, _design_flow_manager, _parameters), modified(false), restart_dead_code(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

Bit_Value_opt::~Bit_Value_opt() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> Bit_Value_opt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
#if HAVE_ILP_BUILT && HAVE_BAMBU_BUILT
         relationships.insert(std::make_pair(SDC_CODE_MOTION, SAME_FUNCTION));
#endif
         /// Following precedence is to reduce invalidation; BIT_VALUE_OPT of called can invalidate DEAD_CODE_ELIMINATION of called and so this
         relationships.insert(std::make_pair(BIT_VALUE_OPT, CALLED_FUNCTIONS));
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         if(parameters->isOption(OPT_bitvalue_ipa) and parameters->getOption<bool>(OPT_bitvalue_ipa))
         {
            relationships.insert(std::make_pair(RANGE_ANALYSIS, WHOLE_APPLICATION));
            relationships.insert(std::make_pair(BIT_VALUE_IPA, WHOLE_APPLICATION));
         }
         else
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(COMPLETE_CALL_GRAPH, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         switch(GetStatus())
         {
            case DesignFlowStep_Status::SUCCESS:
            {
               if(restart_dead_code)
                  relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
               break;
            }
            case DesignFlowStep_Status::SKIPPED:
            case DesignFlowStep_Status::UNCHANGED:
            case DesignFlowStep_Status::UNEXECUTED:
            case DesignFlowStep_Status::UNNECESSARY:
            {
               break;
            }
            case DesignFlowStep_Status::ABORTED:
            case DesignFlowStep_Status::EMPTY:
            case DesignFlowStep_Status::NONEXISTENT:
            default:
               THROW_UNREACHABLE("");
         }
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

static void constrainSSA(ssa_name* op_ssa, tree_managerRef TM)
{
   if(tree_helper::is_real(TM, GET_INDEX_NODE(op_ssa->type)))
   {
      return;
   }
   const auto nbit = op_ssa->bit_values.size();
   const auto op0_type_id = GET_INDEX_NODE(op_ssa->type);
   const auto nbitType = BitLatticeManipulator::size(TM, op0_type_id);
   if(nbit != nbitType)
   {
      const bool isSigned = tree_helper::is_int(TM, op0_type_id);
      if(isSigned)
      {
         RangeRef constraintRange(new Range(Regular, static_cast<Range::bw_t>(nbitType), -(1ll << (nbit - 1)), (1ll << (nbit - 1)) - 1));
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
         op_ssa->min = TM->CreateUniqueIntegerCst(-(1ll << (nbit - 1)), op0_type_id);
         op_ssa->max = TM->CreateUniqueIntegerCst((1ll << (nbit - 1)) - 1, op0_type_id);
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
         op_ssa->min = TM->CreateUniqueIntegerCst(0, op0_type_id);
         op_ssa->max = TM->CreateUniqueIntegerCst((1ll << nbit) - 1, op0_type_id);
      }
      // std::cerr<<"var " << op_ssa->ToString()<<" ";
      // std::cerr << "min " <<op_ssa->min->ToString() << " max " <<op_ssa->max->ToString()<<"\n";
      // std::cerr << "nbit "<< nbit << " nbitType " << nbitType <<"\n";
   }
}

void Bit_Value_opt::optimize(statement_list* sl, tree_managerRef TM, tree_manipulationRef IRman)
{
   for(auto bb_pair : sl->list_of_bloc)
   {
      blocRef B = bb_pair.second;
      unsigned int B_id = B->number;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(B_id));
      const auto list_of_stmt = B->CGetStmtList();
      for(const auto& stmt : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + GET_NODE(stmt)->ToString());
#ifndef NDEBUG
         if(not AppM->ApplyNewTransformation())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because reached limit of CFG transformations");
            continue;
         }
#endif
         if(GetPointer<gimple_node>(GET_NODE(stmt))->keep)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because the statement has been annotated with the keep tag");
            continue;
         }
         if(GET_NODE(stmt)->get_kind() == gimple_assign_K)
         {
            auto* ga = GetPointer<gimple_assign>(GET_NODE(stmt));
            unsigned int output_uid = GET_INDEX_NODE(ga->op0);
            auto* ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));
            if(ssa)
            {
               if(tree_helper::is_real(TM, output_uid))
               {
                  auto real_BVO = [&] {
                     if(GET_NODE(ga->op1)->get_kind() == cond_expr_K)
                     {
                        auto* me = GetPointer<cond_expr>(GET_NODE(ga->op1));
                        tree_nodeRef op0 = GET_NODE(me->op1);
                        tree_nodeRef op1 = GET_NODE(me->op2);
                        tree_nodeRef condition = GET_NODE(me->op0);
                        if(op0 == op1)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with equal operands");
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, me->op1);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else if(condition->get_kind() == integer_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with constant condition");
                           auto* ic = GetPointer<integer_cst>(condition);
                           auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
                              if(ull_value)
                                 TM->ReplaceTreeNode(use.first, ga->op0, me->op1);
                              else
                                 TM->ReplaceTreeNode(use.first, ga->op0, me->op2);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---nothing more can be done");
                     }
                     else if(GetPointer<cst_node>(GET_NODE(ga->op1)))
                     {
                        const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                        for(const auto& use : StmtUses)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                           TM->ReplaceTreeNode(use.first, ga->op0, ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                           modified = true;
                        }
                        if(ssa->CGetUseStmts().empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                           restart_dead_code = true;
                        }
#ifndef NDEBUG
                        AppM->RegisterTransformation(GetName(), stmt);
#endif
                     }
                     else if(GetPointer<ssa_name>(GET_NODE(ga->op1)))
                     {
                        const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                        for(const auto& use : StmtUses)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace ssa usage before: " + use.first->ToString());
                           TM->ReplaceTreeNode(use.first, ga->op0, ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace ssa usage after: " + use.first->ToString());
                           modified = true;
                        }
                        if(ssa->CGetUseStmts().empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                           restart_dead_code = true;
                        }
#ifndef NDEBUG
                        AppM->RegisterTransformation(GetName(), stmt);
#endif
                     }
                     else if(GET_NODE(ga->op1)->get_kind() == view_convert_expr_K)
                     {
                        auto vce = GetPointer<view_convert_expr>(GET_NODE(ga->op1));
                        if(GetPointer<cst_node>(GET_NODE(vce->op)))
                        {
                           if(GET_NODE(vce->op)->get_kind() == integer_cst_K)
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(vce->op));
                              auto bitwidth_op = BitLatticeManipulator::Size(vce->type);
                              tree_nodeRef val;
                              if(bitwidth_op == 32)
                              {
                                 union
                                 {
                                    float dest;
                                    int source;
                                 } __conv_union;
                                 __conv_union.source = static_cast<int>(int_const->value);
                                 const auto data_value_id = TM->new_tree_node_id();
                                 const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
                                 val = tree_man->CreateRealCst(vce->type, static_cast<long double>(__conv_union.dest), data_value_id);
                              }
                              else if(bitwidth_op == 64)
                              {
                                 union
                                 {
                                    double dest;
                                    long long int source;
                                 } __conv_union;
                                 __conv_union.source = int_const->value;
                                 const auto data_value_id = TM->new_tree_node_id();
                                 const tree_manipulationRef tree_man = tree_manipulationRef(new tree_manipulation(TM, parameters));
                                 val = tree_man->CreateRealCst(vce->type, static_cast<long double>(__conv_union.dest), data_value_id);
                              }
                              else
                                 THROW_ERROR("not supported floating point bitwidth");

                              const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                 TM->ReplaceTreeNode(use.first, ga->op0, val);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                 modified = true;
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                        }
                     }
                     else
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---real variables not considered: " + STR(GET_INDEX_NODE(ga->op0)));
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  };
                  real_BVO();
                  continue;
               }
               if(tree_helper::is_a_complex(TM, output_uid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---complex variables not considered: " + STR(GET_INDEX_NODE(ga->op0)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  continue;
               }
               if(tree_helper::is_a_vector(TM, output_uid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---vector variables not considered: " + STR(GET_INDEX_NODE(ga->op0)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  continue;
               }
               if((GetPointer<integer_cst>(GET_NODE(ga->op1)) || GetPointer<real_cst>(GET_NODE(ga->op1))) && tree_helper::is_a_pointer(TM, GET_INDEX_NODE(ga->op1)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---constant pointer value assignments not considered: " + STR(GET_INDEX_NODE(ga->op0)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  continue;
               }
               if(GetPointer<call_expr>(GET_NODE(ga->op1)) and ga->vdef)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---calls with side effects cannot be optimized" + STR(GET_INDEX_NODE(ga->op1)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  continue;
               }
               if(GetPointer<addr_expr>(GET_NODE(ga->op1)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---addr_expr cannot be optimized" + STR(GET_INDEX_NODE(ga->op1)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement " + GET_NODE(stmt)->ToString());
                  continue;
               }
               auto ga_BVO = [&] {
                  unsigned int type_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(ga->op0));
                  tree_nodeRef ga_op_type = TM->GetTreeReindex(type_index);
                  tree_nodeRef Scpe = TM->GetTreeReindex(function_id);
                  const std::string& bit_values = ssa->bit_values;
                  bool is_constant = bit_values.size() != 0 && !tree_helper::is_a_pointer(TM, GET_INDEX_NODE(ga->op1));
                  for(auto current_el : bit_values)
                  {
                     if(current_el == 'U')
                     {
                        is_constant = false;
                        break;
                     }
                  }

                  auto rel_expr_BVO = [&] {
                     auto* me = GetPointer<binary_expr>(GET_NODE(ga->op1));
                     tree_nodeRef op0 = GET_NODE(me->op0);
                     tree_nodeRef op1 = GET_NODE(me->op1);

                     std::string s0, s1;
                     if(GetPointer<ssa_name>(op0))
                        s0 = GetPointer<ssa_name>(op0)->bit_values;
                     if(GetPointer<ssa_name>(op1))
                        s1 = GetPointer<ssa_name>(op1)->bit_values;
                     unsigned int precision;
                     if(s0.size() && s1.size())
                        precision = static_cast<unsigned int>(std::min(s0.size(), s1.size()));
                     else
                        precision = static_cast<unsigned int>(std::max(s0.size(), s1.size()));

                     if(precision)
                     {
                        unsigned int trailing_zero = 0;
                        if(GetPointer<integer_cst>(op0))
                        {
                           auto* ic = GetPointer<integer_cst>(op0);
                           auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                           s0 = convert_to_binary(ull_value, precision);
                        }
                        if(GetPointer<integer_cst>(op1))
                        {
                           auto* ic = GetPointer<integer_cst>(op1);
                           auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                           s1 = convert_to_binary(ull_value, precision);
                        }
                        precision = static_cast<unsigned int>(std::min(s0.size(), s1.size()));
                        if(precision == 0)
                           precision = 1;
                        for(auto s0it = s0.rbegin(), s1it = s1.rbegin(), s0end = s0.rend(), s1end = s1.rend(); s0it != s0end && s1it != s1end; ++s0it, ++s1it)
                        {
                           if((*s0it == *s1it && (*s1it == '0' || *s1it == '1')) || *s0it == 'X' || *s1it == 'X')
                              ++trailing_zero;
                           else
                              break;
                        }
                        if(trailing_zero)
                        {
                           INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Bit Value Opt: " + std::string(GET_NODE(ga->op1)->get_kind_text()) + " optimized, nbits = " + STR(trailing_zero));
                           INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                           modified = true;
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                           const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           unsigned int type_index0 = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op0));
                           tree_nodeRef op0_op_type = TM->GetTreeReindex(type_index0);
                           unsigned int type_index1 = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op1));
                           tree_nodeRef op1_op_type = TM->GetTreeReindex(type_index1);

                           if(GetPointer<ssa_name>(op0))
                           {
                              tree_nodeRef op0_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), type_index0);
                              tree_nodeRef op0_expr = IRman->create_binary_operation(op0_op_type, me->op0, op0_const_node, srcp_default, rshift_expr_K);
                              tree_nodeRef op0_ga = IRman->CreateGimpleAssign(op0_op_type, tree_nodeRef(), tree_nodeRef(), op0_expr, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                              B->PushBefore(op0_ga, stmt);
                              tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;
                              TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
                              /// set the bit_values to the ssa var
                              auto* op0_ssa = GetPointer<ssa_name>(GET_NODE(op0_ga_var));
                              op0_ssa->bit_values = GetPointer<ssa_name>(op0)->bit_values.substr(0, GetPointer<ssa_name>(op0)->bit_values.size() - trailing_zero);
                              constrainSSA(op0_ssa, TM);
                           }
                           else
                           {
                              auto* int_const = GetPointer<integer_cst>(op0);
                              if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op0)))
                                 TM->ReplaceTreeNode(stmt, me->op0, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> trailing_zero), type_index0));
                              else
                                 TM->ReplaceTreeNode(stmt, me->op0, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> trailing_zero), type_index0));
                           }
                           if(GetPointer<ssa_name>(op1))
                           {
                              tree_nodeRef op1_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), type_index1);
                              tree_nodeRef op1_expr = IRman->create_binary_operation(op1_op_type, me->op1, op1_const_node, srcp_default, rshift_expr_K);
                              tree_nodeRef op1_ga = IRman->CreateGimpleAssign(op1_op_type, tree_nodeRef(), tree_nodeRef(), op1_expr, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                              B->PushBefore(op1_ga, stmt);
                              tree_nodeRef op1_ga_var = GetPointer<gimple_assign>(GET_NODE(op1_ga))->op0;
                              TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
                              /// set the bit_values to the ssa var
                              auto* op1_ssa = GetPointer<ssa_name>(GET_NODE(op1_ga_var));
                              op1_ssa->bit_values = GetPointer<ssa_name>(op1)->bit_values.substr(0, GetPointer<ssa_name>(op1)->bit_values.size() - trailing_zero);
                              constrainSSA(op1_ssa, TM);
                           }
                           else
                           {
                              auto* int_const = GetPointer<integer_cst>(op1);
                              if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op1)))
                                 TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> trailing_zero), type_index1));
                              else
                                 TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> trailing_zero), type_index1));
                           }
                        }
                     }
                  };

                  if(is_constant)
                  {
                     auto c_BVO = [&] {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Left part is constant " + bit_values);
                        unsigned long long int const_value = 0;
                        unsigned int index_val = 0;
                        for(auto current_el : boost::adaptors::reverse(bit_values))
                        {
                           if(current_el == '1')
                              const_value |= 1ULL << index_val;
                           ++index_val;
                        }
                        /// in case do sign extension
                        if(tree_helper::is_int(TM, output_uid) && bit_values[0] == '1')
                        {
                           for(; index_val < 64; ++index_val)
                              const_value |= 1ULL << index_val;
                        }
                        tree_nodeRef val;
                        if(GetPointer<integer_cst>(GET_NODE(ga->op1)))
                           val = ga->op1;
                        else
                           val = TM->CreateUniqueIntegerCst(static_cast<long long int>(const_value), type_index);
                        const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                        for(const auto& use : StmtUses)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                           TM->ReplaceTreeNode(use.first, ga->op0, val);
                           modified = true;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                        }
                        if(GET_NODE(ga->op0)->get_kind() == ssa_name_K and ga->predicate)
                        {
                           auto zeroval = TM->CreateUniqueIntegerCst(static_cast<long long int>(0), type_index);
                           TM->ReplaceTreeNode(stmt, ga->predicate, zeroval);
                        }
                        if(ssa->CGetUseStmts().empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                           restart_dead_code = true;
                        }
#ifndef NDEBUG
                        AppM->RegisterTransformation(GetName(), stmt);
#endif
                     };
                     c_BVO();
                  }
                  else if(GetPointer<cst_node>(GET_NODE(ga->op1)))
                  {
                     auto cst_node_BVO = [&] {
                        const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                        for(const auto& use : StmtUses)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                           TM->ReplaceTreeNode(use.first, ga->op0, ga->op1);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                           modified = true;
                        }
                        if(ssa->CGetUseStmts().empty())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                           restart_dead_code = true;
                        }
#ifndef NDEBUG
                        AppM->RegisterTransformation(GetName(), stmt);
#endif
                     };
                     cst_node_BVO();
                  }
                  else if(GetPointer<ssa_name>(GET_NODE(ga->op1)))
                  {
                     auto ssa_name_BVO = [&] {
                        if(!ssa->bit_values.empty() && ssa->bit_values.at(0) == '0' && ssa->bit_values.size() <= 64)
                        {
                           const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst((1LL << (ssa->bit_values.size() - 1)) - 1, ssa->type->index);
                           tree_nodeRef band_expr = IRman->create_binary_operation(ssa->type, ga->op1, bit_mask_constant_node, srcp_default, bit_and_expr_K);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace ssa usage before: " + stmt->ToString());
                           TM->ReplaceTreeNode(stmt, ga->op1, band_expr);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace ssa usage after: " + stmt->ToString());
                           modified = true;
                           ga->keep = true; /// this prevent an infinite loop with CSE
                        }
                        else
                        {
                           auto bw_op1 = BitLatticeManipulator::Size(GET_NODE(ga->op1));
                           auto bw_op0 = BitLatticeManipulator::Size(GET_NODE(ga->op0));
                           auto max_bw = 0u;
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              if(GET_NODE(use.first)->get_kind() == gimple_assign_K && GET_NODE(GetPointer<gimple_assign>(GET_NODE(use.first))->op1)->get_kind() == ssa_name_K)
                                 max_bw = std::max(max_bw, BitLatticeManipulator::Size(GET_NODE(GetPointer<gimple_assign>(GET_NODE(use.first))->op1)));
                              else
                                 max_bw = bw_op1;
                           }
                           if(max_bw < bw_op1)
                           {
                              auto ssa1 = GetPointer<ssa_name>(GET_NODE(ga->op1));
                              ssa1->min = ssa->min;
                              ssa1->max = ssa->max;
                              bw_op1 = BitLatticeManipulator::Size(GET_NODE(ga->op1));
                           }

                           if(bw_op1 <= bw_op0)
                           {
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace ssa usage before: " + use.first->ToString());
                                 TM->ReplaceTreeNode(use.first, ga->op0, ga->op1);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace ssa usage after: " + use.first->ToString());
                                 modified = true;
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                        }
                     };
                     ssa_name_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == mult_expr_K || GET_NODE(ga->op1)->get_kind() == widen_mult_expr_K)
                  {
                     auto mult_expr_BVO = [&] {
                        auto* me = GetPointer<binary_expr>(GET_NODE(ga->op1));
                        tree_nodeRef op0 = GET_NODE(me->op0);
                        tree_nodeRef op1 = GET_NODE(me->op1);
                        /// first check if we have to change a mult_expr in a widen_mult_expr
                        unsigned int data_bitsize_out = resize_to_1_8_16_32_64_128_256_512(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                        unsigned int data_bitsize_in0 = resize_to_1_8_16_32_64_128_256_512(BitLatticeManipulator::Size(op0));
                        unsigned int data_bitsize_in1 = resize_to_1_8_16_32_64_128_256_512(BitLatticeManipulator::Size(op1));
                        bool realp = tree_helper::is_real(TM, GET_INDEX_NODE(GetPointer<binary_expr>(GET_NODE(ga->op1))->type));
                        if(GET_NODE(ga->op1)->get_kind() == mult_expr_K && !realp && std::max(data_bitsize_in0, data_bitsize_in1) * 2 == data_bitsize_out)
                        {
                           tree_nodeRef op0_type = TM->GetTreeReindex(type_index);
                           const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           tree_nodeRef new_widen_expr = IRman->create_binary_operation(op0_type, GetPointer<binary_expr>(GET_NODE(ga->op1))->op0, GetPointer<binary_expr>(GET_NODE(ga->op1))->op1, srcp_default, widen_mult_expr_K);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(ga->op1) + " with " + STR(new_widen_expr) + " in " + STR(stmt));
                           modified = true;
                           TM->ReplaceTreeNode(stmt, ga->op1, new_widen_expr);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace expression with a widen mult_expr: " + stmt->ToString());
                        }
                        else if(GET_NODE(ga->op1)->get_kind() == widen_mult_expr_K && !realp && std::max(data_bitsize_in0, data_bitsize_in1) == data_bitsize_out)
                        {
                           tree_nodeRef op0_type = TM->GetTreeReindex(type_index);
                           const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           tree_nodeRef new_expr = IRman->create_binary_operation(op0_type, GetPointer<binary_expr>(GET_NODE(ga->op1))->op0, GetPointer<binary_expr>(GET_NODE(ga->op1))->op1, srcp_default, mult_expr_K);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(ga->op1) + " with " + STR(new_expr) + " in " + STR(stmt));
                           modified = true;
                           TM->ReplaceTreeNode(stmt, ga->op1, new_expr);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace expression with a mult_expr: " + stmt->ToString());
                        }
                        bool isSigned = tree_helper::is_int(TM, type_index);
                        if(!isSigned && GET_NODE(ga->op1)->get_kind() == mult_expr_K && (data_bitsize_in0 == 1 || data_bitsize_in1 == 1))
                        {
                           modified = true;
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                           tree_nodeRef constNE0 = TM->CreateUniqueIntegerCst(0, type_index);
                           const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           tree_nodeRef bt = IRman->create_boolean_type();
                           tree_nodeRef cond_op0 = IRman->create_binary_operation(bt, data_bitsize_in0 == 1 ? me->op0 : me->op1, constNE0, srcp_default, ne_expr_K);
                           tree_nodeRef op0_ga = IRman->CreateGimpleAssign(bt, TM->CreateUniqueIntegerCst(0, bt->index), TM->CreateUniqueIntegerCst(1, bt->index), cond_op0, B_id, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                           B->PushBefore(op0_ga, stmt);
                           tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;
                           tree_nodeRef const0 = TM->CreateUniqueIntegerCst(0, type_index);
                           tree_nodeRef cond_op = IRman->create_ternary_operation(ga_op_type, op0_ga_var, data_bitsize_in1 == 1 ? me->op0 : me->op1, const0, srcp_default, cond_expr_K);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + STR(ga->op1) + " with " + STR(cond_op) + " in " + STR(stmt));
                           TM->ReplaceTreeNode(stmt, ga->op1, cond_op);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace expression with a cond_expr: " + stmt->ToString());
                        }
                        else
                        {
                           unsigned int trailing_zero_op0 = 0;
                           unsigned int trailing_zero_op1 = 0;
                           if(GetPointer<ssa_name>(op0))
                           {
                              const std::string& bit_values_op0 = GetPointer<ssa_name>(op0)->bit_values;
                              for(auto current_el : boost::adaptors::reverse(bit_values_op0))
                              {
                                 if(current_el == '0' || current_el == 'X')
                                    ++trailing_zero_op0;
                                 else
                                    break;
                              }
                           }
                           else if(GetPointer<integer_cst>(op0))
                           {
                              auto* int_const = GetPointer<integer_cst>(op0);
                              auto value_int = static_cast<unsigned long long int>(int_const->value);
                              for(unsigned int index = 0; index < 64 && value_int != 0; ++index)
                              {
                                 if(value_int & (1ULL << index))
                                    break;
                                 else
                                    ++trailing_zero_op0;
                              }
                           }
                           if(GetPointer<ssa_name>(op1))
                           {
                              const std::string& bit_values_op1 = GetPointer<ssa_name>(op1)->bit_values;
                              for(auto current_el : boost::adaptors::reverse(bit_values_op1))
                              {
                                 if(current_el == '0' || current_el == 'X')
                                    ++trailing_zero_op1;
                                 else
                                    break;
                              }
                           }
                           else if(GetPointer<integer_cst>(op1))
                           {
                              auto* int_const = GetPointer<integer_cst>(op1);
                              auto value_int = static_cast<unsigned long long int>(int_const->value);
                              for(unsigned int index = 0; index < 64 && value_int != 0; ++index)
                              {
                                 if(value_int & (1ULL << index))
                                    break;
                                 else
                                    ++trailing_zero_op1;
                              }
                           }
                           if(trailing_zero_op0 != 0 || trailing_zero_op1 != 0)
                           {
                              modified = true;
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                              INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Bit Value Opt: mult_expr/widen_mult_expr optimized, nbits = " + STR(trailing_zero_op0 + trailing_zero_op1));
                              INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                              const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              if(trailing_zero_op0 != 0)
                              {
                                 const unsigned int op0_type_id = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op0));
                                 tree_nodeRef op0_type = TM->CGetTreeReindex(op0_type_id);
                                 tree_nodeRef op0_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero_op0), op0_type_id);
                                 tree_nodeRef op0_expr = IRman->create_binary_operation(op0_type, me->op0, op0_const_node, srcp_default, rshift_expr_K);
                                 tree_nodeRef op0_ga = IRman->CreateGimpleAssign(op0_type, tree_nodeRef(), tree_nodeRef(), op0_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt);
                                 tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
                                 /// set the bit_values to the ssa var
                                 if(GetPointer<ssa_name>(op0))
                                 {
                                    auto* op0_ssa = GetPointer<ssa_name>(GET_NODE(op0_ga_var));
                                    op0_ssa->bit_values = GetPointer<ssa_name>(op0)->bit_values.substr(0, GetPointer<ssa_name>(op0)->bit_values.size() - trailing_zero_op0);
                                    constrainSSA(op0_ssa, TM);
                                 }
                              }
                              if(trailing_zero_op1 != 0 and op0->index != op1->index)
                              {
                                 const unsigned int op1_type_id = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op1));
                                 tree_nodeRef op1_type = TM->CGetTreeReindex(op1_type_id);
                                 tree_nodeRef op1_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero_op1), op1_type_id);
                                 tree_nodeRef op1_expr = IRman->create_binary_operation(op1_type, me->op1, op1_const_node, srcp_default, rshift_expr_K);
                                 tree_nodeRef op1_ga = IRman->CreateGimpleAssign(op1_type, tree_nodeRef(), tree_nodeRef(), op1_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt);
                                 tree_nodeRef op1_ga_var = GetPointer<gimple_assign>(GET_NODE(op1_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
                                 /// set the bit_values to the ssa var
                                 if(GetPointer<ssa_name>(op1))
                                 {
                                    auto* op1_ssa = GetPointer<ssa_name>(GET_NODE(op1_ga_var));
                                    op1_ssa->bit_values = GetPointer<ssa_name>(op1)->bit_values.substr(0, GetPointer<ssa_name>(op1)->bit_values.size() - trailing_zero_op1);
                                    constrainSSA(op1_ssa, TM);
                                 }
                              }

                              tree_nodeRef ssa_vd = IRman->create_ssa_name(tree_nodeRef(), ga_op_type, tree_nodeRef(), tree_nodeRef());
                              auto* sn = GetPointer<ssa_name>(GET_NODE(ssa_vd));
                              /// set the bit_values to the ssa var
                              sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_zero_op0 - trailing_zero_op1);
                              constrainSSA(sn, TM);
                              tree_nodeRef op_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero_op0 + trailing_zero_op1), type_index);
                              tree_nodeRef op_expr = IRman->create_binary_operation(ga_op_type, ssa_vd, op_const_node, srcp_default, lshift_expr_K);
                              tree_nodeRef curr_ga = IRman->CreateGimpleAssign(ga_op_type, ssa->min, ssa->max, op_expr, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                              TM->ReplaceTreeNode(curr_ga, GetPointer<gimple_assign>(GET_NODE(curr_ga))->op0, ga->op0);
                              TM->ReplaceTreeNode(stmt, ga->op0, ssa_vd);
                              B->PushAfter(curr_ga, stmt);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "pushed");
                           }
                        }
                     };
                     mult_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == plus_expr_K || GET_NODE(ga->op1)->get_kind() == minus_expr_K)
                  {
                     auto plus_minus_BVO = [&] {
                        auto* me = GetPointer<binary_expr>(GET_NODE(ga->op1));
                        if(me->op0->index == me->op1->index)
                        {
                           if(GET_NODE(ga->op1)->get_kind() == minus_expr_K)
                           {
                              TM->ReplaceTreeNode(stmt, ga->op1, TM->CreateUniqueIntegerCst(0, type_index));
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Statement transformed in " + GET_NODE(stmt)->ToString());
                              modified = true;
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                           else
                           {
                              tree_nodeRef op_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(1), type_index);
                              const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              tree_nodeRef op_expr = IRman->create_binary_operation(ga_op_type, me->op0, op_const_node, srcp_default, lshift_expr_K);
                              TM->ReplaceTreeNode(stmt, ga->op1, op_expr);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Statement transformed in " + GET_NODE(stmt)->ToString());
                              modified = true;
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                           return;
                        }
                        tree_nodeRef op0 = GET_NODE(me->op0);
                        tree_nodeRef op1 = GET_NODE(me->op1);
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(output_uid) + " bitstring: " + bit_values);
                        unsigned int trailing_zero_op0 = 0;
                        unsigned int trailing_zero_op1 = 0;
                        bool is_op0_null = false;
                        bool is_op1_null = false;

                        if(GetPointer<ssa_name>(op0) && GET_NODE(ga->op1)->get_kind() == plus_expr_K)
                        {
                           const std::string& bit_values_op0 = GetPointer<ssa_name>(op0)->bit_values;
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(me->op0)) + " bitstring: " + bit_values_op0);
                           for(auto current_el : boost::adaptors::reverse(bit_values_op0))
                           {
                              if(current_el == '0' || current_el == 'X')
                                 ++trailing_zero_op0;
                              else
                                 break;
                           }
                           if(bit_values_op0 == "0")
                              is_op0_null = true;
                        }
                        else if(GetPointer<integer_cst>(op0) && GET_NODE(ga->op1)->get_kind() == plus_expr_K)
                        {
                           auto* int_const = GetPointer<integer_cst>(op0);
                           auto value_int = static_cast<unsigned long long int>(int_const->value);
                           for(unsigned int index = 0; index < 64 && value_int != 0; ++index)
                           {
                              if(value_int & (1ULL << index))
                                 break;
                              else
                                 ++trailing_zero_op0;
                           }
                           if(int_const->value == 0)
                              is_op0_null = true;
                        }

                        if(GetPointer<ssa_name>(op1))
                        {
                           const std::string& bit_values_op1 = GetPointer<ssa_name>(op1)->bit_values;
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(me->op1)) + " bitstring: " + bit_values_op1);
                           for(auto current_el : boost::adaptors::reverse(bit_values_op1))
                           {
                              if(current_el == '0' || current_el == 'X')
                                 ++trailing_zero_op1;
                              else
                                 break;
                           }
                           if(bit_values_op1 == "0")
                              is_op1_null = true;
                        }
                        else if(GetPointer<integer_cst>(op1))
                        {
                           auto* int_const = GetPointer<integer_cst>(op1);
                           auto value_int = static_cast<unsigned long long int>(int_const->value);
                           for(unsigned int index = 0; index < 64 && value_int != 0; ++index)
                           {
                              if(value_int & (1ULL << index))
                                 break;
                              else
                                 ++trailing_zero_op1;
                           }
                           if(int_const->value == 0)
                              is_op1_null = true;
                        }

                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Trailing zeros op0=" + STR(trailing_zero_op0) + ", trailing zeros op1=" + STR(trailing_zero_op1));
                        if(is_op0_null)
                        {
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, me->op1);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else if(is_op1_null)
                        {
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, me->op0);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else if(trailing_zero_op0 != 0 || trailing_zero_op1 != 0)
                        {
                           modified = true;
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                           const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                           const bool is_first_max = trailing_zero_op0 > trailing_zero_op1;
                           const unsigned int shift_const = is_first_max ? trailing_zero_op0 : trailing_zero_op1;
                           INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Bit Value Opt: " + (GET_NODE(ga->op1)->get_kind() == plus_expr_K ? std::string("plus_expr") : std::string("minus_expr")) + " optimized, nbits = " + STR(shift_const));
                           const tree_nodeRef shift_constant_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(shift_const), type_index);
                           bool is_op0_const = GetPointer<integer_cst>(op0);
                           bool is_op1_const = GetPointer<integer_cst>(op1);
                           const unsigned int op0_type_id = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op0));
                           tree_nodeRef op0_type = TM->GetTreeReindex(op0_type_id);
                           const unsigned int op1_type_id = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op1));
                           tree_nodeRef op1_type = TM->GetTreeReindex(op1_type_id);
                           tree_nodeRef b_node = is_first_max ? me->op1 : me->op0;
                           unsigned int b_type_id = is_first_max ? op1_type_id : op0_type_id;
                           tree_nodeRef b_type = TM->GetTreeReindex(b_type_id);

                           if(is_op0_const)
                           {
                              auto* int_const = GetPointer<integer_cst>(op0);
                              if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op0)))
                              {
                                 if(static_cast<long long int>(int_const->value >> shift_const) == 0)
                                    is_op0_null = GET_NODE(ga->op1)->get_kind() == plus_expr_K; // TODO: true?
                                 TM->ReplaceTreeNode(stmt, me->op0, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> shift_const), op0_type_id));
                              }
                              else
                              {
                                 if(static_cast<unsigned long long int>(int_const->value >> shift_const) == 0)
                                    is_op0_null = GET_NODE(ga->op1)->get_kind() == plus_expr_K; // TODO: true?
                                 TM->ReplaceTreeNode(stmt, me->op0, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> shift_const), op0_type_id));
                              }
                           }
                           else
                           {
                              std::string resulting_bit_values;
                              THROW_ASSERT(GetPointer<ssa_name>(op0), "expected an SSA name");

                              if((GetPointer<ssa_name>(op0)->bit_values.size() - shift_const) > 0)
                                 resulting_bit_values = GetPointer<ssa_name>(op0)->bit_values.substr(0, GetPointer<ssa_name>(op0)->bit_values.size() - shift_const);
                              else if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op0)))
                                 resulting_bit_values = GetPointer<ssa_name>(op0)->bit_values.substr(0, 1);
                              else
                                 resulting_bit_values = "0";

                              if(resulting_bit_values == "0" && GET_NODE(ga->op1)->get_kind() == plus_expr_K)
                              {
                                 is_op0_null = true;
                              }
                              else
                              {
                                 tree_nodeRef op0_expr = IRman->create_binary_operation(op0_type, me->op0, shift_constant_node, srcp_default, rshift_expr_K);
                                 tree_nodeRef op0_ga = IRman->CreateGimpleAssign(op0_type, tree_nodeRef(), tree_nodeRef(), op0_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt);
                                 tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + me->op0->ToString() + " with " + op0_ga_var->ToString() + " in " + stmt->ToString());
                                 TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned by this
                                  * bitshift must not be checked if it was applied to
                                  * a variable marked as address.
                                  */
                                 if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                                 {
                                    AppM->RDiscr->ssa_to_skip_if_address.insert(GET_NODE(op0_ga_var));
                                 }
#endif
                                 /// set the bit_values to the ssa var
                                 auto* op0_ssa = GetPointer<ssa_name>(GET_NODE(op0_ga_var));
                                 op0_ssa->bit_values = resulting_bit_values;
                                 constrainSSA(op0_ssa, TM);
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(op0_ga_var)) + " bitstring: " + STR(op0_ssa->bit_values));
                              }
                           }

                           if(is_op1_const)
                           {
                              auto* int_const = GetPointer<integer_cst>(op1);
                              if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op1)))
                              {
                                 if(static_cast<long long int>(int_const->value >> shift_const) == 0)
                                    is_op1_null = true;
                                 TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> shift_const), op1_type_id));
                              }
                              else
                              {
                                 if(static_cast<unsigned long long int>(int_const->value >> shift_const) == 0)
                                    is_op1_null = true;
                                 TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> shift_const), op1_type_id));
                              }
                           }
                           else
                           {
                              std::string resulting_bit_values;
                              THROW_ASSERT(GetPointer<ssa_name>(op1), "expected an SSA name");

                              if((GetPointer<ssa_name>(op1)->bit_values.size() - shift_const) > 0)
                                 resulting_bit_values = GetPointer<ssa_name>(op1)->bit_values.substr(0, GetPointer<ssa_name>(op1)->bit_values.size() - shift_const);
                              else if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op1)))
                                 resulting_bit_values = GetPointer<ssa_name>(op1)->bit_values.substr(0, 1);
                              else
                                 resulting_bit_values = "0";

                              if(resulting_bit_values == "0")
                              {
                                 is_op1_null = true;
                              }
                              else
                              {
                                 tree_nodeRef op1_expr = IRman->create_binary_operation(op1_type, me->op1, shift_constant_node, srcp_default, rshift_expr_K);
                                 tree_nodeRef op1_ga = IRman->CreateGimpleAssign(op1_type, tree_nodeRef(), tree_nodeRef(), op1_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt);
                                 tree_nodeRef op1_ga_var = GetPointer<gimple_assign>(GET_NODE(op1_ga))->op0;
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Replacing " + me->op1->ToString() + " with " + op1_ga_var->ToString() + " in " + stmt->ToString());
                                 TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned by this
                                  * bitshift must not be checked if it was applied to
                                  * a variable marked as address.
                                  */
                                 if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                                 {
                                    AppM->RDiscr->ssa_to_skip_if_address.insert(GET_NODE(op1_ga_var));
                                 }
#endif
                                 /// set the bit_values to the ssa var
                                 auto* op1_ssa = GetPointer<ssa_name>(GET_NODE(op1_ga_var));
                                 op1_ssa->bit_values = resulting_bit_values;
                                 constrainSSA(op1_ssa, TM);
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(op1_ga_var)) + " bitstring: " + STR(op1_ssa->bit_values));
                              }
                           }

                           tree_nodeRef curr_ga;
                           if(is_op0_null)
                           {
                              curr_ga = IRman->CreateGimpleAssign(op1_type, tree_nodeRef(), tree_nodeRef(), me->op1, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                           }
                           else if(is_op1_null)
                           {
                              curr_ga = IRman->CreateGimpleAssign(op1_type, tree_nodeRef(), tree_nodeRef(), me->op0, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                           }
                           else
                           {
                              curr_ga = IRman->CreateGimpleAssign(op1_type, tree_nodeRef(), tree_nodeRef(), ga->op1, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                           }
                           B->PushBefore(curr_ga, stmt);
                           GetPointer<gimple_assign>(GET_NODE(curr_ga))->orig = stmt;
                           tree_nodeRef curr_ga_var = GetPointer<gimple_assign>(GET_NODE(curr_ga))->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                           /*
                            * for discrepancy analysis, the ssa assigned by this
                            * bitshift must not be checked if it was applied to
                            * a variable marked as address.
                            */
                           if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                           {
                              AppM->RDiscr->ssa_to_skip_if_address.insert(GET_NODE(curr_ga_var));
                           }
#endif
                           /// set the bit_values to the ssa var
                           auto* op_ssa = GetPointer<ssa_name>(GET_NODE(curr_ga_var));
                           op_ssa->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - shift_const);
                           constrainSSA(op_ssa, TM);
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(curr_ga_var)) + " bitstring: " + STR(op_ssa->bit_values));

                           tree_nodeRef op_expr = IRman->create_binary_operation(ga_op_type, curr_ga_var, shift_constant_node, srcp_default, lshift_expr_K);
                           tree_nodeRef lshift_ga = IRman->CreateGimpleAssign(ga_op_type, tree_nodeRef(), tree_nodeRef(), op_expr, B_id, srcp_default);
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(lshift_ga));
                           B->PushBefore(lshift_ga, stmt);
                           tree_nodeRef lshift_ga_var = GetPointer<gimple_assign>(GET_NODE(lshift_ga))->op0;
                           /// set the bit_values to the ssa var
                           auto* lshift_ssa = GetPointer<ssa_name>(GET_NODE(lshift_ga_var));
                           lshift_ssa->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - shift_const);
                           while(lshift_ssa->bit_values.size() < ssa->bit_values.size())
                              lshift_ssa->bit_values.push_back('0');
                           constrainSSA(lshift_ssa, TM);
                           PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(lshift_ga_var)) + " bitstring: " + STR(lshift_ssa->bit_values));

                           bool do_final_or = false;
                           unsigned int n_iter = 0;
                           for(auto cur_bit : boost::adaptors::reverse(ssa->bit_values))
                           {
                              if(cur_bit == '1' || cur_bit == 'U')
                              {
                                 do_final_or = true;
                                 break;
                              }
                              n_iter++;
                              if(n_iter == shift_const)
                                 break;
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
                                 AppM->RDiscr->ssa_to_skip_if_address.insert(GET_NODE(lshift_ga_var));
                              }
#endif
                              if(GetPointer<integer_cst>(GET_NODE(b_node)))
                              {
                                 auto* int_const = GetPointer<integer_cst>(GET_NODE(b_node));
                                 tree_nodeRef b_node_val = TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) & ((1ULL << shift_const) - 1)), b_type_id);
                                 TM->ReplaceTreeNode(stmt, ga->op1, IRman->create_ternary_operation(ga_op_type, lshift_ga_var, b_node_val, shift_constant_node, srcp_default, bit_ior_concat_expr_K));
                              }
                              else
                              {
                                 tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(static_cast<long long int>((1ULL << shift_const) - 1), b_type_id);
                                 tree_nodeRef band_expr = IRman->create_binary_operation(b_type, b_node, bit_mask_constant_node, srcp_default, bit_and_expr_K);
                                 tree_nodeRef band_ga = IRman->CreateGimpleAssign(b_type, tree_nodeRef(), tree_nodeRef(), band_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(band_ga));
                                 B->PushBefore(band_ga, stmt);
                                 tree_nodeRef band_ga_var = GetPointer<gimple_assign>(GET_NODE(band_ga))->op0;
#if HAVE_FROM_DISCREPANCY_BUILT
                                 /*
                                  * for discrepancy analysis, the ssa assigned by this
                                  * bitshift must not be checked if it was applied to
                                  * a variable marked as address.
                                  */
                                 if(parameters->isOption(OPT_discrepancy) and parameters->getOption<bool>(OPT_discrepancy))
                                 {
                                    AppM->RDiscr->ssa_to_skip_if_address.insert(GET_NODE(band_ga_var));
                                 }
#endif
                                 /// set the bit_values to the ssa var
                                 auto* band_ssa = GetPointer<ssa_name>(GET_NODE(band_ga_var));
                                 for(auto cur_bit : boost::adaptors::reverse(ssa->bit_values))
                                 {
                                    band_ssa->bit_values = cur_bit + band_ssa->bit_values;
                                    if(band_ssa->bit_values.size() == shift_const)
                                       break;
                                 }
                                 band_ssa->bit_values = "0" + band_ssa->bit_values;
                                 constrainSSA(band_ssa, TM);
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Var_uid: " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(GET_INDEX_NODE(band_ga_var)) + " bitstring: " + STR(band_ssa->bit_values));

                                 tree_nodeRef res_expr = IRman->create_ternary_operation(ga_op_type, lshift_ga_var, band_ga_var, shift_constant_node, srcp_default, bit_ior_concat_expr_K);
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
                        else if(GET_NODE(ga->op1)->get_kind() == minus_expr_K && GetPointer<integer_cst>(op0) && GetPointer<integer_cst>(op0)->value == 0)
                        {
                           if(!parameters->isOption(OPT_use_ALUs) || !parameters->getOption<bool>(OPT_use_ALUs))
                           {
                              const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              tree_nodeRef res_expr = IRman->create_unary_operation(ga_op_type, me->op1, srcp_default, negate_expr_K);
                              TM->ReplaceTreeNode(stmt, ga->op1, res_expr);
                              modified = true;
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                        }
                     };
                     plus_minus_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == eq_expr_K || GET_NODE(ga->op1)->get_kind() == ne_expr_K)
                  {
                     auto eq_ne_expr_BVO = [&] {
                        auto* me = GetPointer<binary_expr>(GET_NODE(ga->op1));
                        tree_nodeRef op0 = GET_NODE(me->op0);
                        tree_nodeRef op1 = GET_NODE(me->op1);
                        if(tree_helper::CGetType(op0)->get_kind() == real_type_K && tree_helper::CGetType(op1)->get_kind() == real_type_K)
                        {
                           // TODO: adapt existing operations to real type (zero sign bug to be considered)
                           return;
                        }
                        unsigned int op0_size = BitLatticeManipulator::size(TM, GET_INDEX_NODE(me->op0));
                        bool is_op1_zero = false;
                        if(GetPointer<integer_cst>(op1))
                        {
                           auto* ic = GetPointer<integer_cst>(op1);
                           auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                           if(ull_value == 0)
                              is_op1_zero = true;
                        }

                        if(op0 == op1)
                        {
                           long long int const_value = GET_NODE(ga->op1)->get_kind() == eq_expr_K ? 1LL : 0LL;
                           tree_nodeRef val = TM->CreateUniqueIntegerCst(const_value, type_index);
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, val);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else if(is_op1_zero && GET_NODE(ga->op1)->get_kind() == ne_expr_K && op0_size == 1)
                        {
                           unsigned int op0_type_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op0));
                           tree_nodeRef op0_op_type = TM->GetTreeReindex(op0_type_index);
                           unsigned data_bitsize = BitLatticeManipulator::size(TM, op0_type_index);
                           if(data_bitsize == 1)
                           {
                              const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                 if(GET_CONST_NODE(use.first)->get_kind() != gimple_cond_K)
                                 {
                                    TM->ReplaceTreeNode(use.first, ga->op0, me->op0);
                                    modified = true;
                                 }
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
                              else
                              {
                                 const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                                 ga->op1 = IRman->create_unary_operation(ga_op_type, me->op0, srcp_default, nop_expr_K);
                                 modified = true;
                              }

#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                           else
                           {
                              const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              tree_nodeRef one_const_node = TM->CreateUniqueIntegerCst(1, op0_type_index);
                              tree_nodeRef bitwise_masked = IRman->create_binary_operation(op0_op_type, me->op0, one_const_node, srcp_default, bit_and_expr_K);
                              tree_nodeRef op0_ga = IRman->CreateGimpleAssign(op0_op_type, TM->CreateUniqueIntegerCst(0, op0_type_index), TM->CreateUniqueIntegerCst(1, op0_type_index), bitwise_masked, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                              B->PushBefore(op0_ga, stmt);
                              tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;

                              const tree_nodeConstRef type_node = tree_helper::CGetType(GET_NODE(ga->op0));
                              const auto type_id = type_node->index;
                              tree_nodeRef ga_nop = IRman->CreateNopExpr(op0_ga_var, TM->CGetTreeReindex(type_id), tree_nodeRef(), tree_nodeRef());
                              B->PushBefore(ga_nop, stmt);
                              modified = true;
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), ga_nop);
#endif
                              tree_nodeRef nop_ga_var = GetPointer<gimple_assign>(GET_NODE(ga_nop))->op0;
                              TM->ReplaceTreeNode(stmt, ga->op1, nop_ga_var);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
                        }
                        else
                        {
                           rel_expr_BVO();
                        }
                     };
                     eq_ne_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == lt_expr_K || GET_NODE(ga->op1)->get_kind() == gt_expr_K || GET_NODE(ga->op1)->get_kind() == le_expr_K || GET_NODE(ga->op1)->get_kind() == ge_expr_K)
                  {
                     rel_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == bit_and_expr_K || GET_NODE(ga->op1)->get_kind() == bit_xor_expr_K)
                  {
                     auto bit_expr_BVO = [&] {
                        auto* me = GetPointer<binary_expr>(GET_NODE(ga->op1));
                        tree_nodeRef op0 = GET_NODE(me->op0);
                        tree_nodeRef op1 = GET_NODE(me->op1);
                        auto expr_kind = GET_NODE(ga->op1)->get_kind();

                        std::string s0, s1;
                        if(GetPointer<ssa_name>(op0))
                           s0 = GetPointer<ssa_name>(op0)->bit_values;
                        if(GetPointer<ssa_name>(op1))
                           s1 = GetPointer<ssa_name>(op1)->bit_values;
                        unsigned int precision;
                        if(s0.size() && s1.size())
                           precision = static_cast<unsigned int>(std::min(s0.size(), s1.size()));
                        else
                           precision = static_cast<unsigned int>(std::max(s0.size(), s1.size()));

                        if(precision)
                        {
                           unsigned int trailing_zero = 0;
                           if(GetPointer<integer_cst>(op0))
                           {
                              auto* ic = GetPointer<integer_cst>(op0);
                              auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                              s0 = convert_to_binary(ull_value, precision);
                           }
                           if(GetPointer<integer_cst>(op1))
                           {
                              auto* ic = GetPointer<integer_cst>(op1);
                              auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                              s1 = convert_to_binary(ull_value, precision);
                           }
                           for(auto s0it = s0.rbegin(), s1it = s1.rbegin(), s0end = s0.rend(), s1end = s1.rend(); s0it != s0end && s1it != s1end; ++s0it, ++s1it)
                           {
                              if((expr_kind == bit_and_expr_K && (*s0it == '0' || *s1it == '0')) || *s0it == 'X' || *s1it == 'X')
                                 ++trailing_zero;
                              else
                                 break;
                           }
                           if(trailing_zero)
                           {
                              INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "---Bit Value Opt: " + std::string(GET_NODE(ga->op1)->get_kind_text()) + " optimized, nbits = " + STR(trailing_zero));
                              modified = true;
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                              const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                              unsigned int type_index0 = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op0));
                              tree_nodeRef op0_op_type = TM->GetTreeReindex(type_index0);
                              unsigned int type_index1 = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op1));
                              tree_nodeRef op1_op_type = TM->GetTreeReindex(type_index1);

                              if(GetPointer<ssa_name>(op0))
                              {
                                 tree_nodeRef op0_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), type_index0);
                                 tree_nodeRef op0_expr = IRman->create_binary_operation(op0_op_type, me->op0, op0_const_node, srcp_default, rshift_expr_K);
                                 tree_nodeRef op0_ga = IRman->CreateGimpleAssign(op0_op_type, tree_nodeRef(), tree_nodeRef(), op0_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                 B->PushBefore(op0_ga, stmt);
                                 tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op0, op0_ga_var);
                                 /// set the bit_values to the ssa var
                                 auto* op0_ssa = GetPointer<ssa_name>(GET_NODE(op0_ga_var));
                                 op0_ssa->bit_values = GetPointer<ssa_name>(op0)->bit_values.substr(0, GetPointer<ssa_name>(op0)->bit_values.size() - trailing_zero);
                                 constrainSSA(op0_ssa, TM);
                              }
                              else
                              {
                                 auto* int_const = GetPointer<integer_cst>(op0);
                                 if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op0)))
                                    TM->ReplaceTreeNode(stmt, me->op0, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> trailing_zero), type_index0));
                                 else
                                    TM->ReplaceTreeNode(stmt, me->op0, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> trailing_zero), type_index0));
                              }
                              if(GetPointer<ssa_name>(op1))
                              {
                                 tree_nodeRef op1_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), type_index1);
                                 tree_nodeRef op1_expr = IRman->create_binary_operation(op1_op_type, me->op1, op1_const_node, srcp_default, rshift_expr_K);
                                 tree_nodeRef op1_ga = IRman->CreateGimpleAssign(op1_op_type, tree_nodeRef(), tree_nodeRef(), op1_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                 B->PushBefore(op1_ga, stmt);
                                 tree_nodeRef op1_ga_var = GetPointer<gimple_assign>(GET_NODE(op1_ga))->op0;
                                 TM->ReplaceTreeNode(stmt, me->op1, op1_ga_var);
                                 /// set the bit_values to the ssa var
                                 auto* op1_ssa = GetPointer<ssa_name>(GET_NODE(op1_ga_var));
                                 op1_ssa->bit_values = GetPointer<ssa_name>(op1)->bit_values.substr(0, GetPointer<ssa_name>(op1)->bit_values.size() - trailing_zero);
                                 constrainSSA(op1_ssa, TM);
                              }
                              else
                              {
                                 auto* int_const = GetPointer<integer_cst>(op1);
                                 if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op1)))
                                    TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> trailing_zero), type_index1));
                                 else
                                    TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> trailing_zero), type_index1));
                              }

                              tree_nodeRef ssa_vd = IRman->create_ssa_name(tree_nodeRef(), ga_op_type, tree_nodeRef(), tree_nodeRef());
                              auto* sn = GetPointer<ssa_name>(GET_NODE(ssa_vd));
                              /// set the bit_values to the ssa var
                              sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_zero);
                              constrainSSA(sn, TM);
                              tree_nodeRef op_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_zero), type_index);
                              tree_nodeRef op_expr = IRman->create_binary_operation(ga_op_type, ssa_vd, op_const_node, srcp_default, lshift_expr_K);
                              tree_nodeRef curr_ga = IRman->CreateGimpleAssign(ga_op_type, ssa->min, ssa->max, op_expr, B_id, srcp_default);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                              TM->ReplaceTreeNode(curr_ga, GetPointer<gimple_assign>(GET_NODE(curr_ga))->op0, ga->op0);
                              TM->ReplaceTreeNode(stmt, ga->op0, ssa_vd);
                              B->PushAfter(curr_ga, stmt);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "pushed");
                           }
                        }
                     };
                     bit_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == cond_expr_K)
                  {
                     auto cond_expr_BVO = [&] {
                        auto* me = GetPointer<cond_expr>(GET_NODE(ga->op1));
                        if(!tree_helper::is_bool(TM, GET_INDEX_NODE(me->op0)))
                        {
                           /// try to fix cond_expr condition
                           auto cond_op0_ssa = GetPointer<ssa_name>(GET_NODE(me->op0));
                           if(cond_op0_ssa)
                           {
                              auto defStmt = GET_NODE(cond_op0_ssa->CGetDefStmt());
                              if(defStmt->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(defStmt);
                                 auto prev_code1 = GET_NODE(prev_ga->op1)->get_kind();
                                 if(prev_code1 == nop_expr_K)
                                 {
                                    auto ne = GetPointer<nop_expr>(GET_NODE(prev_ga->op1));
                                    if(tree_helper::is_bool(TM, GET_INDEX_NODE(ne->op)))
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace cond_expr condition before: " + stmt->ToString());
                                       TM->ReplaceTreeNode(stmt, me->op0, ne->op);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace cond_expr condition after: " + stmt->ToString());
                                       modified = true;
                                       if(cond_op0_ssa->CGetUseStmts().empty())
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                          restart_dead_code = true;
                                       }
#ifndef NDEBUG
                                       AppM->RegisterTransformation(GetName(), stmt);
#endif
                                    }
                                 }
                              }
                           }
                        }
#ifndef NDEBUG
                        if(not AppM->ApplyNewTransformation())
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Skipped because reached limit of cfg transformations");
                           return;
                        }
#endif
                        tree_nodeRef op0 = GET_NODE(me->op1);
                        tree_nodeRef op1 = GET_NODE(me->op2);
                        tree_nodeRef condition = GET_NODE(me->op0);
                        if(op0 == op1)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with equal operands");
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, me->op1);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else if(condition->get_kind() == integer_cst_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with constant condition");
                           auto* ic = GetPointer<integer_cst>(condition);
                           auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
                              if(ull_value)
                                 TM->ReplaceTreeNode(use.first, ga->op0, me->op1);
                              else
                                 TM->ReplaceTreeNode(use.first, ga->op0, me->op2);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                        else
                        {
                           THROW_ASSERT(op0 != op1, "unexpected condition");
                           std::string s0, s1;
                           if(GetPointer<ssa_name>(op0))
                              s0 = GetPointer<ssa_name>(op0)->bit_values;
                           if(GetPointer<ssa_name>(op1))
                              s1 = GetPointer<ssa_name>(op1)->bit_values;
                           unsigned int precision;
                           precision = static_cast<unsigned int>(std::max(s0.size(), s1.size()));
                           if(GetPointer<integer_cst>(op0))
                           {
                              auto* ic = GetPointer<integer_cst>(op0);
                              auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                              s0 = convert_to_binary(ull_value, std::max(precision, BitLatticeManipulator::Size(op0)));
                           }
                           if(GetPointer<integer_cst>(op1))
                           {
                              auto* ic = GetPointer<integer_cst>(op1);
                              auto ull_value = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                              s1 = convert_to_binary(ull_value, std::max(precision, BitLatticeManipulator::Size(op1)));
                           }
                           precision = static_cast<unsigned int>(std::max(s0.size(), s1.size()));

                           if(precision)
                           {
                              unsigned int trailing_eq = 0;
                              unsigned int minimum_precision = static_cast<unsigned int>(std::min(s0.size(), s1.size()));
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Bit_value strings are " + s0 + " and " + s1);
                              if(precision == 0)
                                 precision = 1;
                              for(unsigned int index = 0; index < (minimum_precision - 1); ++index)
                              {
                                 if((s0[s0.size() - index - 1] == '0' || s0[s0.size() - index - 1] == 'X') && (s1[s1.size() - index - 1] == '0' || s1[s1.size() - index - 1] == 'X'))
                                    ++trailing_eq;
                                 else
                                    break;
                              }
                              if(trailing_eq)
                              {
                                 INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "-->Bit Value Opt: cond_expr optimized, nbits = " + STR(trailing_eq));
                                 INDENT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "<--");
                                 modified = true;
                                 const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                                 unsigned int type_index0 = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op1));
                                 tree_nodeRef op0_op_type = TM->GetTreeReindex(type_index0);
                                 unsigned int type_index1 = tree_helper::get_type_index(TM, GET_INDEX_NODE(me->op2));
                                 tree_nodeRef op1_op_type = TM->GetTreeReindex(type_index1);

                                 if(GetPointer<ssa_name>(op0))
                                 {
                                    tree_nodeRef op0_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), type_index0);
                                    tree_nodeRef op0_expr = IRman->create_binary_operation(op0_op_type, me->op1, op0_const_node, srcp_default, rshift_expr_K);
                                    tree_nodeRef op0_ga = IRman->CreateGimpleAssign(op0_op_type, tree_nodeRef(), tree_nodeRef(), op0_expr, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op0_ga));
                                    B->PushBefore(op0_ga, stmt);
                                    tree_nodeRef op0_ga_var = GetPointer<gimple_assign>(GET_NODE(op0_ga))->op0;
                                    TM->ReplaceTreeNode(stmt, me->op1, op0_ga_var);
                                    /// set the bit_values to the ssa var
                                    auto* op0_ssa = GetPointer<ssa_name>(GET_NODE(op0_ga_var));
                                    op0_ssa->bit_values = GetPointer<ssa_name>(op0)->bit_values.substr(0, GetPointer<ssa_name>(op0)->bit_values.size() - trailing_eq);
                                    constrainSSA(op0_ssa, TM);
                                 }
                                 else
                                 {
                                    auto* int_const = GetPointer<integer_cst>(op0);
                                    if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op0)))
                                       TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> trailing_eq), type_index0));
                                    else
                                       TM->ReplaceTreeNode(stmt, me->op1, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> trailing_eq), type_index0));
                                 }
                                 if(GetPointer<ssa_name>(op1))
                                 {
                                    tree_nodeRef op1_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), type_index1);
                                    tree_nodeRef op1_expr = IRman->create_binary_operation(op1_op_type, me->op2, op1_const_node, srcp_default, rshift_expr_K);
                                    tree_nodeRef op1_ga = IRman->CreateGimpleAssign(op1_op_type, tree_nodeRef(), tree_nodeRef(), op1_expr, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(op1_ga));
                                    B->PushBefore(op1_ga, stmt);
                                    tree_nodeRef op1_ga_var = GetPointer<gimple_assign>(GET_NODE(op1_ga))->op0;
                                    TM->ReplaceTreeNode(stmt, me->op2, op1_ga_var);
                                    /// set the bit_values to the ssa var
                                    auto* op1_ssa = GetPointer<ssa_name>(GET_NODE(op1_ga_var));
                                    op1_ssa->bit_values = GetPointer<ssa_name>(op1)->bit_values.substr(0, GetPointer<ssa_name>(op1)->bit_values.size() - trailing_eq);
                                    constrainSSA(op1_ssa, TM);
                                 }
                                 else
                                 {
                                    auto* int_const = GetPointer<integer_cst>(op1);
                                    if(tree_helper::is_int(TM, GET_INDEX_NODE(me->op2)))
                                       TM->ReplaceTreeNode(stmt, me->op2, TM->CreateUniqueIntegerCst(static_cast<long long int>(int_const->value >> trailing_eq), type_index1));
                                    else
                                       TM->ReplaceTreeNode(stmt, me->op2, TM->CreateUniqueIntegerCst(static_cast<long long int>(static_cast<unsigned long long int>(int_const->value) >> trailing_eq), type_index1));
                                 }
                                 tree_nodeRef ssa_vd = IRman->create_ssa_name(tree_nodeRef(), ga_op_type, tree_nodeRef(), tree_nodeRef());
                                 auto* sn = GetPointer<ssa_name>(GET_NODE(ssa_vd));
                                 /// set the bit_values to the ssa var
                                 if(ssa->bit_values.size())
                                 {
                                    sn->bit_values = ssa->bit_values.substr(0, ssa->bit_values.size() - trailing_eq);
                                    constrainSSA(sn, TM);
                                 }
                                 tree_nodeRef op_const_node = TM->CreateUniqueIntegerCst(static_cast<long long int>(trailing_eq), type_index);
                                 tree_nodeRef op_expr = IRman->create_binary_operation(ga_op_type, ssa_vd, op_const_node, srcp_default, lshift_expr_K);
                                 tree_nodeRef curr_ga = IRman->CreateGimpleAssign(ga_op_type, ssa->min, ssa->max, op_expr, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Created " + STR(curr_ga));
                                 TM->ReplaceTreeNode(curr_ga, GetPointer<gimple_assign>(GET_NODE(curr_ga))->op0, ga->op0);
                                 TM->ReplaceTreeNode(stmt, ga->op0, ssa_vd);
                                 B->PushAfter(curr_ga, stmt);
                                 modified = true;
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), stmt);
#endif
                              }
                              else if(precision == 1 && s0 == "1" && s1 == "0")
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with true and false");
                                 tree_nodeRef ga_nop = IRman->CreateNopExpr(me->op0, GetPointer<ternary_expr>(GET_NODE(ga->op1))->type, ssa->min, ssa->max);
                                 B->PushBefore(ga_nop, stmt);
                                 modified = true;
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), ga_nop);
#endif
                                 tree_nodeRef nop_ga_var = GetPointer<gimple_assign>(GET_NODE(ga_nop))->op0;

                                 const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                 for(const auto& use : StmtUses)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
                                    TM->ReplaceTreeNode(use.first, ga->op0, nop_ga_var);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
                                    modified = true;
                                 }
                                 if(ssa->CGetUseStmts().empty())
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                    restart_dead_code = true;
                                 }
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), stmt);
#endif
                              }
                              else if(precision == 1 and s0 == "0" and s1 == "1")
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Cond expr with false and true");
                                 /// second argument is null since we cannot add the new statement at the end of the current BB
                                 const auto new_ssa = IRman->CreateNotExpr(me->op0, blocRef());
                                 const auto new_stmt = GetPointer<const ssa_name>(GET_CONST_NODE(new_ssa))->CGetDefStmt();
                                 B->PushBefore(new_stmt, stmt);
                                 tree_nodeRef ga_nop = IRman->CreateNopExpr(new_ssa, GetPointer<ternary_expr>(GET_NODE(ga->op1))->type, ssa->min, ssa->max);
                                 B->PushBefore(ga_nop, stmt);
                                 tree_nodeRef nop_ga_var = GetPointer<gimple_assign>(GET_NODE(ga_nop))->op0;
                                 const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                 for(const auto& use : StmtUses)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage before: " + use.first->ToString());
                                    TM->ReplaceTreeNode(use.first, ga->op0, nop_ga_var);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace var usage after: " + use.first->ToString());
                                    modified = true;
                                 }
                                 if(ssa->CGetUseStmts().empty())
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                    restart_dead_code = true;
                                 }
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), stmt);
#endif
                              }
                           }
                           else if(GetPointer<integer_cst>(op0) && GetPointer<integer_cst>(op1))
                           {
                              auto* ic = GetPointer<integer_cst>(op0);
                              auto ull_value0 = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                              ic = GetPointer<integer_cst>(op1);
                              auto ull_value1 = static_cast<unsigned long long int>(tree_helper::get_integer_cst_value(ic));
                              if(ull_value0 == 1 && ull_value1 == 0)
                              {
                                 tree_nodeRef ga_nop = IRman->CreateNopExpr(me->op0, GetPointer<ternary_expr>(GET_NODE(ga->op1))->type, TM->CreateUniqueIntegerCst(0, GetPointer<ternary_expr>(GET_NODE(ga->op1))->type->index),
                                                                            TM->CreateUniqueIntegerCst(1, GetPointer<ternary_expr>(GET_NODE(ga->op1))->type->index));
                                 B->PushBefore(ga_nop, stmt);
                                 modified = true;
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), ga_nop);
#endif
                                 tree_nodeRef nop_ga_var = GetPointer<gimple_assign>(GET_NODE(ga_nop))->op0;
                                 TM->ReplaceTreeNode(stmt, ga->op1, nop_ga_var);
                              }
                           }
                        }
                     };
                     cond_expr_BVO();
                  }
#if !HAVE_STDCXX_17

                  else if(GET_NODE(ga->op1)->get_kind() == truth_not_expr_K)
                  {
                     auto tne_BVO = [&] {
                        auto* tne = GetPointer<truth_not_expr>(GET_NODE(ga->op1));
                        if(GET_NODE(tne->op)->get_kind() == integer_cst_K)
                        {
                           auto* int_const = GetPointer<integer_cst>(GET_NODE(tne->op));
                           long long int const_value = int_const->value == 0 ? 1LL : 0LL;
                           tree_nodeRef val = TM->CreateUniqueIntegerCst(const_value, type_index);
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, val);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                     };
                     tne_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == truth_and_expr_K)
                  {
                     auto tae_BVO = [&] {
                        auto* tae = GetPointer<truth_and_expr>(GET_NODE(ga->op1));
                        if(GET_NODE(tae->op0)->get_kind() == integer_cst_K || GET_NODE(tae->op1)->get_kind() == integer_cst_K || GET_INDEX_NODE(tae->op0) == GET_INDEX_NODE(tae->op1))
                        {
                           tree_nodeRef val;
                           if(GET_NODE(tae->op0)->get_kind() == integer_cst_K)
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(tae->op0));
                              if(int_const->value == 0)
                                 val = tae->op0;
                              else
                                 val = tae->op1;
                           }
                           else if(GET_NODE(tae->op1)->get_kind() == integer_cst_K)
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(tae->op1));
                              if(int_const->value == 0)
                                 val = tae->op1;
                              else
                                 val = tae->op0;
                           }
                           else
                           {
                              val = tae->op0;
                           }
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, val);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                     };
                     tae_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == truth_or_expr_K)
                  {
                     auto toe_BVO = [&] {
                        auto* toe = GetPointer<truth_or_expr>(GET_NODE(ga->op1));
                        if(GET_NODE(toe->op0)->get_kind() == integer_cst_K || GET_NODE(toe->op1)->get_kind() == integer_cst_K || GET_INDEX_NODE(toe->op0) == GET_INDEX_NODE(toe->op1))
                        {
                           tree_nodeRef val;
                           if(GET_NODE(toe->op0)->get_kind() == integer_cst_K)
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(toe->op0));
                              if(int_const->value == 0)
                                 val = toe->op1;
                              else
                                 val = toe->op0;
                           }
                           else if(GET_NODE(toe->op1)->get_kind() == integer_cst_K)
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(toe->op1));
                              if(int_const->value == 0)
                                 val = toe->op0;
                              else
                                 val = toe->op1;
                           }
                           else
                           {
                              val = toe->op0;
                           }
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, val);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                     };
                     toe_BVO();
                  }
#endif
                  else if(GET_NODE(ga->op1)->get_kind() == bit_ior_expr_K)
                  {
                     auto bit_ior_expr_BVO = [&] {
                        auto* bie = GetPointer<bit_ior_expr>(GET_NODE(ga->op1));
                        if(GET_NODE(bie->op0)->get_kind() == integer_cst_K || GET_NODE(bie->op1)->get_kind() == integer_cst_K)
                        {
                           tree_nodeRef val;
                           if(GET_NODE(bie->op0)->get_kind() == integer_cst_K)
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(bie->op0));
                              if(int_const->value == 0)
                                 val = bie->op1;
                           }
                           else
                           {
                              auto* int_const = GetPointer<integer_cst>(GET_NODE(bie->op1));
                              if(int_const->value == 0)
                                 val = bie->op0;
                           }
                           if(val)
                           {
                              const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                 TM->ReplaceTreeNode(use.first, ga->op0, val);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                 modified = true;
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                        }
                     };
                     bit_ior_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == pointer_plus_expr_K)
                  {
                     auto pointer_plus_expr_BVO = [&] {
                        auto* ppe = GetPointer<pointer_plus_expr>(GET_NODE(ga->op1));
                        if(GET_NODE(ppe->op1)->get_kind() == integer_cst_K)
                        {
                           auto* int_const = GetPointer<integer_cst>(GET_NODE(ppe->op1));
                           if(int_const->value == 0)
                           {
                              const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                 TM->ReplaceTreeNode(use.first, ga->op0, ppe->op0);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                 modified = true;
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                           else if(GetPointer<ssa_name>(GET_NODE(ppe->op0)))
                           {
                              auto temp_def = GET_NODE(GetPointer<const ssa_name>(GET_NODE(ppe->op0))->CGetDefStmt());
                              if(temp_def->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(temp_def);
                                 if(GET_NODE(prev_ga->op1)->get_kind() == pointer_plus_expr_K)
                                 {
                                    const auto prev_ppe = GetPointer<const pointer_plus_expr>(GET_NODE(prev_ga->op1));
                                    if(GetPointer<ssa_name>(GET_NODE(prev_ppe->op0)) && GetPointer<integer_cst>(GET_NODE(prev_ppe->op1)))
                                    {
                                       auto* ssa_ppe_op0 = GetPointer<ssa_name>(GET_NODE(ppe->op0));

                                       auto prev_val = static_cast<size_t>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(prev_ppe->op1))));
                                       auto curr_val = static_cast<size_t>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(ppe->op1))));
                                       unsigned int type_ppe_op1_index = tree_helper::get_type_index(TM, GET_INDEX_NODE(ppe->op1));
                                       ppe->op1 = TM->CreateUniqueIntegerCst(static_cast<long long int>(prev_val + curr_val), type_ppe_op1_index);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + stmt->ToString());
                                       TM->ReplaceTreeNode(stmt, ppe->op0, prev_ppe->op0);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + stmt->ToString());
                                       if(ssa_ppe_op0->CGetUseStmts().empty())
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                          restart_dead_code = true;
                                       }
                                       modified = true;
#ifndef NDEBUG
                                       AppM->RegisterTransformation(GetName(), stmt);
#endif
                                    }
                                 }
                              }
                           }
                        }
                     };
                     pointer_plus_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == addr_expr_K)
                  {
                     auto addr_expr_BVO = [&] {
                        auto* ae = GetPointer<addr_expr>(GET_NODE(ga->op1));
                        enum kind ae_code = GET_NODE(ae->op)->get_kind();
                        if(ae_code == mem_ref_K)
                        {
                           auto* MR = GetPointer<mem_ref>(GET_NODE(ae->op));
                           tree_nodeRef op1 = MR->op1;
                           long long int op1_val = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(op1)));
                           if(op1_val == 0 && GET_NODE(MR->op0)->get_kind() == ssa_name_K)
                           {
                              auto temp_def = GET_NODE(GetPointer<const ssa_name>(GET_NODE(MR->op0))->CGetDefStmt());
                              if(temp_def->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(temp_def);
                                 if(GET_NODE(prev_ga->op1)->get_kind() == addr_expr_K)
                                 {
                                    const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                    for(const auto& use : StmtUses)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                       TM->ReplaceTreeNode(use.first, ga->op0, prev_ga->op0);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                       modified = true;
                                    }
                                    if(ssa->CGetUseStmts().empty())
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                       restart_dead_code = true;
                                    }
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                              }
                           }
                           else if(op1_val == 0 && GET_NODE(MR->op0)->get_kind() == integer_cst_K)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + stmt->ToString());
                              TM->ReplaceTreeNode(stmt, ga->op1, MR->op0);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + stmt->ToString());
                           }
                        }
                     };
                     addr_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == extract_bit_expr_K)
                  {
                     auto extract_bit_expr_BVO = [&] {
                        const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                        auto* ebe = GetPointer<extract_bit_expr>(GET_NODE(ga->op1));
                        THROW_ASSERT(GET_NODE(ebe->op1)->get_kind() == integer_cst_K, "unexpected condition");
                        auto pos_value = GetPointer<integer_cst>(GET_NODE(ebe->op1))->value;
                        auto ebe_op0_ssa = GetPointer<ssa_name>(GET_NODE(ebe->op0));
                        if(ebe_op0_ssa)
                        {
                           if(BitLatticeManipulator::Size(ebe->op0) <= pos_value)
                           {
                              const auto right_id = GET_INDEX_NODE(ebe->op0);
                              const bool right_signed = tree_helper::is_int(TM, right_id);
                              if(right_signed)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                 tree_nodeRef new_pos = TM->CreateUniqueIntegerCst(BitLatticeManipulator::Size(ebe->op0) - 1, GET_INDEX_NODE(GetPointer<integer_cst>(GET_NODE(ebe->op1))->type));
                                 tree_nodeRef eb_op = IRman->create_extract_bit_expr(ebe->op0, new_pos, srcp_default);
                                 tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                 B->PushBefore(eb_ga, stmt);
                                 tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                 auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                 TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                 modified = true;
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), stmt);
#endif
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                 tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type));
                                 const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                 for(const auto& use : StmtUses)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                    TM->ReplaceTreeNode(use.first, ga->op0, zero_node);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                    modified = true;
                                 }
                                 if(ssa->CGetUseStmts().empty())
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                    restart_dead_code = true;
                                 }
#ifndef NDEBUG
                                 AppM->RegisterTransformation(GetName(), stmt);
#endif
                              }
                           }
                           else if(tree_helper::is_bool(TM, GET_INDEX_NODE(ebe->op0)))
                           {
                              const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr input bool usage before: " + use.first->ToString());
                                 TM->ReplaceTreeNode(use.first, ga->op0, ebe->op0);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr input bool usage after: " + use.first->ToString());
                                 modified = true;
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                           else
                           {
                              auto defStmt = GET_NODE(ebe_op0_ssa->CGetDefStmt());
                              if(defStmt->get_kind() == gimple_assign_K)
                              {
                                 const auto prev_ga = GetPointer<const gimple_assign>(defStmt);
                                 auto prev_code1 = GET_NODE(prev_ga->op1)->get_kind();
                                 if(prev_code1 == nop_expr_K || prev_code1 == convert_expr_K)
                                 {
                                    auto ne = GetPointer<unary_expr>(GET_NODE(prev_ga->op1));
                                    if(tree_helper::is_bool(TM, GET_INDEX_NODE(ne->op)))
                                    {
                                       if(pos_value == 0)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                          auto masking = IRman->create_binary_operation(ebe->type, ne->op, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                          modified = true;
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                       else
                                       {
                                          tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type));
                                          const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                          for(const auto& use : StmtUses)
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr constant usage before: " + use.first->ToString());
                                             TM->ReplaceTreeNode(use.first, ga->op0, zero_node);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr constant usage after: " + use.first->ToString());
                                             modified = true;
                                          }
                                          if(ssa->CGetUseStmts().empty())
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                             restart_dead_code = true;
                                          }
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                    }
                                    else
                                    {
                                       const tree_nodeConstRef neType_node = tree_helper::CGetType(GET_NODE(ne->op));
                                       if(neType_node->get_kind() == integer_type_K)
                                       {
                                          if(BitLatticeManipulator::Size(ne->op) > pos_value)
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                             tree_nodeRef eb_op = IRman->create_extract_bit_expr(ne->op, ebe->op1, srcp_default);
                                             tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                             B->PushBefore(eb_ga, stmt);
                                             tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                             auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                             TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                             modified = true;
#ifndef NDEBUG
                                             AppM->RegisterTransformation(GetName(), stmt);
#endif
                                          }
                                          else
                                          {
                                             const auto right_id = GET_INDEX_NODE(ne->op);
                                             const bool right_signed = tree_helper::is_int(TM, right_id);
                                             if(right_signed)
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                                tree_nodeRef new_pos = TM->CreateUniqueIntegerCst(BitLatticeManipulator::Size(ne->op) - 1, GET_INDEX_NODE(GetPointer<integer_cst>(GET_NODE(ebe->op1))->type));
                                                tree_nodeRef eb_op = IRman->create_extract_bit_expr(ne->op, new_pos, srcp_default);
                                                tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                                B->PushBefore(eb_ga, stmt);
                                                tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                                auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                                TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                                modified = true;
#ifndef NDEBUG
                                                AppM->RegisterTransformation(GetName(), stmt);
#endif
                                             }
                                             else
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                                tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type));
                                                const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                                for(const auto& use : StmtUses)
                                                {
                                                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                                   TM->ReplaceTreeNode(use.first, ga->op0, zero_node);
                                                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                                   modified = true;
                                                }
                                                if(ssa->CGetUseStmts().empty())
                                                {
                                                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                                   restart_dead_code = true;
                                                }
#ifndef NDEBUG
                                                AppM->RegisterTransformation(GetName(), stmt);
#endif
                                             }
                                          }
                                       }
                                    }
                                 }
                                 else if(prev_code1 == bit_and_expr_K)
                                 {
                                    auto bae = GetPointer<bit_and_expr>(GET_NODE(prev_ga->op1));
                                    if(GET_NODE(bae->op0)->get_kind() == integer_cst_K || GET_NODE(bae->op1)->get_kind() == integer_cst_K)
                                    {
                                       auto bae_op0 = bae->op0;
                                       auto bae_op1 = bae->op1;
                                       if(GET_NODE(bae->op0)->get_kind() == integer_cst_K)
                                          std::swap(bae_op0, bae_op1);
                                       auto bae_mask_value = GetPointer<integer_cst>(GET_NODE(bae_op1))->value;
                                       auto masked_value = (bae_mask_value & (1ll << pos_value));
                                       if(masked_value && GET_NODE(bae_op0)->get_kind() != integer_cst_K)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          tree_nodeRef eb_op = IRman->create_extract_bit_expr(bae_op0, ebe->op1, srcp_default);
                                          tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                          B->PushBefore(eb_ga, stmt);
                                          tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                          auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                          modified = true;
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(masked_value ? 1 : 0, GET_INDEX_NODE(ebe->type));
                                          const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                          for(const auto& use : StmtUses)
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                             TM->ReplaceTreeNode(use.first, ga->op0, zero_node);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                             modified = true;
                                          }
                                          if(ssa->CGetUseStmts().empty())
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                             restart_dead_code = true;
                                          }
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                    }
                                 }
                                 else if(prev_code1 == bit_ior_concat_expr_K)
                                 {
                                    auto bice = GetPointer<bit_ior_concat_expr>(GET_NODE(prev_ga->op1));
                                    THROW_ASSERT(GET_NODE(bice->op2)->get_kind() == integer_cst_K, "unexpected condition");
                                    auto nbit_value = GetPointer<integer_cst>(GET_NODE(bice->op2))->value;
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    tree_nodeRef eb_op = IRman->create_extract_bit_expr(nbit_value > pos_value ? bice->op1 : bice->op0, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt);
                                    tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                    auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                                 else if(prev_code1 == lshift_expr_K)
                                 {
                                    auto lse = GetPointer<lshift_expr>(GET_NODE(prev_ga->op1));
                                    if(GET_NODE(lse->op1)->get_kind() == integer_cst_K)
                                    {
                                       auto lsbit_value = GetPointer<integer_cst>(GET_NODE(lse->op1))->value;
                                       if((pos_value - lsbit_value) >= 0)
                                       {
                                          tree_nodeRef new_pos = TM->CreateUniqueIntegerCst(pos_value - lsbit_value, GET_INDEX_NODE(GetPointer<integer_cst>(GET_NODE(ebe->op1))->type));
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          tree_nodeRef eb_op = IRman->create_extract_bit_expr(lse->op0, new_pos, srcp_default);
                                          tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                          B->PushBefore(eb_ga, stmt);
                                          tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                          auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                          modified = true;
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type));
                                          const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                                          for(const auto& use : StmtUses)
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage before: " + use.first->ToString());
                                             TM->ReplaceTreeNode(use.first, ga->op0, zero_node);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace constant usage after: " + use.first->ToString());
                                             modified = true;
                                          }
                                          if(ssa->CGetUseStmts().empty())
                                          {
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                             restart_dead_code = true;
                                          }
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                    }
                                 }
                                 else if(prev_code1 == rshift_expr_K)
                                 {
                                    auto rse = GetPointer<rshift_expr>(GET_NODE(prev_ga->op1));
                                    if(GET_NODE(rse->op1)->get_kind() == integer_cst_K)
                                    {
                                       auto rsbit_value = GetPointer<integer_cst>(GET_NODE(rse->op1))->value;
                                       THROW_ASSERT((pos_value + rsbit_value) >= 0, "unexpected condition");
                                       tree_nodeRef new_pos = TM->CreateUniqueIntegerCst(pos_value + rsbit_value, GET_INDEX_NODE(GetPointer<integer_cst>(GET_NODE(ebe->op1))->type));
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                       tree_nodeRef eb_op = IRman->create_extract_bit_expr(rse->op0, new_pos, srcp_default);
                                       tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                       B->PushBefore(eb_ga, stmt);
                                       tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                       auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                       TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                       modified = true;
#ifndef NDEBUG
                                       AppM->RegisterTransformation(GetName(), stmt);
#endif
                                    }
                                    else if(GET_NODE(rse->op0)->get_kind() == integer_cst_K)
                                    {
                                       long long res_value;
                                       if(tree_helper::is_int(TM, GET_INDEX_NODE(rse->op0)))
                                       {
                                          auto val = GetPointer<integer_cst>(GET_NODE(rse->op0))->value;
                                          val = (val >> pos_value);
                                          res_value = val;
                                       }
                                       else
                                       {
                                          auto val = static_cast<unsigned long long>(GetPointer<integer_cst>(GET_NODE(rse->op0))->value);
                                          val = (val >> pos_value);
                                          res_value = static_cast<long long>(val);
                                       }
                                       if(res_value)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          unsigned int precision = BitLatticeManipulator::Size(ebe_op0_ssa->type);
                                          unsigned int log2;
                                          for(log2 = 1; precision > (1u << log2); ++log2)
                                             ;
                                          tree_nodeRef op1, op2, op3, op4, op5, op6, op7, op8;
                                          for(auto i = 0u; i < log2; ++i)
                                          {
                                             tree_nodeRef new_pos = TM->CreateUniqueIntegerCst(i, GET_INDEX_NODE(GetPointer<integer_cst>(GET_NODE(ebe->op1))->type));
                                             tree_nodeRef eb_op = IRman->create_extract_bit_expr(rse->op1, new_pos, srcp_default);
                                             tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                             B->PushBefore(eb_ga, stmt);
                                             auto eb_ga_ssa_var = GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0;
                                             if(i == 0)
                                                op1 = eb_ga_ssa_var;
                                             else if(i == 1)
                                                op2 = eb_ga_ssa_var;
                                             else if(i == 2)
                                                op3 = eb_ga_ssa_var;
                                             else if(i == 3)
                                                op4 = eb_ga_ssa_var;
                                             else if(i == 4)
                                                op5 = eb_ga_ssa_var;
                                             else if(i == 5)
                                                op6 = eb_ga_ssa_var;
                                             else
                                                THROW_ERROR("unexpected condition");
                                          }
                                          const auto LutConstType = IRman->CreateDefaultUnsignedLongLongInt();

                                          tree_nodeRef lut_constant_node = TM->CreateUniqueIntegerCst(res_value, GET_INDEX_NODE(LutConstType));
                                          tree_nodeRef eb_op = IRman->create_lut_expr(ebe->type, lut_constant_node, op1, op2, op3, op4, op5, op6, op7, op8, srcp_default);
                                          tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                          B->PushBefore(eb_ga, stmt);
                                          tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type));
                                          auto masking = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                          TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                          modified = true;
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                       else
                                       {
                                          tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type));
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                          TM->ReplaceTreeNode(stmt, ga->op1, zero_node);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                          modified = true;
#ifndef NDEBUG
                                          AppM->RegisterTransformation(GetName(), stmt);
#endif
                                       }
                                    }
                                 }
                                 else if(prev_code1 == bit_not_expr_K)
                                 {
                                    auto bne = GetPointer<bit_not_expr>(GET_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    tree_nodeRef eb_op = IRman->create_extract_bit_expr(bne->op, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt);
                                    auto negating = IRman->create_unary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, srcp_default, truth_not_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, negating);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                                 else if(prev_code1 == bit_and_expr_K)
                                 {
                                    auto bae = GetPointer<bit_and_expr>(GET_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    tree_nodeRef eb_op0 = IRman->create_extract_bit_expr(bae->op0, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga0 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op0, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                                    B->PushBefore(eb_ga0, stmt);
                                    tree_nodeRef eb_op1 = IRman->create_extract_bit_expr(bae->op1, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op1, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt);
                                    auto anding = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga0))->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, srcp_default, truth_and_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, anding);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                                 else if(prev_code1 == bit_ior_expr_K)
                                 {
                                    auto bie = GetPointer<bit_ior_expr>(GET_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    tree_nodeRef eb_op0 = IRman->create_extract_bit_expr(bie->op0, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga0 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op0, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                                    B->PushBefore(eb_ga0, stmt);
                                    tree_nodeRef eb_op1 = IRman->create_extract_bit_expr(bie->op1, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op1, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt);
                                    auto anding = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga0))->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, srcp_default, truth_or_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, anding);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                                 else if(prev_code1 == bit_xor_expr_K)
                                 {
                                    auto bxe = GetPointer<bit_xor_expr>(GET_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    tree_nodeRef eb_op0 = IRman->create_extract_bit_expr(bxe->op0, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga0 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op0, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga0));
                                    B->PushBefore(eb_ga0, stmt);
                                    tree_nodeRef eb_op1 = IRman->create_extract_bit_expr(bxe->op1, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op1, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt);
                                    auto anding = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga0))->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, srcp_default, truth_xor_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, anding);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                                 else if(prev_code1 == cond_expr_K)
                                 {
                                    auto ce = GetPointer<cond_expr>(GET_NODE(prev_ga->op1));
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    tree_nodeRef eb_op1 = IRman->create_extract_bit_expr(ce->op1, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op1, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                    B->PushBefore(eb_ga1, stmt);
                                    tree_nodeRef eb_op2 = IRman->create_extract_bit_expr(ce->op2, ebe->op1, srcp_default);
                                    tree_nodeRef eb_ga2 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op2, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga2));
                                    B->PushBefore(eb_ga2, stmt);
                                    auto ceRes = IRman->create_ternary_operation(ebe->type, ce->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga2))->op0, srcp_default, cond_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, ceRes);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                                 else if(prev_code1 == plus_expr_K)
                                 {
                                    THROW_ASSERT(GetPointer<const HLS_manager>(AppM)->get_HLS_target(), "unexpected condition");
                                    const auto hls_target = GetPointer<const HLS_manager>(AppM)->get_HLS_target();
                                    THROW_ASSERT(hls_target->get_target_device()->has_parameter("max_lut_size"), "");
                                    auto max_lut_size = hls_target->get_target_device()->get_parameter<size_t>("max_lut_size");
                                    auto pe = GetPointer<plus_expr>(GET_NODE(prev_ga->op1));
                                    if(GET_NODE(pe->op1)->get_kind() == integer_cst_K && BitLatticeManipulator::Size(ebe->op0) <= max_lut_size)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                       tree_nodeRef carry = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type));
                                       tree_nodeRef sum;
                                       for(long long int bitIndex = 0; bitIndex <= pos_value; ++bitIndex)
                                       {
                                          tree_nodeRef bitIndex_node = TM->CreateUniqueIntegerCst(bitIndex, GET_INDEX_NODE(GetPointer<integer_cst>(GET_NODE(ebe->op1))->type));
                                          tree_nodeRef eb_op1 = IRman->create_extract_bit_expr(pe->op0, bitIndex_node, srcp_default);
                                          tree_nodeRef eb_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op1, B_id, srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga1));
                                          B->PushBefore(eb_ga1, stmt);
                                          tree_nodeRef eb_op2 = IRman->create_extract_bit_expr(pe->op1, bitIndex_node, srcp_default);
                                          tree_nodeRef eb_ga2 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), eb_op2, B_id, srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga2));
                                          B->PushBefore(eb_ga2, stmt);
                                          auto sum0 = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga2))->op0, srcp_default, truth_xor_expr_K);
                                          tree_nodeRef sum0_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), sum0, B_id, srcp_default);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(sum0_ga1));
                                          B->PushBefore(sum0_ga1, stmt);
                                          sum = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(sum0_ga1))->op0, carry, srcp_default, truth_xor_expr_K);
                                          if(bitIndex < pos_value)
                                          {
                                             auto sum_ga1 = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), sum, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(sum_ga1));
                                             B->PushBefore(sum_ga1, stmt);

                                             auto and1 = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, carry, srcp_default, truth_and_expr_K);
                                             auto and1_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), and1, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(and1_ga));
                                             B->PushBefore(and1_ga, stmt);

                                             auto and2 = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga2))->op0, carry, srcp_default, truth_and_expr_K);
                                             auto and2_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), and2, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(and2_ga));
                                             B->PushBefore(and2_ga, stmt);

                                             auto and3 = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(eb_ga1))->op0, GetPointer<gimple_assign>(GET_NODE(eb_ga2))->op0, srcp_default, truth_and_expr_K);
                                             auto and3_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), and3, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(and3_ga));
                                             B->PushBefore(and3_ga, stmt);

                                             auto or1 = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(and1_ga))->op0, GetPointer<gimple_assign>(GET_NODE(and2_ga))->op0, srcp_default, truth_or_expr_K);
                                             auto or1_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), or1, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(or1_ga));
                                             B->PushBefore(or1_ga, stmt);

                                             auto carry1 = IRman->create_binary_operation(ebe->type, GetPointer<gimple_assign>(GET_NODE(or1_ga))->op0, GetPointer<gimple_assign>(GET_NODE(and3_ga))->op0, srcp_default, truth_or_expr_K);
                                             auto carry1_ga = IRman->CreateGimpleAssign(ebe->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ebe->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ebe->type)), carry1, B_id, srcp_default);
                                             INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(carry1_ga));
                                             B->PushBefore(carry1_ga, stmt);
                                             carry = GetPointer<gimple_assign>(GET_NODE(carry1_ga))->op0;
                                          }
                                       }
                                       TM->ReplaceTreeNode(stmt, ga->op1, sum);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                       modified = true;
#ifndef NDEBUG
                                       AppM->RegisterTransformation(GetName(), stmt);
#endif
                                    }
                                 }
                              }
                           }
                        }
                        else if(GET_NODE(ebe->op0)->get_kind() == integer_cst_K)
                        {
                           bool res_value;
                           if(tree_helper::is_int(TM, GET_INDEX_NODE(ebe->op0)))
                           {
                              auto val = GetPointer<integer_cst>(GET_NODE(ebe->op0))->value;
                              val = (val >> pos_value) & 1;
                              res_value = val;
                           }
                           else
                           {
                              auto val = static_cast<unsigned long long>(GetPointer<integer_cst>(GET_NODE(ebe->op0))->value);
                              val = (val >> pos_value) & 1;
                              res_value = val;
                           }
                           tree_nodeRef res_node = TM->CreateUniqueIntegerCst(res_value, GET_INDEX_NODE(ebe->type));
                           const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                           for(const auto& use : StmtUses)
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr constant input usage before: " + use.first->ToString());
                              TM->ReplaceTreeNode(use.first, ga->op0, res_node);
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr constant input usage after: " + use.first->ToString());
                              modified = true;
                           }
                           if(ssa->CGetUseStmts().empty())
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                              restart_dead_code = true;
                           }
#ifndef NDEBUG
                           AppM->RegisterTransformation(GetName(), stmt);
#endif
                        }
                     };
                     extract_bit_expr_BVO();
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == nop_expr_K)
                  {
                     auto nop_expr_BVO = [&] {
                        auto* ne = GetPointer<nop_expr>(GET_NODE(ga->op1));
                        if(tree_helper::is_bool(TM, GET_INDEX_NODE(ga->op0)))
                        {
                           if(tree_helper::is_bool(TM, GET_INDEX_NODE(ne->op)))
                           {
                              const TreeNodeMap<size_t> StmtUses = ssa->CGetUseStmts();
                              for(const auto& use : StmtUses)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace bool nop usage before: " + use.first->ToString());
                                 TM->ReplaceTreeNode(use.first, ga->op0, ne->op);
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace bool nop usage after: " + use.first->ToString());
                                 modified = true;
                              }
                              if(ssa->CGetUseStmts().empty())
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Restarted dead code");
                                 restart_dead_code = true;
                              }
#ifndef NDEBUG
                              AppM->RegisterTransformation(GetName(), stmt);
#endif
                           }
                           else
                           {
                              auto ne_op_ssa = GetPointer<ssa_name>(GET_NODE(ne->op));
                              if(ne_op_ssa)
                              {
                                 if(GET_NODE(ne_op_ssa->type)->get_kind() == integer_type_K)
                                 {
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage before: " + stmt->ToString());
                                    const auto indexType = IRman->CreateDefaultUnsignedLongLongInt();
                                    tree_nodeRef zero_node = TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(indexType));
                                    const std::string srcp_default = ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
                                    tree_nodeRef eb_op = IRman->create_extract_bit_expr(ne->op, zero_node, srcp_default);
                                    tree_nodeRef eb_ga = IRman->CreateGimpleAssign(ne->type, TM->CreateUniqueIntegerCst(0, GET_INDEX_NODE(ne->type)), TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ne->type)), eb_op, B_id, srcp_default);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Created " + STR(eb_ga));
                                    B->PushBefore(eb_ga, stmt);
                                    tree_nodeRef bit_mask_constant_node = TM->CreateUniqueIntegerCst(1, GET_INDEX_NODE(ne->type));
                                    auto masking = IRman->create_binary_operation(ne->type, GetPointer<gimple_assign>(GET_NODE(eb_ga))->op0, bit_mask_constant_node, srcp_default, truth_and_expr_K);
                                    TM->ReplaceTreeNode(stmt, ga->op1, masking); /// replaced with redundant code to restart lut_transformation
                                    INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---replace extract_bit_expr usage after: " + stmt->ToString());
                                    modified = true;
#ifndef NDEBUG
                                    AppM->RegisterTransformation(GetName(), stmt);
#endif
                                 }
                              }
                           }
                        }
                     };
                     nop_expr_BVO();
                  }
               };
               ga_BVO();
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement analyzed " + GET_NODE(stmt)->ToString());
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--BB analyzed " + STR(B_id));
   }
}

DesignFlowStep_Status Bit_Value_opt::InternalExec()
{
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, " --------- BIT_VALUE_OPT ----------");
   tree_managerRef TM = AppM->get_tree_manager();
   tree_manipulationRef IRman = tree_manipulationRef(new tree_manipulation(TM, parameters));

   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   // tree_nodeRef Scpe = TM->GetTreeReindex(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointer<statement_list>(GET_NODE(fd->body));
   THROW_ASSERT(sl, "Body is not a statement_list");
   /// for each basic block B in CFG do > Consider all blocks successively
   restart_dead_code = false;
   modified = false;
   optimize(sl, TM, IRman);
   modified ? function_behavior->UpdateBBVersion() : 0;
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

bool Bit_Value_opt::HasToBeExecuted() const
{
   return (bitvalue_version != function_behavior->GetBitValueVersion()) or FunctionFrontendFlowStep::HasToBeExecuted();
}

void Bit_Value_opt::Initialize()
{
}
