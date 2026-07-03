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
 *              Copyright (C) 2016-2026 Politecnico di Milano
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
 * @file predicate_statements.cpp
 * @brief This class contains the methods for setting predicates for instructions which require predication for control
 * motion
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "predicate_statements.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "ir_basic_block.hpp"
#include "ir_manager.hpp"
#include "ir_manipulation.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

PredicateStatements::PredicateStatements(const application_managerRef _AppM, unsigned int _function_id,
                                         const DesignFlowManager& _design_flow_manager,
                                         const ParameterConstRef _parameters)
    : FunctionFrontendFlowStep(_AppM, _function_id, FrontendFlowStepType::PREDICATE_STATEMENTS, _design_flow_manager,
                               _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionFrontendFlowStep::FunctionRelationship>>
PredicateStatements::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(FUNCTION_ANALYSIS, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
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

bool PredicateStatements::HasToBeExecuted() const
{
   return bb_version == 0 && FunctionFrontendFlowStep::HasToBeExecuted();
}

DesignFlowStep_Status PredicateStatements::InternalExec()
{
   const auto behavioral_helper = function_behavior->CGetBehavioralHelper();
   const auto TM = AppM->get_ir_manager();
   const auto ir_man = ir_manipulationRef(new ir_manipulation(TM, parameters, AppM));
   const auto boolean_type = ir_man->GetBooleanType();
   const auto true_value = TM->CreateUniqueIntegerCst(1, boolean_type);

   bool bb_modified = false;
   const auto fd = GetPointer<const function_val_node>(TM->GetIRNode(function_id));
   const auto sl = GetPointer<const statement_list_node>(fd->body);
   for(const auto& block : sl->list_of_bloc)
   {
      for(const auto& stmt : block.second->CGetStmtList())
      {
         const auto ga = GetPointer<assign_stmt>(stmt);
         const auto gc = GetPointer<call_stmt>(stmt);
         if(!behavioral_helper->RequiresPredicationForControlMotion(stmt->index) || (!ga && !gc))
         {
            continue;
         }

         const auto predicate_stmt = gc ? static_cast<node_stmt*>(gc) : static_cast<node_stmt*>(ga);
         THROW_ASSERT(predicate_stmt, "unexpected condition");
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Predicating " + STR(stmt));
         THROW_ASSERT(!predicate_stmt->predicate || predicate_stmt->predicate->index == true_value->index,
                      "unexpected condition");
         if(!predicate_stmt->predicate)
         {
            predicate_stmt->predicate = true_value;
            bb_modified = true;
         }
      }
   }

   bb_modified ? function_behavior->UpdateBBVersion() : 0;
   return bb_modified ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}
