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
 * @file call_graph_computation.cpp
 * @brief Build call_graph data structure starting from the tree_manager.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietro.fezzardi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "call_graph_computation.hpp"

#include "Parameter.hpp"
#include "application_manager.hpp"
#include "behavioral_helper.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "dbgPrintHelper.hpp"
#include "ext_tree_node.hpp"
#include "function_behavior.hpp"
#include "hls_manager.hpp"
#include "string_manipulation.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

call_graph_computation::call_graph_computation(const ParameterConstRef _parameters, const application_managerRef _AppM,
                                               const DesignFlowManagerConstRef _design_flow_manager)
    : ApplicationFrontendFlowStep(_AppM, FUNCTION_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

call_graph_computation::~call_graph_computation() = default;

CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
call_graph_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_TASTE
         relationships.insert(std::make_pair(CREATE_ADDRESS_TRANSLATION, WHOLE_APPLICATION));
#endif
         relationships.insert(std::make_pair(HDL_FUNCTION_DECL_FIX, WHOLE_APPLICATION));
#if HAVE_FROM_PRAGMA_BUILT
         relationships.insert(std::make_pair(PRAGMA_ANALYSIS, WHOLE_APPLICATION));
#endif
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
   const auto TM = AppM->get_tree_manager();
   const auto CGM = AppM->GetCallGraphManager();
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
   CGM->SetRootFunctions(functions);

   // iterate on functions and add them to the call graph
   for(const auto f_id : functions)
   {
      const auto fu_name = tree_helper::name_function(TM, f_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "---Adding function " + STR(f_id) + " " + fu_name + " to call graph");
      if(fu_name == "__start_pragma__" || fu_name == "__close_pragma__" || fu_name.find("__pragma__") == 0)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Skipped...");
         continue;
      }
      // avoid nested functions
      const auto fun = TM->GetTreeNode(f_id);
      const auto fd = GetPointerS<const function_decl>(fun);
      if(fd->scpe && fd->scpe->get_kind() == function_decl_K)
      {
         THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(f_id));
      }

      // add the function to the call graph if necessary
      if(!CGM->IsVertex(f_id))
      {
         const auto has_body = TM->get_implementation_node(f_id) != 0;
         const auto helper = BehavioralHelperRef(new BehavioralHelper(AppM, f_id, has_body, parameters));
         const auto FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
         CGM->AddFunction(f_id, FB);
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
      CGM->CGetCallGraph()->WriteDot("call_graph.dot");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Created call graph");
   return DesignFlowStep_Status::SUCCESS;
}
