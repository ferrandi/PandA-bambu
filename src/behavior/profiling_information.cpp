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
 * @file profiling_information.cpp
 * @brief Class implementation for storing profiling information.
 *
 * This structure stores information about loop and path profiling
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "profiling_information.hpp"

#include "basic_block.hpp"                // for BBEdgeSorter, BBVertexS...
#include "host_profiling_xml.hpp"         // for STR_XML_host_profiling_id
#include "loop.hpp"                       // for LoopConstRef, Loop
#include "tree_basic_block.hpp"           // for bloc
#include "xml_element.hpp"                // for xml_element
#include "xml_helper.hpp"                 // for WRITE_XNVM2
#include <boost/graph/adjacency_list.hpp> // for adjacency_list, source
#include <boost/graph/filtered_graph.hpp> // for source, target
#include <boost/lexical_cast.hpp>         // for lexical_cast
#include <string>                         // for string, operator+
#include <utility>                        // for pair

#if HAVE_UNORDERED
BBExecutions::BBExecutions(const BBGraphConstRef) : CustomUnorderedMap<vertex, unsigned long long int>()
{
}
#else
BBExecutions::BBExecutions(const BBGraphConstRef _bb_graph) : std::map<vertex, unsigned long long int, BBVertexSorter>(BBVertexSorter(_bb_graph))
{
}
#endif

#if HAVE_UNORDERED
BBEdgeExecutions::BBEdgeExecutions(const BBGraphConstRef) : CustomUnorderedMap<EdgeDescriptor, unsigned long long int>()
{
}
#else
BBEdgeExecutions::BBEdgeExecutions(const BBGraphConstRef _bb_graph) : std::map<EdgeDescriptor, unsigned long long int, BBEdgeSorter>(BBEdgeSorter(_bb_graph))
{
}
#endif

ProfilingInformation::ProfilingInformation(const BBGraphConstRef _bb_graph) : bb_executions(_bb_graph), edge_executions(_bb_graph)
{
}

ProfilingInformation::~ProfilingInformation() = default;

const PathProfilingInformation& ProfilingInformation::GetPathProfiling() const
{
   return path_profiling;
}

unsigned long long int ProfilingInformation::GetBBExecutions(const vertex bb_vertex) const
{
   if(bb_executions.find(bb_vertex) != bb_executions.end())
      return bb_executions.find(bb_vertex)->second;
   return 0.0;
}

unsigned long long int ProfilingInformation::GetEdgeExecutions(const EdgeDescriptor edge) const
{
   if(edge_executions.find(edge) != edge_executions.end())
      return edge_executions.find(edge)->second;
   return 0.0;
}

unsigned long long int ProfilingInformation::GetLoopMaxIterations(const unsigned int loop_id) const
{
   if(max_iterations.find(loop_id) != max_iterations.end())
      return max_iterations.find(loop_id)->second;
   return 0.0;
}

long double ProfilingInformation::GetLoopAvgIterations(const unsigned int loop_id) const
{
   if(avg_iterations.find(loop_id) != avg_iterations.end())
      return avg_iterations.find(loop_id)->second;
   return 0.0L;
}

unsigned long long int ProfilingInformation::GetLoopAbsIterations(const unsigned int loop_id) const
{
   if(abs_iterations.find(loop_id) != abs_iterations.end())
      return abs_iterations.find(loop_id)->second;
   return 0;
}

long double ProfilingInformation::GetLoopAvgIterations(const LoopConstRef loop) const
{
   return GetLoopAvgIterations(loop->GetId());
}

unsigned long long int ProfilingInformation::GetLoopMaxIterations(const LoopConstRef loop) const
{
   return GetLoopMaxIterations(loop->GetId());
}

unsigned long long ProfilingInformation::GetLoopAbsIterations(const LoopConstRef loop) const
{
   return GetLoopAbsIterations(loop->GetId());
}

void ProfilingInformation::WriteToXml(xml_element* root, const BBGraphConstRef fcfg) const
{
   xml_element* path_profiling_xml = root->add_child_element(STR_XML_host_profiling_paths);
   PathProfilingInformation::const_iterator loop, loop_end = path_profiling.end();
   for(loop = path_profiling.begin(); loop != loop_end; ++loop)
   {
      xml_element* loop_xml = path_profiling_xml->add_child_element(STR_XML_host_profiling_paths_loop);
      WRITE_XNVM2(STR_XML_host_profiling_id, boost::lexical_cast<std::string>(loop->first), loop_xml);
      const std::map<CustomOrderedSet<unsigned int>, long double>& loop_path_profiling = loop->second;
      std::map<CustomOrderedSet<unsigned int>, long double>::const_iterator loop_path, loop_path_end = loop_path_profiling.end();
      for(loop_path = loop_path_profiling.begin(); loop_path != loop_path_end; ++loop_path)
      {
         xml_element* path = loop_xml->add_child_element(STR_XML_host_profiling_path);
         std::string cer_path_string;
         const CustomOrderedSet<unsigned int>& cer_path = loop_path->first;
         CustomOrderedSet<unsigned int>::const_iterator cer, cer_end = cer_path.end();
         for(cer = cer_path.begin(); cer != cer_end; ++cer)
         {
            cer_path_string += boost::lexical_cast<std::string>(*cer) + "#";
         }
         WRITE_XNVM2(STR_XML_host_profiling_cers, cer_path_string, path);
         WRITE_XNVM2(STR_XML_host_profiling_frequency, boost::lexical_cast<std::string>(loop_path->second), path);
      }
   }

   /// Map used to print profiling information in deterministic order
   std::map<unsigned int, long double> ordered_bb_executions;

   xml_element* bb_executions_xml = root->add_child_element(STR_XML_host_profiling_bb_executions);
   BBExecutions::const_iterator bb_execution, bb_execution_end = bb_executions.end();
   for(bb_execution = bb_executions.begin(); bb_execution != bb_execution_end; ++bb_execution)
   {
      ordered_bb_executions[fcfg->CGetBBNodeInfo(bb_execution->first)->block->number] = bb_execution->second;
   }

   std::map<unsigned int, long double>::const_iterator ordered_bb_execution, ordered_bb_execution_end = ordered_bb_executions.end();
   for(ordered_bb_execution = ordered_bb_executions.begin(); ordered_bb_execution != ordered_bb_execution_end; ++ordered_bb_execution)
   {
      xml_element* bb_execution_xml = bb_executions_xml->add_child_element(STR_XML_host_profiling_bb_execution);
      WRITE_XNVM2(STR_XML_host_profiling_id, boost::lexical_cast<std::string>(ordered_bb_execution->first), bb_execution_xml);
      WRITE_XNVM2(STR_XML_host_profiling_executions, boost::lexical_cast<std::string>(ordered_bb_execution->second), bb_execution_xml);
   }

   /// Map used to print profiling information in deterministic order
   std::map<std::pair<unsigned int, unsigned int>, long double> ordered_edge_executions;

   xml_element* edge_executions_xml = root->add_child_element(STR_XML_host_profiling_edge_executions);
   BBEdgeExecutions::const_iterator edge_execution, edge_execution_end = edge_executions.end();
   for(edge_execution = edge_executions.begin(); edge_execution != edge_execution_end; ++edge_execution)
   {
      ordered_edge_executions[std::pair<unsigned int, unsigned int>(fcfg->CGetBBNodeInfo(boost::source(edge_execution->first, *fcfg))->block->number, fcfg->CGetBBNodeInfo(boost::target(edge_execution->first, *fcfg))->block->number)] =
          edge_execution->second;
   }

   std::map<std::pair<unsigned int, unsigned int>, long double>::const_iterator ordered_edge_execution, ordered_edge_execution_end = ordered_edge_executions.end();
   for(ordered_edge_execution = ordered_edge_executions.begin(); ordered_edge_execution != ordered_edge_execution_end; ++ordered_edge_execution)
   {
      xml_element* edge_execution_xml = edge_executions_xml->add_child_element(STR_XML_host_profiling_edge_execution);
      WRITE_XNVM2(STR_XML_host_profiling_source_id, boost::lexical_cast<std::string>(ordered_edge_execution->first.first), edge_execution_xml);
      WRITE_XNVM2(STR_XML_host_profiling_target_id, boost::lexical_cast<std::string>(ordered_edge_execution->first.second), edge_execution_xml);
      WRITE_XNVM2(STR_XML_host_profiling_executions, boost::lexical_cast<std::string>(ordered_edge_execution->second), edge_execution_xml);
   }

   xml_element* avg_iterations_xml = root->add_child_element(STR_XML_host_profiling_avg_iterations);
   AvgIterations::const_iterator avg_iteration, avg_iteration_end = avg_iterations.end();
   for(avg_iteration = avg_iterations.begin(); avg_iteration != avg_iteration_end; ++avg_iteration)
   {
      xml_element* avg_iteration_xml = avg_iterations_xml->add_child_element(STR_XML_host_profiling_avg_iteration);
      WRITE_XNVM2(STR_XML_host_profiling_id, boost::lexical_cast<std::string>(avg_iteration->first), avg_iteration_xml);
      WRITE_XNVM2(STR_XML_host_profiling_iterations, boost::lexical_cast<std::string>(avg_iteration->second), avg_iteration_xml);
   }

   xml_element* abs_iterations_xml = root->add_child_element(STR_XML_host_profiling_abs_iterations);
   Iterations::const_iterator abs_iteration, abs_iteration_end = abs_iterations.end();
   for(abs_iteration = abs_iterations.begin(); abs_iteration != abs_iteration_end; ++abs_iteration)
   {
      xml_element* abs_iteration_xml = abs_iterations_xml->add_child_element(STR_XML_host_profiling_abs_iteration);
      WRITE_XNVM2(STR_XML_host_profiling_id, boost::lexical_cast<std::string>(abs_iteration->first), abs_iteration_xml);
      WRITE_XNVM2(STR_XML_host_profiling_iterations, boost::lexical_cast<std::string>(abs_iteration->second), abs_iteration_xml);
   }

   xml_element* max_iterations_xml = root->add_child_element(STR_XML_host_profiling_max_iterations);
   Iterations::const_iterator max_iteration, max_iteration_end = max_iterations.end();
   for(max_iteration = max_iterations.begin(); max_iteration != max_iteration_end; ++max_iteration)
   {
      xml_element* max_iteration_xml = max_iterations_xml->add_child_element(STR_XML_host_profiling_max_iteration);
      WRITE_XNVM2(STR_XML_host_profiling_id, boost::lexical_cast<std::string>(max_iteration->first), max_iteration_xml);
      WRITE_XNVM2(STR_XML_host_profiling_iterations, boost::lexical_cast<std::string>(max_iteration->second), max_iteration_xml);
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
