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
 * @file call_graph.cpp
 * @brief Call graph hierarchy.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "call_graph.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "loops.hpp"
#include "string_manipulation.hpp"

#include <filesystem>
#include <ostream>
#include <utility>

#include "config_HAVE_HOST_PROFILING_BUILT.hpp"

CallGraph::CallGraph(const CallGraphsCollection& call_graphs_collection, const int _selector)
    : graph(call_graphs_collection, _selector)
{
}

CallGraph::CallGraph(const CallGraphsCollection& call_graphs_collection, const int _selector,
                     const CustomUnorderedSet<vertex_descriptor>& _vertices)
    : graph(call_graphs_collection, _selector, _vertices)
{
}

void CallGraph::writeDot(const std::filesystem::path& file_name) const
{
   std::filesystem::create_directories(file_name.parent_path());
   FunctionVertexWriter function_writer(*this);
   FunctionEdgeWriter function_edge_writer(*this);
   graph::writeDot(file_name, function_writer, function_edge_writer);
}

FunctionVertexWriter::FunctionVertexWriter(const CallGraph& call_graph)
    : VertexWriter(call_graph, 0), behaviors(call_graph.CGetGraphInfo().behaviors)
{
}

void FunctionVertexWriter::operator()(std::ostream& out, CallGraph::vertex_descriptor v) const
{
   THROW_ASSERT(behaviors.find(printing_graph.CGetNodeInfo(v).nodeID) != behaviors.end(),
                "Function " + std::to_string(printing_graph.CGetNodeInfo(v).nodeID) + " not found");
   const auto FB = behaviors.at(printing_graph.CGetNodeInfo(v).nodeID);
   out << "[shape=box, label=\"" << FB->CGetBehavioralHelper()->GetFunctionName();
   const CustomOrderedSet<unsigned int>& mem_nodeID = FB->get_function_mem();
   if(mem_nodeID.size())
   {
      out << "\\nMEMORY:";
      for(unsigned int l : mem_nodeID)
      {
         std::string label = FB->CGetBehavioralHelper()->PrintVariable(l);
         add_escape(label, "\"");
         out << "\\n";
         out << label;
      }
   }
   out << "\"]";
}

FunctionEdgeWriter::FunctionEdgeWriter(const CallGraph& call_graph)
    : EdgeWriter(call_graph, 0), behaviors(call_graph.CGetGraphInfo().behaviors)
{
}

void FunctionEdgeWriter::operator()(std::ostream& out, const CallGraph::edge_descriptor& e) const
{
   const auto& direct_call_points = printing_graph.CGetEdgeInfo(e).direct_call_points;
   const auto& indirect_call_points = printing_graph.CGetEdgeInfo(e).indirect_call_points;
   const auto& function_addresses = printing_graph.CGetEdgeInfo(e).function_addresses;
   std::string color;
   if(STD_SELECTOR & printing_graph.GetSelector(e))
   {
      color = "blue";
   }
   else if(FEEDBACK_SELECTOR & printing_graph.GetSelector(e))
   {
      color = "red";
   }
   else
   {
      THROW_ERROR(std::string("InconsistentDataStructure"));
   }

   out << "[color=" << color << ", label=\"";
   if(direct_call_points.size())
   {
      out << "DIRECT: ";
      for(const auto& call : direct_call_points)
      {
         out << "\\n" << call;
      }
   }
   if(indirect_call_points.size())
   {
      if(direct_call_points.size())
      {
         out << "\\n";
      }
      out << "INDIRECT: ";
      for(const auto& call : indirect_call_points)
      {
         out << "\\n" << call;
      }
   }
   if(function_addresses.size())
   {
      if(direct_call_points.size() or indirect_call_points.size())
      {
         out << "\\n";
      }
      out << "TAKE ADDRESS: ";
      for(const auto& call : function_addresses)
      {
         out << "\\n" << call;
      }
   }
   out << "\"]";
}
