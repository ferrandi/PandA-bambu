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
 *              Copyright (C) 2004-2021 Politecnico di Milano
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
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "call_graph_computation.hpp"

/// Behavior include
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// Parameter include
#include "Parameter.hpp"

/// Tree include
#include "behavioral_helper.hpp"
#include "ext_tree_node.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

/// Utility include
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

call_graph_computation::call_graph_computation(const ParameterConstRef _parameters, const application_managerRef _AppM, const DesignFlowManagerConstRef _design_flow_manager)
    : ApplicationFrontendFlowStep(_AppM, FUNCTION_ANALYSIS, _design_flow_manager, _parameters)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

call_graph_computation::~call_graph_computation() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> call_graph_computation::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(DEPENDENCE_RELATIONSHIP):
      {
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_TREE_MANAGER, WHOLE_APPLICATION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
#if HAVE_FROM_PRAGMA_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(PRAGMA_ANALYSIS, WHOLE_APPLICATION));
#endif
#if HAVE_BAMBU_BUILT
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(HDL_FUNCTION_DECL_FIX, WHOLE_APPLICATION));
#endif
#if HAVE_TASTE
         relationships.insert(std::pair<FrontendFlowStepType, FunctionRelationship>(CREATE_ADDRESS_TRANSLATION, WHOLE_APPLICATION));
#endif
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
   const tree_managerRef TM = AppM->get_tree_manager();
   already_visited.clear();

   /// Root functions
   CustomOrderedSet<unsigned int> functions;
   const unsigned int main_index = TM->function_index("main");
   /// If top function option has been passed
   if(parameters->isOption(OPT_top_functions_names))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Top functions passed by user");
      const auto top_functions_names = parameters->getOption<const std::list<std::string>>(OPT_top_functions_names);
      for(const auto& top_function_name : top_functions_names)
      {
         const unsigned int top_function_index = TM->function_index(top_function_name);
         if(top_function_index == 0)
         {
            THROW_ERROR("Function " + top_function_name + " not found");
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Root function " + STR(top_function_index));
            functions.insert(top_function_index);
         }
      }
   }
   /// If not -c option has been passed we assume that whole program has been passed, so the main must be present
   else if(not parameters->getOption<bool>(OPT_gcc_c))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Expected main");
      /// Main not found
      if(not main_index)
      {
         THROW_ERROR("No main function found, but -c option not passed");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---main found");
         functions.insert(main_index);
      }
   }
   /// If there is the main, we return it
   else if(main_index)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---main was not expected but is present: " + STR(main_index));
      functions.insert(main_index);
   }
   /// Return all the functions not called by any other function
   else
   {
      functions.insert(TM->GetAllFunctions().begin(), TM->GetAllFunctions().end());
   }

   // iterate on functions and add them to the call graph
   for(const auto f_id : functions)
   {
      const std::string fu_name = tree_helper::name_function(TM, f_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Adding function " + STR(f_id) + " " + fu_name + " to call graph");
      if(fu_name == "__start_pragma__" or fu_name == "__close_pragma__" or boost::algorithm::starts_with(fu_name, "__pragma__"))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Skipped...");
         continue;
      }
      // avoid nested functions
      const tree_nodeRef fun = TM->get_tree_node_const(f_id);
      const auto* fd = GetPointer<const function_decl>(fun);
      if(fd->scpe and GET_NODE(fd->scpe)->get_kind() == function_decl_K)
      {
         THROW_ERROR_CODE(NESTED_FUNCTIONS_EC, "Nested functions not yet supported " + STR(f_id));
      }

      // add the function to the call graph if necessary
      if(not AppM->GetCallGraphManager()->IsVertex(f_id))
      {
         bool has_body = TM->get_implementation_node(f_id) != 0;
         BehavioralHelperRef helper = BehavioralHelperRef(new BehavioralHelper(AppM, f_id, has_body, parameters));
         FunctionBehaviorRef FB = FunctionBehaviorRef(new FunctionBehavior(AppM, helper, parameters));
         AppM->GetCallGraphManager()->AddFunction(f_id, FB);
         CallGraphManager::expandCallGraphFromFunction(already_visited, AppM, f_id, debug_level);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Added function " + STR(f_id) + " " + fu_name + " to call graph");
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Function " + STR(f_id) + " " + fu_name + " was already in call graph");
      }
   }

   if(debug_level >= DEBUG_LEVEL_PEDANTIC or parameters->getOption<bool>(OPT_print_dot))
   {
      AppM->GetCallGraphManager()->CGetCallGraph()->WriteDot("call_graph.dot");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Created call graph");
   return DesignFlowStep_Status::SUCCESS;
}
