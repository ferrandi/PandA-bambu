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
 * @file rebuild_initialization2.cpp
 * @brief rebuild initialization where it is possible
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "rebuild_initialization2.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "basic_block.hpp"
#include "behavioral_helper.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "string_manipulation.hpp"
#include "token_interface.hpp"

#include <fstream>
#include <utility>
#include <vector>

#define REBUILD2_DEVEL 0
#if REBUILD2_DEVEL
#define unexpetedPattern(node) \
   THROW_ERROR("unexpected condition: " + node->get_kind_text() + " --- " + node->ToString());
#else
static bool unexpetedPattern(ir_nodeRef)
{
   return false;
}
#endif

#define foundNonConstant(VD)        \
   do                               \
   {                                \
      nonConstantVars.insert(VD);   \
      auto key = TM->GetIRNode(VD); \
      inits.erase(key);             \
   } while(0)

#if REBUILD2_DEVEL
#define unexpetedPattern2(node, VD) \
   THROW_ERROR("unexpected condition: " + node->get_kind_text() + " --- " + node->ToString());
#else
#define unexpetedPattern2(node, VD) foundNonConstant(VD)
#endif

static ir_nodeRef extractOp1(ir_nodeRef opSSA)
{
   if(opSSA->get_kind() == constant_int_val_node_K)
   {
      return ir_nodeRef();
   }
   THROW_ASSERT(opSSA->get_kind() == ssa_node_K, "unexpected condition:" + opSSA->ToString());
   auto* ssa_opSSA = GetPointerS<ssa_node>(opSSA);
   auto opSSA_def_stmt = ssa_opSSA->GetDefStmt();
   if(opSSA_def_stmt->get_kind() == nop_stmt_K || opSSA_def_stmt->get_kind() == phi_stmt_K)
   {
      return ir_nodeRef();
   }
   THROW_ASSERT(opSSA_def_stmt->get_kind() == assign_stmt_K,
                "unexpected condition: " + opSSA_def_stmt->get_kind_text());
   auto* opSSA_assign = GetPointerS<assign_stmt>(opSSA_def_stmt);
   return opSSA_assign->op1;
}

static bool varFound(ir_nodeRef node, unsigned& vd_index, ir_nodeRef& vd_node)
{
   THROW_ASSERT(node->get_kind() == addr_node_K, "unexpected condition");
   auto* ae = GetPointerS<addr_node>(node);
   auto ae_op = ae->op;
   if(ae_op->get_kind() == argument_val_node_K)
   {
      return false;
   }
   THROW_ASSERT(ae_op->get_kind() == variable_val_node_K, "unexpected condition: " + ae_op->get_kind_text());
   vd_index = ae->op->index;
   vd_node = ae->op;
   return true;
}

static ir_nodeRef getAssign(ir_nodeRef SSAop, unsigned vd_index, CustomOrderedSet<unsigned>& nonConstantVars,
                            IRNodeMap<std::map<integer_cst_t, ir_nodeRef>>& inits, ir_managerRef TM)
{
   THROW_ASSERT(SSAop->get_kind() == ssa_node_K, "unexpected condition");
   auto* ssa_var = GetPointerS<ssa_node>(SSAop);
   auto ssa_def_stmt = ssa_var->GetDefStmt();
   if(ssa_def_stmt->get_kind() == nop_stmt_K || ssa_def_stmt->get_kind() == phi_stmt_K)
   {
      nonConstantVars.insert(vd_index);
      auto key = TM->GetIRNode(vd_index);
      inits.erase(key);
      return ir_nodeRef();
   }
   else
   {
      THROW_ASSERT(ssa_def_stmt->get_kind() == assign_stmt_K, "unexpected condition: " + ssa_def_stmt->get_kind_text());
      auto* assign = GetPointerS<assign_stmt>(ssa_def_stmt);
      return assign->op1;
   }
}

rebuild_initialization2::rebuild_initialization2(const ParameterConstRef Param, const application_managerRef _AppM,
                                                 unsigned int _function_id,
                                                 const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, REBUILD_INITIALIZATION2, _design_flow_manager, Param)
{
   debug_level = Param->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
rebuild_initialization2::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

bool rebuild_initialization2::extract_var_decl_ppe(ir_nodeRef addr_assign_op1, unsigned& vd_index, ir_nodeRef& vd_node)
{
   auto* ppe = GetPointerS<gep_node>(addr_assign_op1);
   auto ppe_op0 = ppe->op0;
   auto addr2_assign_op1 = extractOp1(ppe_op0);
   if(!addr2_assign_op1)
   {
      return false;
   }
   if(addr2_assign_op1->get_kind() == bitcast_node_K || addr2_assign_op1->get_kind() == nop_node_K)
   {
      auto* ue = GetPointerS<unary_node>(addr2_assign_op1);
      auto ue_op = ue->op;
      auto addr3_assign_op1 = extractOp1(ue_op);
      if(!addr3_assign_op1)
      {
         return false;
      }
      if(addr3_assign_op1->get_kind() == addr_node_K)
      {
         return varFound(addr3_assign_op1, vd_index, vd_node);
      }
      else if(ppe->op1->get_kind() == constant_int_val_node_K && ir_helper::GetConstValue(ppe->op1) == 0)
      {
         if(addr3_assign_op1->get_kind() == ssa_node_K)
         {
            auto addr4_assign_op1 = extractOp1(addr3_assign_op1);
            if(!addr4_assign_op1)
            {
               return false;
            }
            if(addr4_assign_op1->get_kind() == gep_node_K)
            {
               addr_assign_op1 = addr4_assign_op1;
               return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
            }
            else if(addr4_assign_op1->get_kind() == addr_node_K)
            {
               return varFound(addr4_assign_op1, vd_index, vd_node);
            }
            else if(addr4_assign_op1->get_kind() == ssa_node_K)
            {
               auto addr5_assign_op1 = extractOp1(addr4_assign_op1);
               if(!addr5_assign_op1)
               {
                  return false;
               }
               if(addr5_assign_op1->get_kind() == gep_node_K)
               {
                  addr_assign_op1 = addr5_assign_op1;
                  return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
               }
               else
               {
                  return unexpetedPattern(addr5_assign_op1);
               }
            }
            else if(addr4_assign_op1->get_kind() == mem_access_node_K)
            {
               return false;
            }
            else
            {
               return unexpetedPattern(addr4_assign_op1);
            }
         }
         else if(addr3_assign_op1->get_kind() == gep_node_K)
         {
            addr_assign_op1 = addr3_assign_op1;
            return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
         }
         else
         {
            return unexpetedPattern(addr3_assign_op1);
         }
      }
      else if(addr3_assign_op1->get_kind() == ssa_node_K) /// starting from this condition offset is not anymore null
      {
         auto addr4_assign_op1 = extractOp1(addr3_assign_op1);
         if(!addr4_assign_op1)
         {
            return false;
         }
         if(addr4_assign_op1->get_kind() == addr_node_K)
         {
            return varFound(addr4_assign_op1, vd_index, vd_node);
         }
         else if(addr4_assign_op1->get_kind() == ssa_node_K)
         {
            auto addr5_assign_op1 = extractOp1(addr4_assign_op1);
            if(!addr5_assign_op1)
            {
               return false;
            }
            return unexpetedPattern(addr5_assign_op1);
         }
         else if(addr4_assign_op1->get_kind() == nop_node_K)
         {
            auto* ne1 = GetPointerS<nop_node>(addr4_assign_op1);
            auto ne1_op = ne1->op;
            auto addr5_assign_op1 = extractOp1(ne1_op);
            if(!addr5_assign_op1)
            {
               return false;
            }
            return unexpetedPattern(addr5_assign_op1);
         }
         else if(addr4_assign_op1->get_kind() == gep_node_K)
         {
            return false;
         }
         else
         {
            return unexpetedPattern(addr4_assign_op1);
         }
      }
      else if(addr3_assign_op1->get_kind() == bitcast_node_K)
      {
         auto* ue1 = GetPointerS<unary_node>(addr3_assign_op1);
         auto ue1_op = ue1->op;
         if(ue1_op->get_kind() == ssa_node_K)
         {
            auto addr4_assign_op1 = extractOp1(ue1_op);
            if(!addr4_assign_op1)
            {
               return false;
            }
            if(addr4_assign_op1->get_kind() == addr_node_K)
            {
               return varFound(addr4_assign_op1, vd_index, vd_node);
            }
            else if(addr4_assign_op1->get_kind() == gep_node_K)
            {
               return false;
            }
            else
            {
               return unexpetedPattern(addr4_assign_op1);
            }
         }
         else
         {
            return unexpetedPattern(ue1_op);
         }
      }
      else if(addr3_assign_op1->get_kind() == gep_node_K)
      {
         return false;
      }
      else if(addr3_assign_op1->get_kind() == add_node_K)
      {
         return false;
      }
      else if(addr3_assign_op1->get_kind() == call_node_K)
      {
         return false;
      }
      else
      {
         return unexpetedPattern(addr3_assign_op1);
      }
   }
   else if(addr2_assign_op1->get_kind() == addr_node_K)
   {
      return varFound(addr2_assign_op1, vd_index, vd_node);
   }
   else if(addr2_assign_op1->get_kind() == ssa_node_K)
   {
      auto addr3_assign_op1 = extractOp1(addr2_assign_op1);
      if(!addr3_assign_op1)
      {
         return false;
      }
      if(addr3_assign_op1->get_kind() == gep_node_K)
      {
         if(ppe->op1->get_kind() == constant_int_val_node_K && ir_helper::GetConstValue(ppe->op1) == 0)
         {
            addr_assign_op1 = addr3_assign_op1;
            return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
         }
         else
         {
            return false;
         }
      }
      else if(addr3_assign_op1->get_kind() == addr_node_K)
      {
         return varFound(addr3_assign_op1, vd_index, vd_node);
      }
      else
      {
         return unexpetedPattern(addr3_assign_op1);
      }
   }
   else if(addr2_assign_op1->get_kind() == gep_node_K)
   {
      return false;
   }
   else if(addr2_assign_op1->get_kind() == mem_access_node_K)
   {
      return false;
   }
   else if(addr2_assign_op1->get_kind() == call_node_K)
   {
      return false;
   }
   else if(addr2_assign_op1->get_kind() == select_node_K)
   {
      return false;
   }
   else
   {
      return unexpetedPattern(addr2_assign_op1);
   }
}

bool rebuild_initialization2::extract_var_decl(const mem_access_node* me, unsigned& vd_index, ir_nodeRef& vd_node,
                                               ir_nodeRef& addr_assign_op1)
{
   auto me_op0 = me->op;
   addr_assign_op1 = extractOp1(me_op0);
   if(!addr_assign_op1)
   {
      return false;
   }
   if(addr_assign_op1->get_kind() == gep_node_K)
   {
      return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
   }
   else if(addr_assign_op1->get_kind() == ssa_node_K)
   {
      auto addr2_assign_op1 = extractOp1(addr_assign_op1);
      if(!addr2_assign_op1)
      {
         return false;
      }
      if(addr2_assign_op1->get_kind() == nop_node_K)
      {
         auto* ne = GetPointerS<nop_node>(addr2_assign_op1);
         auto ne_op = ne->op;
         auto addr3_assign_op1 = extractOp1(ne_op);
         if(!addr3_assign_op1)
         {
            return false;
         }
         return unexpetedPattern(addr3_assign_op1);
      }
      else if(addr2_assign_op1->get_kind() == gep_node_K)
      {
         addr_assign_op1 = addr2_assign_op1;
         return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
      }
      else if(addr2_assign_op1->get_kind() == addr_node_K)
      {
         return varFound(addr2_assign_op1, vd_index, vd_node);
      }
      else if(addr2_assign_op1->get_kind() == mem_access_node_K)
      {
         return false;
      }
      else
      {
         return unexpetedPattern(addr2_assign_op1);
      }
   }
   else if(addr_assign_op1->get_kind() == addr_node_K)
   {
      return varFound(addr_assign_op1, vd_index, vd_node);
   }
   else if(addr_assign_op1->get_kind() == bitcast_node_K || addr_assign_op1->get_kind() == nop_node_K)
   {
      auto* ue = GetPointerS<unary_node>(addr_assign_op1);
      auto ue_op = ue->op;
      auto addr1_assign_op1 = extractOp1(ue_op);
      if(!addr1_assign_op1)
      {
         return false;
      }
      if(addr1_assign_op1->get_kind() == addr_node_K)
      {
         return varFound(addr1_assign_op1, vd_index, vd_node);
      }
      else if(addr1_assign_op1->get_kind() == ssa_node_K)
      {
         auto addr2_assign_op1 = extractOp1(addr1_assign_op1);
         if(!addr2_assign_op1)
         {
            return false;
         }
         if(addr2_assign_op1->get_kind() == gep_node_K)
         {
            addr_assign_op1 = addr2_assign_op1;
            return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
         }
         return unexpetedPattern(addr2_assign_op1);
      }
      else if(addr1_assign_op1->get_kind() == nop_node_K)
      {
         auto* ne1 = GetPointerS<unary_node>(addr1_assign_op1);
         auto ne1_op = ne1->op;
         auto addr2_assign_op1 = extractOp1(ne1_op);
         if(!addr2_assign_op1)
         {
            return false;
         }
         if(addr2_assign_op1->get_kind() == ssa_node_K)
         {
            auto addr3_assign_op1 = extractOp1(ne1_op);
            if(!addr3_assign_op1)
            {
               return false;
            }
            if(addr3_assign_op1->get_kind() == gep_node_K)
            {
               addr_assign_op1 = addr3_assign_op1;
               return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
            }
            else
            {
               return unexpetedPattern(addr3_assign_op1);
            }
         }
         else
         {
            return unexpetedPattern(addr2_assign_op1);
         }
      }
      else if(addr1_assign_op1->get_kind() == gep_node_K)
      {
         addr_assign_op1 = addr1_assign_op1;
         return extract_var_decl_ppe(addr_assign_op1, vd_index, vd_node);
      }
      else
      {
         return unexpetedPattern(addr1_assign_op1);
      }
   }
   else if(addr_assign_op1->get_kind() == mem_access_node_K)
   {
      return false;
   }
   else if(addr_assign_op1->get_kind() == call_node_K)
   {
      return false;
   }
   else if(addr_assign_op1->get_kind() == select_node_K)
   {
      return false;
   }
   else
   {
      return unexpetedPattern(addr_assign_op1);
   }
}

bool rebuild_initialization2::look_for_ROMs()
{
   ir_managerRef TM = AppM->get_ir_manager();
   ir_manipulationRef ir_man(new ir_manipulation(TM, parameters, AppM));
   ir_nodeRef tn = TM->GetIRNode(function_id);
   auto* fd = GetPointerS<function_val_node>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");
   auto* sl = GetPointerS<statement_list_node>(fd->body);
   THROW_ASSERT(sl, "Body is not a statement_list_node");
   bool not_supported = false;
   std::map<unsigned, unsigned> var_writing_BB_relation;
   std::map<unsigned, unsigned long long> var_writing_size_relation;
   std::map<unsigned, unsigned long long> var_writing_elts_size_relation;
   CustomOrderedSet<unsigned> nonConstantVars;
   IRNodeMap<std::map<integer_cst_t, ir_nodeRef>> inits;

   /// for each basic block B in CFG compute constantVars candidates
   for(const auto& Bit : sl->list_of_bloc)
   {
      // used to collect the reads of a given variable done in the same basic block.
      // This is done to avoid to classify a variable constant in case is first
      // written the read and then written again.
      CustomOrderedSet<unsigned> VarsReadSeen;
      auto B = Bit.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining for write BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      for(const auto& inst : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + inst->ToString());
         auto gn = GetPointerS<node_stmt>(inst);
         auto stmt_kind = inst->get_kind();
         if(gn->vdef && stmt_kind != assign_stmt_K)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: pattern not supported");
            not_supported = true;
            break;
         }
         if(stmt_kind == assign_stmt_K && gn->vdef)
         {
            auto ga = GetPointerS<assign_stmt>(inst);
            auto op0 = ga->op0;
            auto op1 = ga->op1;
            if(op0->get_kind() == mem_access_node_K)
            {
               unsigned vd_index = 0;
               ir_nodeRef vd_node;
               ir_nodeRef addr_assign_op1;
               auto* me = GetPointerS<mem_access_node>(op0);
               auto resolved = extract_var_decl(me, vd_index, vd_node, addr_assign_op1);
               if(resolved && nonConstantVars.find(vd_index) == nonConstantVars.end())
               {
                  THROW_ASSERT(vd_index && vd_node, "unexpected condition");
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---variable written: " + TM->GetIRNode(vd_index)->ToString());
                  /// are we writing a constant value
                  if(!GetPointer<cst_node>(op1))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---variable is not constant(1): " + TM->GetIRNode(vd_index)->ToString());
                     foundNonConstant(vd_index);
                  }
                  if(nonConstantVars.find(vd_index) == nonConstantVars.end())
                  {
                     if(VarsReadSeen.find(vd_index) != VarsReadSeen.end())
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(2): " + TM->GetIRNode(vd_index)->ToString());
                        foundNonConstant(vd_index);
                     }
                     else if(var_writing_BB_relation.find(vd_index) == var_writing_BB_relation.end())
                     {
                        /// first check if the variable is initialized
                        auto* vd = GetPointerS<variable_val_node>(vd_node);
                        if(vd->init)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---variable is initialized: " + TM->GetIRNode(vd_index)->ToString());
                           foundNonConstant(vd_index);
                        }
                        else if(not vd->parent or vd->parent->get_kind() == module_unit_node_K)
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                          "---variable is not local: " + TM->GetIRNode(vd_index)->ToString());
                           foundNonConstant(vd_index);
                        }
                        else
                        {
                           auto Type = vd->type;
                           /// then we check if the variable is an array
                           if(Type->get_kind() == array_ty_node_K)
                           {
                              std::vector<unsigned long long> dims;
                              unsigned long long elts_size;
                              auto type_index = ir_helper::CGetType(vd_node)->index;
                              ir_helper::get_array_dim_and_bitsize(TM, type_index, dims, elts_size);
                              if(dims.size() == 1)
                              {
                                 /// then in case we are fine we classify as good candidate for being a constant var
                                 var_writing_BB_relation[vd_index] = B->number;
                                 var_writing_size_relation[vd_index] = dims[0];
                                 var_writing_elts_size_relation[vd_index] = elts_size;
                              }
                              else
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---variable is not constant(3): " +
                                                    TM->GetIRNode(vd_index)->ToString());
                                 foundNonConstant(vd_index);
                              }
                           }
                           else
                           {
                              INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                             "---variable is currently classified as non-constant: " +
                                                 TM->GetIRNode(vd_index)->ToString());
                              foundNonConstant(vd_index);
                           }
                        }
                     }
                     else if(var_writing_BB_relation.find(vd_index)->second != B->number)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(4): " + TM->GetIRNode(vd_index)->ToString());
                        foundNonConstant(vd_index);
                     }
                     /// if it is still a good candidate
                     if(nonConstantVars.find(vd_index) == nonConstantVars.end())
                     {
                        /// check if the offset is constant
                        if(addr_assign_op1->get_kind() == gep_node_K)
                        {
                           auto* ppe = GetPointerS<gep_node>(addr_assign_op1);
                           auto ppe_op1 = ppe->op1;
                           if(ppe_op1->get_kind() == ssa_node_K)
                           {
                              auto offset_assign_op1 = getAssign(ppe_op1, vd_index, nonConstantVars, inits, TM);
                              if(!offset_assign_op1)
                              {
                                 INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                "---variable is not constant(9): " +
                                                    TM->GetIRNode(vd_index)->ToString());
                              }
                              else
                              {
                                 if(offset_assign_op1->get_kind() == shl_node_K)
                                 {
                                    auto* ls = GetPointerS<shl_node>(offset_assign_op1);
                                    auto ls_op1 = ls->op1;
                                    if(ls_op1->get_kind() == constant_int_val_node_K)
                                    {
                                       THROW_ASSERT(ir_helper::GetConstValue(ls->op1) >= 0, "");
                                       auto nbit = static_cast<unsigned long long>(ir_helper::GetConstValue(ls->op1));
                                       THROW_ASSERT(nbit < 32, "unexpected condition");
                                       std::vector<unsigned long long> dims;
                                       THROW_ASSERT(var_writing_elts_size_relation.find(vd_index) !=
                                                        var_writing_elts_size_relation.end(),
                                                    "unexpected condition");
                                       auto elts_size = var_writing_elts_size_relation[vd_index];
                                       if(elts_size != (8ULL << nbit))
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---variable is not constant(9c): " +
                                                             TM->GetIRNode(vd_index)->ToString());
                                          foundNonConstant(vd_index);
                                       }
                                       else
                                       {
                                          auto ls_op0 = ls->op0;
                                          if(ls_op0->get_kind() == ssa_node_K)
                                          {
                                             auto nop_assign_op1 =
                                                 getAssign(ls_op0, vd_index, nonConstantVars, inits, TM);
                                             if(!nop_assign_op1)
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---variable is not constant(9): " +
                                                                   TM->GetIRNode(vd_index)->ToString());
                                             }
                                             else
                                             {
                                                if(nop_assign_op1->get_kind() == nop_node_K)
                                                {
                                                   auto* ne = GetPointerS<nop_node>(nop_assign_op1);
                                                   auto ne_op = ne->op;
                                                   if(ne_op->get_kind() == constant_int_val_node_K)
                                                   {
                                                      /// index is constant
                                                      inits[vd_node][ir_helper::GetConstValue(ne->op)] = ga->op1;
                                                   }
                                                   else
                                                   {
                                                      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                     "---variable is not constant(5a): " +
                                                                         TM->GetIRNode(vd_index)->ToString());
                                                      foundNonConstant(vd_index);
                                                   }
                                                }
                                                else if(nop_assign_op1->get_kind() == constant_int_val_node_K)
                                                {
                                                   /// index is constant
                                                   inits[vd_node][ir_helper::GetConstValue(nop_assign_op1)] = ga->op1;
                                                }
                                                else if(nop_assign_op1->get_kind() != bitcast_node_K)
                                                {
                                                   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                                  "---variable is not constant(5b): " +
                                                                      TM->GetIRNode(vd_index)->ToString());
                                                   foundNonConstant(vd_index);
                                                }
                                                else
                                                {
                                                   unexpetedPattern2(nop_assign_op1, vd_index);
                                                }
                                             }
                                          }
                                          else
                                          {
                                             unexpetedPattern2(ls_op0, vd_index);
                                          }
                                       }
                                    }
                                    else
                                    {
                                       unexpetedPattern2(ls_op1, vd_index);
                                    }
                                 }
                                 else if(offset_assign_op1->get_kind() == ssa_node_K)
                                 {
                                    auto offset_assign1_op1 =
                                        getAssign(offset_assign_op1, vd_index, nonConstantVars, inits, TM);
                                    if(!offset_assign1_op1)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                      "---variable is not constant(9): " +
                                                          TM->GetIRNode(vd_index)->ToString());
                                    }
                                    else
                                    {
                                       if(offset_assign1_op1->get_kind() == nop_node_K)
                                       {
                                          auto* ne = GetPointerS<nop_node>(offset_assign1_op1);
                                          auto ne_op = ne->op;
                                          if(ne_op->get_kind() == constant_int_val_node_K)
                                          {
                                             inits[vd_node][ir_helper::GetConstValue(ne->op)] = ga->op1;
                                          }
                                          else if(ne_op->get_kind() == ssa_node_K)
                                          {
                                             auto offset_assign2_op1 =
                                                 getAssign(ne_op, vd_index, nonConstantVars, inits, TM);
                                             if(!offset_assign2_op1)
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---variable is not constant(5c): " +
                                                                   TM->GetIRNode(vd_index)->ToString());
                                             }
                                             else if(offset_assign2_op1->get_kind() == constant_int_val_node_K)
                                             {
                                                inits[vd_node][ir_helper::GetConstValue(offset_assign2_op1)] = ga->op1;
                                             }
                                             else
                                             {
                                                INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                               "---variable is not constant(5c): " +
                                                                   TM->GetIRNode(vd_index)->ToString());
                                                foundNonConstant(vd_index);
                                             }
                                          }
                                          else
                                          {
                                             unexpetedPattern2(ne_op, vd_index);
                                          }
                                       }
                                       else
                                       {
                                          unexpetedPattern2(offset_assign1_op1, vd_index);
                                       }
                                    }
                                 }
                                 else if(offset_assign_op1->get_kind() == nop_node_K)
                                 {
                                    auto* ne = GetPointerS<nop_node>(offset_assign_op1);
                                    auto ne_op = ne->op;
                                    if(ne_op->get_kind() == ssa_node_K)
                                    {
                                       auto offset_assign3_op1 = getAssign(ne_op, vd_index, nonConstantVars, inits, TM);
                                       if(!offset_assign3_op1)
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                                         "---variable is not constant(9): " +
                                                             TM->GetIRNode(vd_index)->ToString());
                                       }
                                       else
                                       {
                                          unexpetedPattern2(offset_assign3_op1, vd_index);
                                       }
                                    }
                                    else
                                    {
                                       unexpetedPattern2(ne_op, vd_index);
                                    }
                                 }
                                 else
                                 {
                                    unexpetedPattern2(offset_assign_op1, vd_index);
                                 }
                              }
                           }
                           else if(ppe_op1->get_kind() == constant_int_val_node_K)
                           {
                              THROW_ASSERT(var_writing_elts_size_relation.find(vd_index) !=
                                               var_writing_elts_size_relation.end(),
                                           "unexpected condition");
                              inits[vd_node]
                                   [ir_helper::GetConstValue(ppe_op1) /
                                    (static_cast<integer_cst_t>(var_writing_elts_size_relation[vd_index]) / 8)] =
                                       ga->op1;
                           }
                           else
                           {
                              unexpetedPattern2(ppe_op1, vd_index);
                           }
                        }
                        else if(addr_assign_op1->get_kind() == ssa_node_K)
                        {
                           inits[vd_node][0] = ga->op1;
                        }
                        else if(addr_assign_op1->get_kind() == addr_node_K)
                        {
                           inits[vd_node][0] = ga->op1;
                        }
                        else
                        {
                           unexpetedPattern2(addr_assign_op1, vd_index);
                        }
                     }
                  }
                  /// else do nothing
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Examined statement: pattern not supported");
                  not_supported = true;
                  break;
               }
            }
            else if(op0->get_kind() == ssa_node_K && op1->get_kind() == call_node_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: IPA is required");
               not_supported = true;
               break;
            }
            else if(op0->get_kind() == ssa_node_K && op1->get_kind() == mem_access_node_K)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement: Not supported");
               not_supported = true;
               break;
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "<--Examined statement: Not supported" + op0->get_kind_text());
               not_supported = true;
               break;
            }
         }
         else if(stmt_kind == assign_stmt_K && !gn->vuses.empty())
         {
            auto ga = GetPointerS<assign_stmt>(inst);
            auto op1 = ga->op1;
            if(op1->get_kind() == mem_access_node_K)
            {
               unsigned vd_index = 0;
               ir_nodeRef vd_node;
               ir_nodeRef dummy_var;
               auto* me = GetPointerS<mem_access_node>(op1);
               auto resolved = extract_var_decl(me, vd_index, vd_node, dummy_var);
               if(resolved)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---variable read: " + TM->GetIRNode(vd_index)->ToString());
                  VarsReadSeen.insert(vd_index);
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "<--Examined statement: pattern not supported");
                  not_supported = true;
                  break;
               }
            }
            else
            {
               for(auto var_written : var_writing_BB_relation)
               {
                  if(var_written.second == B->number)
                  {
                     VarsReadSeen.insert(var_written.first);
                  }
               }
            }
         }
         else if(!gn->vuses.empty())
         {
            for(auto var_written : var_writing_BB_relation)
            {
               if(var_written.second == B->number)
               {
                  VarsReadSeen.insert(var_written.first);
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined for write BB" + STR(B->number));
      if(not_supported)
      {
         break;
      }
   }
   if(not_supported || var_writing_BB_relation.empty() || var_writing_BB_relation.size() == nonConstantVars.size())
   {
      return false;
   }

   /// compute the CFG
   BBGraphsCollection ir_bb_graphs_collection(BBGraphInfo(AppM, function_id));
   BBGraph ir_bb_graph(ir_bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, BBGraph::vertex_descriptor> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map[block.first] = ir_bb_graphs_collection.AddVertex(BBNodeInfo(block.second));
   }
   /// add edges
   for(const auto& curr_bb_pair : sl->list_of_bloc)
   {
      auto curr_bb = curr_bb_pair.first;
      auto lop_it_end = sl->list_of_bloc[curr_bb]->list_of_pred.end();
      for(auto lop_it = sl->list_of_bloc[curr_bb]->list_of_pred.begin(); lop_it != lop_it_end; ++lop_it)
      {
         THROW_ASSERT(inverse_vertex_map.find(*lop_it) != inverse_vertex_map.end(),
                      "BB" + STR(*lop_it) + " (successor of BB" + STR(curr_bb) + ") does not exist");
         ir_bb_graphs_collection.AddEdge(inverse_vertex_map[*lop_it], inverse_vertex_map[curr_bb], CFG_SELECTOR);
      }
      auto los_it_end = sl->list_of_bloc[curr_bb]->list_of_succ.end();
      for(auto los_it = sl->list_of_bloc[curr_bb]->list_of_succ.begin(); los_it != los_it_end; ++los_it)
      {
         if(*los_it == bloc::EXIT_BLOCK_ID)
         {
            ir_bb_graphs_collection.AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[*los_it], CFG_SELECTOR);
         }
      }
      if(sl->list_of_bloc[curr_bb]->list_of_succ.empty())
      {
         ir_bb_graphs_collection.AddEdge(inverse_vertex_map[curr_bb], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                         CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   ir_bb_graphs_collection.AddEdge(inverse_vertex_map[bloc::ENTRY_BLOCK_ID], inverse_vertex_map[bloc::EXIT_BLOCK_ID],
                                   CFG_SELECTOR);

   /// check if reads are consistent with writes: writes are always dominating the following reads
   for(const auto& Bit : sl->list_of_bloc)
   {
      auto B = Bit.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining for write BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      for(const auto& inst : list_of_stmt)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + inst->ToString());
         auto gn = GetPointerS<node_stmt>(inst);
         auto stmt_kind = inst->get_kind();
         if(stmt_kind == assign_stmt_K && !gn->vuses.empty())
         {
            auto ga = GetPointerS<assign_stmt>(inst);
            auto op1 = ga->op1;
            if(op1->get_kind() == mem_access_node_K)
            {
               unsigned vd_index = 0;
               ir_nodeRef vd_node;
               ir_nodeRef dummy_var;
               auto* me = GetPointerS<mem_access_node>(op1);
               auto resolved = extract_var_decl(me, vd_index, vd_node, dummy_var);
               if(resolved && nonConstantVars.find(vd_index) == nonConstantVars.end())
               {
                  if(var_writing_BB_relation.find(vd_index) != var_writing_BB_relation.end())
                  {
                     auto BB_written = var_writing_BB_relation.find(vd_index)->second;
                     if(ir_bb_graph.IsReachable(inverse_vertex_map[BB_written], inverse_vertex_map[B->number]))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(6): " + TM->GetIRNode(vd_index)->ToString());
                        foundNonConstant(vd_index);
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(resolved, "unexpected condition");
               }
            }
            else
            {
               for(auto var_written : var_writing_BB_relation)
               {
                  if(nonConstantVars.find(var_written.first) == nonConstantVars.end())
                  {
                     auto BB_written = var_written.second;
                     if(ir_bb_graph.IsReachable(inverse_vertex_map[BB_written], inverse_vertex_map[B->number]))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                       "---variable is not constant(7): " +
                                           TM->GetIRNode(var_written.first)->ToString());
                        foundNonConstant(var_written.first);
                     }
                  }
               }
            }
         }
         else if(!gn->vuses.empty())
         {
            for(auto var_written : var_writing_BB_relation)
            {
               if(nonConstantVars.find(var_written.first) == nonConstantVars.end())
               {
                  auto BB_written = var_written.second;
                  if(ir_bb_graph.IsReachable(inverse_vertex_map[BB_written], inverse_vertex_map[B->number]))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---variable is not constant(8): " + TM->GetIRNode(var_written.first)->ToString());
                     foundNonConstant(var_written.first);
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined for write BB" + STR(B->number));
   }

   /// list all constant variables found
   CustomOrderedSet<unsigned> ConstantVars;
   for(auto vars : var_writing_BB_relation)
   {
      if(nonConstantVars.find(vars.first) == nonConstantVars.end())
      {
         auto key = TM->GetIRNode(vars.first);
         auto initIt = inits.find(key);
         THROW_ASSERT(initIt != inits.end(), "unexpected condition");
         THROW_ASSERT(var_writing_size_relation.find(vars.first) != var_writing_size_relation.end(),
                      "unexpected condition");
         if(initIt->second.size() == var_writing_size_relation.find(vars.first)->second)
         {
            auto vd_node = TM->GetIRNode(vars.first);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Constant variable: " + vd_node->ToString());
            ConstantVars.insert(vars.first);
            GetPointerS<variable_val_node>(vd_node)->readonly_flag = true;
         }
         else
         {
            inits.erase(initIt);
         }
      }
   }
   if(ConstantVars.empty())
   {
      return false;
   }

   for(const auto& Bit : sl->list_of_bloc)
   {
      auto B = Bit.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining for write BB" + STR(B->number));
      const auto& list_of_stmt = B->CGetStmtList();
      auto it_los_end = list_of_stmt.end();
      auto it_los = list_of_stmt.begin();
      while(it_los != it_los_end)
      {
         auto& inst = *it_los;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + inst->ToString());
         auto gn = GetPointerS<node_stmt>(inst);
         auto stmt_kind = inst->get_kind();
         if(stmt_kind == assign_stmt_K && gn->vdef)
         {
            auto ga = GetPointerS<assign_stmt>(inst);
            auto op0 = ga->op0;
            if(op0->get_kind() == mem_access_node_K)
            {
               unsigned vd_index = 0;
               ir_nodeRef vd_node;
               ir_nodeRef dummy_var;
               auto* me = GetPointerS<mem_access_node>(op0);
               auto resolved = extract_var_decl(me, vd_index, vd_node, dummy_var);
               if(resolved && ConstantVars.find(vd_index) != ConstantVars.end())
               {
                  if(ga->memdef)
                  {
                     ir_manager::IRSchema nop_stmt_schema;
                     nop_stmt_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
                     nop_stmt_schema[TOK(TOK_PARENT)] = STR(function_id);
                     GetPointerS<ssa_node>(ga->memdef)->SetDefStmt(TM->create_ir_node(nop_stmt_K, nop_stmt_schema));
                  }
                  if(ga->vdef)
                  {
                     ir_manager::IRSchema nop_stmt_schema;
                     nop_stmt_schema[TOK(TOK_IR_LOCINFO)] = BUILTIN_LOCINFO;
                     nop_stmt_schema[TOK(TOK_PARENT)] = STR(function_id);
                     GetPointerS<ssa_node>(ga->vdef)->SetDefStmt(TM->create_ir_node(nop_stmt_K, nop_stmt_schema));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Removing " + STR(*it_los));
                  B->RemoveStmt(*it_los, AppM);
                  it_los = list_of_stmt.begin();
                  it_los_end = list_of_stmt.end();
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
                  continue;
               }
            }
         }
         ++it_los;
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined statement");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Examined for write BB" + STR(B->number));
   }
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto integer_ty_node = ir_man->GetSignedIntegerType();
   for(const auto& init : inits)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Rebuilding init of " + STR(init.first));
      ir_manager::IRSchema constructor_ir_node_schema;
      const auto array_ty_node = ir_helper::CGetType(init.first);
      constructor_ir_node_schema[TOK(TOK_TYPE)] = STR(array_ty_node->index);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Type is " + STR(array_ty_node));
      const auto element_type = ir_helper::CGetElements(array_ty_node);
      auto ctor_node = TM->create_ir_node(constructor_node_K, constructor_ir_node_schema);
      auto* constr = GetPointerS<constructor_node>(ctor_node);
      const auto last_index = init.second.rbegin()->first;
      integer_cst_t index = 0;
      for(index = 0; index <= last_index; index++)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + STR(index));
         if(init.second.count(index))
         {
            constr->add_idx_valu(TM->CreateUniqueIntegerCst(index, integer_ty_node), init.second.at(index));
         }
         else
         {
            THROW_ASSERT(element_type->get_kind() == integer_ty_node_K, "Type not supported " + STR(element_type));
            const auto default_value = TM->CreateUniqueIntegerCst(0, element_type);
            constr->add_idx_valu(TM->CreateUniqueIntegerCst(index, integer_ty_node), default_value);
         }
      }
      GetPointerS<variable_val_node>(init.first)->init = ctor_node;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Rebuilt init of " + STR(init.first));
   }

   return true;
}

DesignFlowStep_Status rebuild_initialization2::InternalExec()
{
   bool modified = look_for_ROMs();
   if(modified)
   {
      function_behavior->UpdateBBVersion();
   }
   return modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
