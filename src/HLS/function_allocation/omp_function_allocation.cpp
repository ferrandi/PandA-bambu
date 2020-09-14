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
 * @brief Class to allocate function in HLS based on dominators and openmp information
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "omp_function_allocation.hpp"

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

/// tree includes
#include "behavioral_helper.hpp"
#include "tree_manager.hpp"

/// utility include
#include "dbgPrintHelper.hpp"
#include "utility.hpp"

OmpFunctionAllocation::OmpFunctionAllocation(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager)
    : fun_dominator_allocation(_parameters, _HLSMgr, _design_flow_manager, HLSFlowStep_Type::OMP_FUNCTION_ALLOCATION)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

OmpFunctionAllocation::~OmpFunctionAllocation()
{
}

DesignFlowStep_Status OmpFunctionAllocation::Exec()
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
   for(const auto function : sorted_functions)
   {
      const auto function_id = call_graph_manager->get_function(function);
      if(HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->GetOmpForDegree())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found omp for wrapper " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
         omp_functions->omp_for_wrappers.insert(function_id);
         continue;
      }
      InEdgeIterator ie, ie_end;
      for(boost::tie(ie, ie_end) = boost::in_edges(function, *call_graph); ie != ie_end; ie++)
      {
         const auto source = boost::source(*ie, *call_graph);
         const auto source_id = call_graph_manager->get_function(source);
         if(omp_functions->omp_for_wrappers.find(source_id) != omp_functions->omp_for_wrappers.end() or omp_functions->parallelized_functions.find(source_id) != omp_functions->parallelized_functions.end())
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found function to be parallelized: " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
            omp_functions->parallelized_functions.insert(function_id);
            break;
         }
      }
   }
   const auto panda_pthread_mutex = call_graph_manager->GetVertex(TM->function_index("panda_pthread_mutex"));
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(panda_pthread_mutex, *call_graph); ie != ie_end; ie++)
   {
      omp_functions->locking_functions.insert(call_graph_manager->get_function(boost::source(*ie, *call_graph)));
   }
   auto current_locks_allocation_candidates = omp_functions->locking_functions;
   CustomOrderedSet<unsigned int> reached_body_functions = call_graph_manager->GetReachedBodyFunctions();
   for(const auto function : boost::adaptors::reverse(sorted_functions))
   {
      const auto function_id = call_graph_manager->get_function(function);
      if(reached_body_functions.find(function_id) == reached_body_functions.end())
         continue;
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing function " + HLSMgr->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->get_function_name());
      if(current_locks_allocation_candidates.find(function_id) != current_locks_allocation_candidates.end())
      {
         current_locks_allocation_candidates.erase(current_locks_allocation_candidates.find(function_id));
#ifndef NDEBUG
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Other locks allocation candidates are:");
         for(const auto current_locks_allocation_candidate : current_locks_allocation_candidates)
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---" + HLSMgr->CGetFunctionBehavior(current_locks_allocation_candidate)->CGetBehavioralHelper()->get_function_name());
#endif
         if(current_locks_allocation_candidates.size() == 0)
         {
            /// This function is parallelized, so we have to move a step up
            if(omp_functions->parallelized_functions.find(function_id) != omp_functions->parallelized_functions.end())
            {
               for(boost::tie(ie, ie_end) = boost::in_edges(function, *call_graph); ie != ie_end; ie++)
               {
                  const auto source_id = call_graph_manager->get_function(boost::source(*ie, *call_graph));
                  if(reached_body_functions.find(source_id) == reached_body_functions.end())
                     current_locks_allocation_candidates.insert(source_id);
               }
               if(omp_functions->omp_for_wrappers.find(function_id) != omp_functions->omp_for_wrappers.end())
               {
                  omp_functions->locks_parallel_comunication.insert(function_id);
               }
               else
               {
                  omp_functions->locks_merge_communication.insert(function_id);
               }
            }
            else
            {
               omp_functions->locks_allocation = function_id;
            }
         }
         else
         {
            for(boost::tie(ie, ie_end) = boost::in_edges(function, *call_graph); ie != ie_end; ie++)
            {
               const auto source_id = call_graph_manager->get_function(boost::source(*ie, *call_graph));
               if(reached_body_functions.find(source_id) == reached_body_functions.end())
                  current_locks_allocation_candidates.insert(source_id);
            }
            if(omp_functions->omp_for_wrappers.find(function_id) != omp_functions->omp_for_wrappers.end())
            {
               omp_functions->locks_parallel_comunication.insert(function_id);
            }
            else
            {
               omp_functions->locks_merge_communication.insert(function_id);
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
   }
   if(omp_functions)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, STR(omp_functions));
   }

   return DesignFlowStep_Status::SUCCESS;
}

void OmpFunctionAllocation::Initialize()
{
   HLS_step::Initialize();
   HLSMgr->Rfuns = functionsRef(new OmpFunctions(HLSMgr));
}
