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
 * @file call_graph_computation.cpp
 * @brief Build call_graph data structure starting from the ir_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietro.fezzardi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "call_graph_computation.hpp"

#include "Parameter.hpp"
#include "SemiNCADominance.hpp"
#include "application_manager.hpp"
#include "basic_blocks_graph_constructor.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "ir_basic_block.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "string_manipulation.hpp"

call_graph_computation::call_graph_computation(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                               const DesignFlowManager& _design_flow_manager)
    : ApplicationFrontendFlowStep(_AppM, FUNCTION_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
call_graph_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CREATE_IR_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
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

DesignFlowStep_Status call_graph_computation::Exec()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Creating call graph data structure");
   const auto HLSMgr = GetPointer<HLS_manager>(AppM);
   const auto TM = AppM->get_ir_manager();
   auto& CGM = AppM->GetCallGraphManager();
   already_visited.clear();

   /// Root functions
   CustomSet<unsigned int> functions;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Top functions passed by user");
   auto function_symbols = parameters->getOption<std::vector<std::string>>(OPT_top_functions_names);
   /// checking if the top functions are present in the IR
   for(const auto& symbol : function_symbols)
   {
      const auto fnode = TM->GetFunction(symbol);
      if(!fnode)
      {
         THROW_ERROR("Function " + symbol + " not found in IR");
      }
   }

   for(const auto& [symbol, arch] : *HLSMgr->module_arch)
   {
      THROW_ASSERT(arch, "Expected function architecture for function " + symbol);
      const auto dataflow_attr = arch->attrs.find(FunctionArchitecture::func_dataflow_module);
      if(dataflow_attr != arch->attrs.end() && dataflow_attr->second == "1")
      {
         function_symbols.push_back(symbol);
      }
   }
   for(const auto& symbol : function_symbols)
   {
      const auto fnode = TM->GetFunction(symbol);
      if(fnode)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                        "---Root function " + STR(fnode->index) + " - " + symbol);
         functions.insert(fnode->index);
      }
   }
   CGM.SetRootFunctions(functions);

   // iterate on functions and add them to the call graph
   for(const auto f_id : functions)
   {
      const auto fnode = TM->GetIRNode(f_id);
      const auto fu_name = ir_helper::GetFunctionName(fnode);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---Adding function " + STR(f_id) + " " + fu_name + " to call graph");
      // avoid nested functions
      const auto fd = GetPointerS<const function_val_node>(fnode);
      if(fd->parent && fd->parent->get_kind() == function_val_node_K)
      {
         THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(f_id));
      }

      // add the function to the call graph if necessary
      if(!CGM.IsVertex(f_id))
      {
         const auto helper = std::make_shared<BehavioralHelper>(AppM, f_id, parameters);
         const auto FB = std::make_shared<FunctionBehavior>(AppM, helper, parameters);
         CGM.AddFunction(f_id, FB);
         CallGraphManager::expandCallGraphFromFunction(already_visited, AppM, f_id, debug_level);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Added function " + STR(f_id) + " " + fu_name + " to call graph");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---Function " + STR(f_id) + " " + fu_name + " was already in call graph");
      }
   }

   if(debug_level >= DEBUG_LEVEL_PEDANTIC || parameters->getOption<bool>(OPT_print_dot))
   {
      CGM.GetCallGraph().writeDot(parameters->getOption<std::filesystem::path>(OPT_dot_directory) / "call_graph.dot");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Created call graph");
   return DesignFlowStep_Status::SUCCESS;
}
