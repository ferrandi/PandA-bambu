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
 *              Copyright (c) 2015-2020 Politecnico di Milano
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
 * @file omp_function_allocation.cpp
 * @brief extension of Class to allocate function in HLS based on context switch
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 *
 */

/// Header include
#include "omp_function_allocation_CS.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// boost include
#include <boost/range/adaptor/reversed.hpp>

/// HLS include
#include "hls_manager.hpp"

/// HLS/function_allocation include
#include "omp_functions.hpp"

/// STL include
#include <list>

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_manager.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"

OmpFunctionAllocationCS::OmpFunctionAllocationCS(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : fun_dominator_allocation(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION_CS)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

OmpFunctionAllocationCS::~OmpFunctionAllocationCS()
{
}

DesignFlowStep_Status OmpFunctionAllocationCS::Exec()
{
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   const auto TM = HLSMgr->get_tree_manager();
   fun_dominator_allocation::Exec();
   const auto call_graph_manager = HLSMgr->CGetCallGraphManager();
   auto root_functions = call_graph_manager->GetRootFunctions();
   if(parameters->isOption(OPT_top_design_name)) // top design function become the top_vertex
   {
      const auto top_rtldesign_function_id = HLSMgr->get_tree_manager()->function_index(parameters->getOption<std::string>(OPT_top_design_name));
      if(top_rtldesign_function_id != 0 and root_functions.find(top_rtldesign_function_id) != root_functions.end())
      {
         root_functions.clear();
         root_functions.insert(top_rtldesign_function_id);
      }
   }
   CustomUnorderedSet<vertex> vertex_subset;
   for(const auto f_id : root_functions)
      for(const auto reached_f_id : call_graph_manager->GetReachedBodyFunctionsFrom(f_id))
         vertex_subset.insert(call_graph_manager->GetVertex(reached_f_id));
   const auto call_graph = call_graph_manager->CGetCallSubGraph(vertex_subset);
   std::list<vertex> sorted_functions;
   call_graph->TopologicalSort(sorted_functions);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Computing functions to be parallelized");
   int cycleInd = 0;
   for(const auto function : sorted_functions)
   {
      bool function_classification_found = false;
      const auto function_id = call_graph_manager->get_function(function);
      std::cout << cycleInd << " " << HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name() << std::endl;
      if(HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetOmpForDegree()) // look for OMP function, add it to struct
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found omp for wrapper " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
         omp_functions->omp_for_wrappers.insert(function_id);
         ++cycleInd;
         continue;
      }
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(function, *call_graph); ie != ie_end && !function_classification_found; ie++)
      { // if current function is called by parallel then is kernel
         const auto source = boost::source(*ie, *call_graph);
         const auto source_id = call_graph_manager->get_function(source);
         if(omp_functions->omp_for_wrappers.find(source_id) != omp_functions->omp_for_wrappers.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found kernel function: " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
            omp_functions->kernel_functions.insert(function_id);
            function_classification_found = true;
            break;
         }
      }
      for(boost::tie(ie, ie_end) = boost::in_edges(function, *call_graph); ie != ie_end && !function_classification_found; ie++)
      { // if current function is called by kernel or a parallel function then is a function inside the kernel
         const auto source = boost::source(*ie, *call_graph);
         const auto source_id = call_graph_manager->get_function(source);
         if(omp_functions->kernel_functions.find(source_id) != omp_functions->kernel_functions.end() or omp_functions->parallelized_functions.find(source_id) != omp_functions->parallelized_functions.end())
         {
            if(HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->IsOmpFunctionAtomic()) // atomic function
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found atomic function: " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
               omp_functions->atomic_functions.insert(function_id);
            }
            else // is a normal function under the kernel
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found function to be parallelized: " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
               omp_functions->parallelized_functions.insert(function_id);
            }
            break;
         }
      }
      cycleInd = cycleInd + 1;
   }
   // invert list
   cycleInd = 0;
   for(const auto function : boost::adaptors::reverse(sorted_functions))
   {
      const auto function_id = call_graph_manager->get_function(function);
      std::cout << cycleInd << " " << HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name() << std::endl;
      OutEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::out_edges(function, *call_graph); ie != ie_end; ie++)
      { // if current function is called by parallel then is kernel
         const auto target = boost::target(*ie, *call_graph);
         const auto target_id = call_graph_manager->get_function(target);
         if(omp_functions->omp_for_wrappers.find(target_id) != omp_functions->omp_for_wrappers.end() or omp_functions->hierarchical_functions.find(target_id) != omp_functions->hierarchical_functions.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found hierarchical function: " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
            omp_functions->hierarchical_functions.insert(function_id);
            break;
         }
      }
      ++cycleInd;
   }
   return DesignFlowStep_Status::SUCCESS;
}

void OmpFunctionAllocationCS::Initialize()
{
   HLS_step::Initialize();
   HLSMgr->Rfuns = functionsRef(new OmpFunctions(HLSMgr));
}
