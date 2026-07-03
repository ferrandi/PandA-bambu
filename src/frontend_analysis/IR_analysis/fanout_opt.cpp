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
 *              Copyright (C) 2017-2026 Politecnico di Milano
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
 * @file fanout_opt.cpp
 * @brief Fanout optimization step.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "fanout_opt.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "schedule.hpp"
#include "string_manipulation.hpp"
#include <cmath>
#include <fstream>
#include <string>

fanout_opt::fanout_opt(const ParameterConstRef _parameters, const application_managerRef _AppM,
                       unsigned int _function_id, const DesignFlowManager& _design_flow_manager)
    : FunctionFrontendFlowStep(_AppM, _function_id, FANOUT_OPT, _design_flow_manager, _parameters),
      TM(_AppM->get_ir_manager())
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
fanout_opt::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(INVALIDATION_RELATIONSHIP):
      {
         if(GetStatus() == DesignFlowStep_Status::SUCCESS)
         {
            relationships.insert(std::make_pair(BIT_VALUE, SAME_FUNCTION));
         }
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         break;
      }
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CSE_STEP, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_PATTERNS, SAME_FUNCTION));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

bool fanout_opt::is_dest_relevant(ir_nodeRef t, bool)
{
   if(t->get_kind() == assign_stmt_K)
   {
      auto* temp_assign = GetPointer<assign_stmt>(t);
      if(temp_assign->op1->get_kind() == mul_node_K || temp_assign->op1->get_kind() == widen_mul_node_K ||
         temp_assign->op1->get_kind() == ternary_add_node_K || temp_assign->op1->get_kind() == ternary_ss_node_K ||
         temp_assign->op1->get_kind() == ternary_as_node_K || temp_assign->op1->get_kind() == ternary_sa_node_K)
      {
         return true;
      }
   }
   return false;
}

DesignFlowStep_Status fanout_opt::InternalExec()
{
   if(parameters->IsParameter("disable-fanout_opt"))
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
   bool IR_changed = false;

   ir_nodeRef temp = TM->GetIRNode(function_id);
   auto* fd = GetPointer<function_val_node>(temp);
   auto* sl = GetPointer<statement_list_node>(fd->body);
   const ir_manipulationRef ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));

   for(const auto& block : sl->list_of_bloc)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining BB" + STR(block.first));
      for(const auto& stmt : block.second->CGetStmtList())
      {
         if(not AppM->ApplyNewTransformation())
         {
            break;
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Examining statement " + stmt->ToString());
         if(stmt->get_kind() == assign_stmt_K)
         {
            auto* ga = GetPointer<assign_stmt>(stmt);
            const std::string loc_info_default =
                ga->include_name + ":" + STR(ga->line_number) + ":" + STR(ga->column_number);
            if(ga->op0->get_kind() == ssa_node_K)
            {
               auto* ssa_defined = GetPointer<ssa_node>(ga->op0);
               if(ssa_defined->CGetNumberUses() > 1)
               {
                  const auto assigned_ssa_type_node = ir_helper::CGetType(ga->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                 "---the assigned ssa_node " + STR(ga->op0) + " has type " +
                                     STR(assigned_ssa_type_node));
                  bool is_first_stmt = true;
                  std::list<ir_nodeRef> list_of_dest_statements;
                  for(const auto& dest_statement : ssa_defined->CGetUseStmts())
                  {
                     if(is_first_stmt)
                     {
                        is_first_stmt = false;
                     }
                     else if(is_dest_relevant(dest_statement.first, false))
                     {
                        list_of_dest_statements.push_back(dest_statement.first);
                     }
                  }
                  for(const auto& dest_statement : list_of_dest_statements)
                  {
                     ir_nodeRef temp_assign =
                         ir_man->CreateAssignStmt(assigned_ssa_type_node, ssa_defined->min, ssa_defined->max, ga->op0,
                                                  function_id, loc_info_default);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---create a temporary assignment " + temp_assign->ToString());
                     block.second->PushAfter(temp_assign, stmt, AppM);
                     ir_nodeRef temp_ssa_var = GetPointer<assign_stmt>(temp_assign)->op0;
                     GetPointer<assign_stmt>(temp_assign)->keep = true;
                     GetPointer<assign_stmt>(temp_assign)->temporary_address = ga->temporary_address;
                     GetPointer<ssa_node>(temp_ssa_var)->SetDefStmt(temp_assign);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---dest statement before replacement " + dest_statement->ToString());
                     TM->ReplaceIRNode(dest_statement, ga->op0, temp_ssa_var);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                                    "---dest statement after replacement " + dest_statement->ToString());
                     IR_changed = true;
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Statement examined " + stmt->ToString());
      }
#if 1
      for(const auto& phi : block.second->CGetPhiList())
      {
         auto gp = GetPointer<phi_stmt>(phi);
         auto* ssa_defined = GetPointer<ssa_node>(gp->res);

         if(ssa_defined->CGetNumberUses() > 1)
         {
            bool is_first_stmt = true;
            std::list<ir_nodeRef> list_of_dest_statements;
            for(const auto& dest_statement : ssa_defined->CGetUseStmts())
            {
               if(is_first_stmt)
               {
                  is_first_stmt = false;
               }
               else if(is_dest_relevant(dest_statement.first, true))
               {
                  list_of_dest_statements.push_back(dest_statement.first);
               }
            }
            for(const auto& dest_statement : list_of_dest_statements)
            {
               /// Copy the list of def edges
               std::vector<std::pair<ir_nodeRef, unsigned int>> list_of_def_edge;
               for(const auto& def_edge : gp->CGetDefEdgesList())
               {
                  list_of_def_edge.push_back(std::pair<ir_nodeRef, unsigned int>(def_edge.first, def_edge.second));
               }
               ir_nodeRef new_res_var;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---starting from phi " + phi->ToString());
               auto new_phi = ir_man->create_phi_node(new_res_var, list_of_def_edge, gp->parent->index);
               auto new_res_var_ssa = GetPointer<ssa_node>(new_res_var);
               new_res_var_ssa->min = ssa_defined->min;
               new_res_var_ssa->max = ssa_defined->max;
               GetPointer<phi_stmt>(new_phi)->SetSSAUsesComputed();
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---created a new phi " + new_phi->ToString());
               block.second->AddPhi(new_phi);
               GetPointer<phi_stmt>(new_phi)->keep = true;
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---dest statement before replacement " + dest_statement->ToString());
               TM->ReplaceIRNode(dest_statement, gp->res, new_res_var);
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                              "---dest statement after replacement " + dest_statement->ToString());
               IR_changed = true;
            }
         }
      }
#endif
      if(IR_changed && schedule)
      {
         for(const auto& stmt : block.second->CGetStmtList())
         {
            schedule->UpdateTime(stmt->index);
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered BB" + STR(block.first));
   }
   IR_changed ? function_behavior->UpdateBBVersion() : 0;
   return IR_changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void fanout_opt::Initialize()
{
   if(GetPointer<const HLS_manager>(AppM) and GetPointer<const HLS_manager>(AppM)->get_HLS(function_id) and
      GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch)
   {
      schedule = GetPointer<const HLS_manager>(AppM)->get_HLS(function_id)->Rsch;
   }
}
