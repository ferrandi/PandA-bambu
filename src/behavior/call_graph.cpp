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
 * @file call_graph.cpp
 * @brief Call graph hierarchy.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "call_graph.hpp"

#include "Parameter.hpp"         // for OPT_dot_directory
#include "behavioral_helper.hpp" // for BehavioralHelper
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "exceptions.hpp"                     // for THROW_ASSERT, THROW...
#include "function_behavior.hpp"              // for FunctionBehavior
#include "graph.hpp"                          // for graph, Cget_edge_info
#include "loops.hpp"                          // for FunctionBehaviorRef
#include "string_manipulation.hpp"            // for add_escape
#include <boost/filesystem/operations.hpp>    // for create_directories
#include <boost/iterator/iterator_facade.hpp> // for operator!=, operator++
#include <boost/lexical_cast.hpp>             // for lexical_cast
#include <ostream>                            // for operator<<, ostream
#include <utility>                            // for pair

/**
 * @name function graph selector
 */
//@{
/// Data line selector
#define STD_SELECTOR 1 << 0
/// Clock line selector
#define FEEDBACK_SELECTOR 1 << 1
//@}

FunctionInfo::FunctionInfo() : nodeID(0)
{
}

FunctionEdgeInfo::FunctionEdgeInfo() = default;

CallGraphsCollection::CallGraphsCollection(const CallGraphInfoRef call_graph_info, const ParameterConstRef _parameters) : graphs_collection(call_graph_info, _parameters)
{
}

CallGraphsCollection::~CallGraphsCollection() = default;

CallGraph::CallGraph(const CallGraphsCollectionRef call_graphs_collection, const int _selector) : graph(call_graphs_collection.get(), _selector)
{
}

CallGraph::CallGraph(const CallGraphsCollectionRef call_graphs_collection, const int _selector, const CustomUnorderedSet<vertex>& _vertices) : graph(call_graphs_collection.get(), _selector, _vertices)
{
}

CallGraph::~CallGraph() = default;

void CallGraph::WriteDot(const std::string& file_name) const
{
   const std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory);
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name;
   const VertexWriterConstRef function_writer(new FunctionWriter(this));
   const EdgeWriterConstRef function_edge_writer(new FunctionEdgeWriter(this));
   InternalWriteDot<const FunctionWriter, const FunctionEdgeWriter>(full_name, function_writer, function_edge_writer);
}

FunctionWriter::FunctionWriter(const CallGraph* call_graph) : VertexWriter(call_graph, 0), behaviors(call_graph->CGetCallGraphInfo()->behaviors)
{
}

void FunctionWriter::operator()(std::ostream& out, const vertex& v) const
{
   THROW_ASSERT(behaviors.find(Cget_node_info<FunctionInfo, graph>(v, *printing_graph)->nodeID) != behaviors.end(), "Function " + boost::lexical_cast<std::string>(Cget_node_info<FunctionInfo, graph>(v, *printing_graph)->nodeID) + " not found");
   const FunctionBehaviorRef FB = behaviors.find(Cget_node_info<FunctionInfo, graph>(v, *printing_graph)->nodeID)->second;
   out << "[shape=box, label=\"" << FB->CGetBehavioralHelper()->get_function_name();
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

FunctionEdgeWriter::FunctionEdgeWriter(const CallGraph* call_graph) : EdgeWriter(call_graph, 0), behaviors(call_graph->CGetCallGraphInfo()->behaviors)
{
}

FunctionEdgeWriter::~FunctionEdgeWriter() = default;

void FunctionEdgeWriter::operator()(std::ostream& out, const EdgeDescriptor& e) const
{
   const CustomOrderedSet<unsigned int>& direct_call_points = Cget_edge_info<FunctionEdgeInfo, graph>(e, *printing_graph)->direct_call_points;
   const CustomOrderedSet<unsigned int>& indirect_call_points = Cget_edge_info<FunctionEdgeInfo, graph>(e, *printing_graph)->indirect_call_points;
   const CustomOrderedSet<unsigned int>& function_addresses = Cget_edge_info<FunctionEdgeInfo, graph>(e, *printing_graph)->function_addresses;
   std::string color;
   if(STD_SELECTOR & printing_graph->GetSelector(e))
      color = "blue";
   else if(FEEDBACK_SELECTOR & printing_graph->GetSelector(e))
      color = "red";
   else
      THROW_ERROR(std::string("InconsistentDataStructure"));

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
         out << "\\n";
      out << "INDIRECT: ";
      for(const auto& call : indirect_call_points)
      {
         out << "\\n" << call;
      }
   }
   if(function_addresses.size())
   {
      if(direct_call_points.size() or indirect_call_points.size())
         out << "\\n";
      out << "TAKE ADDRESS: ";
      for(const auto& call : function_addresses)
      {
         out << "\\n" << call;
      }
   }
   out << "\"]";
}
