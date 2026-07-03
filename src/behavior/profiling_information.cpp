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
 * @file profiling_information.cpp
 * @brief Class implementation for storing profiling information.
 *
 * This structure stores information about loop and path profiling
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#include "profiling_information.hpp"

#include "basic_block.hpp"
#include "host_profiling_xml.hpp"
#include "ir_basic_block.hpp"
#include "loop.hpp"
#include "xml_element.hpp"
#include "xml_helper.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>

#include <string>
#include <utility>

#if HAVE_UNORDERED
BBExecutions::BBExecutions(const BBGraphsCollection*)
    : CustomUnorderedMap<BBGraphsCollection::vertex_descriptor, unsigned long long int>()
{
}
#else
BBExecutions::BBExecutions(const BBGraphsCollection* _bb_graph)
    : std::map<BBGraphsCollection::vertex_descriptor, unsigned long long int, BBVertexSorter>(BBVertexSorter(_bb_graph))
{
}
#endif

#if HAVE_UNORDERED
BBEdgeExecutions::BBEdgeExecutions(const BBGraphsCollection*)
    : CustomUnorderedMap<BBGraphsCollection::edge_descriptor, unsigned long long int>()
{
}
#else
BBEdgeExecutions::BBEdgeExecutions(const BBGraphsCollection* _bb_graph)
    : std::map<BBGraph::edge_descriptor, unsigned long long int, BBEdgeSorter>(BBEdgeSorter(_bb_graph))
{
}
#endif

ProfilingInformation::ProfilingInformation(const BBGraphsCollection* bb_graph)
    : bb_executions(bb_graph), edge_executions(bb_graph)
{
}

const PathProfilingInformation& ProfilingInformation::GetPathProfiling() const
{
   return path_profiling;
}

unsigned long long int ProfilingInformation::GetBBExecutions(BBGraph::vertex_descriptor bb_vertex) const
{
   if(bb_executions.find(bb_vertex) != bb_executions.end())
   {
      return bb_executions.find(bb_vertex)->second;
   }
   return 0.0;
}

unsigned long long int ProfilingInformation::GetEdgeExecutions(const BBGraph::edge_descriptor& edge) const
{
   if(edge_executions.find(edge) != edge_executions.end())
   {
      return edge_executions.find(edge)->second;
   }
   return 0.0;
}

unsigned long long int ProfilingInformation::GetLoopMaxIterations(const unsigned int loop_id) const
{
   if(max_iterations.find(loop_id) != max_iterations.end())
   {
      return max_iterations.find(loop_id)->second;
   }
   return 0.0;
}

long double ProfilingInformation::GetLoopAvgIterations(const unsigned int loop_id) const
{
   if(avg_iterations.find(loop_id) != avg_iterations.end())
   {
      return avg_iterations.find(loop_id)->second;
   }
   return 0.0L;
}

unsigned long long int ProfilingInformation::GetLoopAbsIterations(const unsigned int loop_id) const
{
   if(abs_iterations.find(loop_id) != abs_iterations.end())
   {
      return abs_iterations.find(loop_id)->second;
   }
   return 0;
}

long double ProfilingInformation::GetLoopAvgIterations(const LoopConstRef loop) const
{
   return GetLoopAvgIterations(loop->getLoopId());
}

unsigned long long int ProfilingInformation::GetLoopMaxIterations(const LoopConstRef loop) const
{
   return GetLoopMaxIterations(loop->getLoopId());
}

unsigned long long ProfilingInformation::GetLoopAbsIterations(const LoopConstRef loop) const
{
   return GetLoopAbsIterations(loop->getLoopId());
}

void ProfilingInformation::WriteToXml(xml_element* root, const BBGraph& fcfg) const
{
   xml_element* path_profiling_xml = root->add_child_element(STR_XML_host_profiling_paths);
   PathProfilingInformation::const_iterator loop, loop_end = path_profiling.end();
   for(loop = path_profiling.begin(); loop != loop_end; ++loop)
   {
      xml_element* loop_xml = path_profiling_xml->add_child_element(STR_XML_host_profiling_paths_loop);
      WRITE_XNVM2(STR_XML_host_profiling_id, std::to_string(loop->first), loop_xml);
      const std::map<CustomOrderedSet<unsigned int>, long double>& loop_path_profiling = loop->second;
      std::map<CustomOrderedSet<unsigned int>, long double>::const_iterator loop_path,
          loop_path_end = loop_path_profiling.end();
      for(loop_path = loop_path_profiling.begin(); loop_path != loop_path_end; ++loop_path)
      {
         xml_element* path = loop_xml->add_child_element(STR_XML_host_profiling_path);
         std::string cer_path_string;
         const CustomOrderedSet<unsigned int>& cer_path = loop_path->first;
         CustomOrderedSet<unsigned int>::const_iterator cer, cer_end = cer_path.end();
         for(cer = cer_path.begin(); cer != cer_end; ++cer)
         {
            cer_path_string += std::to_string(*cer) + "#";
         }
         WRITE_XNVM2(STR_XML_host_profiling_cers, cer_path_string, path);
         WRITE_XNVM2(STR_XML_host_profiling_frequency, std::to_string(loop_path->second), path);
      }
   }

   /// Map used to print profiling information in deterministic order
   std::map<unsigned int, long double> ordered_bb_executions;

   xml_element* bb_executions_xml = root->add_child_element(STR_XML_host_profiling_bb_executions);
   BBExecutions::const_iterator bb_execution, bb_execution_end = bb_executions.end();
   for(bb_execution = bb_executions.begin(); bb_execution != bb_execution_end; ++bb_execution)
   {
      ordered_bb_executions[fcfg.CGetNodeInfo(bb_execution->first).block->number] = bb_execution->second;
   }

   std::map<unsigned int, long double>::const_iterator ordered_bb_execution,
       ordered_bb_execution_end = ordered_bb_executions.end();
   for(ordered_bb_execution = ordered_bb_executions.begin(); ordered_bb_execution != ordered_bb_execution_end;
       ++ordered_bb_execution)
   {
      xml_element* bb_execution_xml = bb_executions_xml->add_child_element(STR_XML_host_profiling_bb_execution);
      WRITE_XNVM2(STR_XML_host_profiling_id, std::to_string(ordered_bb_execution->first), bb_execution_xml);
      WRITE_XNVM2(STR_XML_host_profiling_executions, std::to_string(ordered_bb_execution->second), bb_execution_xml);
   }

   /// Map used to print profiling information in deterministic order
   std::map<std::pair<unsigned int, unsigned int>, long double> ordered_edge_executions;

   xml_element* edge_executions_xml = root->add_child_element(STR_XML_host_profiling_edge_executions);
   BBEdgeExecutions::const_iterator edge_execution, edge_execution_end = edge_executions.end();
   for(edge_execution = edge_executions.begin(); edge_execution != edge_execution_end; ++edge_execution)
   {
      ordered_edge_executions[std::pair<unsigned int, unsigned int>(
          fcfg.CGetNodeInfo(fcfg.source(edge_execution->first)).block->number,
          fcfg.CGetNodeInfo(fcfg.target(edge_execution->first)).block->number)] = edge_execution->second;
   }

   std::map<std::pair<unsigned int, unsigned int>, long double>::const_iterator ordered_edge_execution,
       ordered_edge_execution_end = ordered_edge_executions.end();
   for(ordered_edge_execution = ordered_edge_executions.begin(); ordered_edge_execution != ordered_edge_execution_end;
       ++ordered_edge_execution)
   {
      xml_element* edge_execution_xml = edge_executions_xml->add_child_element(STR_XML_host_profiling_edge_execution);
      WRITE_XNVM2(STR_XML_host_profiling_source_id, std::to_string(ordered_edge_execution->first.first),
                  edge_execution_xml);
      WRITE_XNVM2(STR_XML_host_profiling_target_id, std::to_string(ordered_edge_execution->first.second),
                  edge_execution_xml);
      WRITE_XNVM2(STR_XML_host_profiling_executions, std::to_string(ordered_edge_execution->second),
                  edge_execution_xml);
   }

   xml_element* avg_iterations_xml = root->add_child_element(STR_XML_host_profiling_avg_iterations);
   AvgIterations::const_iterator avg_iteration, avg_iteration_end = avg_iterations.end();
   for(avg_iteration = avg_iterations.begin(); avg_iteration != avg_iteration_end; ++avg_iteration)
   {
      xml_element* avg_iteration_xml = avg_iterations_xml->add_child_element(STR_XML_host_profiling_avg_iteration);
      WRITE_XNVM2(STR_XML_host_profiling_id, std::to_string(avg_iteration->first), avg_iteration_xml);
      WRITE_XNVM2(STR_XML_host_profiling_iterations, std::to_string(avg_iteration->second), avg_iteration_xml);
   }

   xml_element* abs_iterations_xml = root->add_child_element(STR_XML_host_profiling_abs_iterations);
   Iterations::const_iterator abs_iteration, abs_iteration_end = abs_iterations.end();
   for(abs_iteration = abs_iterations.begin(); abs_iteration != abs_iteration_end; ++abs_iteration)
   {
      xml_element* abs_iteration_xml = abs_iterations_xml->add_child_element(STR_XML_host_profiling_abs_iteration);
      WRITE_XNVM2(STR_XML_host_profiling_id, std::to_string(abs_iteration->first), abs_iteration_xml);
      WRITE_XNVM2(STR_XML_host_profiling_iterations, std::to_string(abs_iteration->second), abs_iteration_xml);
   }

   xml_element* max_iterations_xml = root->add_child_element(STR_XML_host_profiling_max_iterations);
   Iterations::const_iterator max_iteration, max_iteration_end = max_iterations.end();
   for(max_iteration = max_iterations.begin(); max_iteration != max_iteration_end; ++max_iteration)
   {
      xml_element* max_iteration_xml = max_iterations_xml->add_child_element(STR_XML_host_profiling_max_iteration);
      WRITE_XNVM2(STR_XML_host_profiling_id, std::to_string(max_iteration->first), max_iteration_xml);
      WRITE_XNVM2(STR_XML_host_profiling_iterations, std::to_string(max_iteration->second), max_iteration_xml);
   }
}

void ProfilingInformation::Clear()
{
   path_profiling.clear();
   bb_executions.clear();
   edge_executions.clear();
   avg_iterations.clear();
   abs_iterations.clear();
   max_iterations.clear();
}
