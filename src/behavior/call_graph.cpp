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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

///Autoheader include
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "config_HAVE_POLIXML_BUILT.hpp"

///Header include
#include "call_graph.hpp"

///Behavior include
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "loop.hpp"
#include "loops.hpp"
#include "op_graph.hpp"
#if HAVE_HOST_PROFILING_BUILT
#include "profiling_information.hpp"
#endif
///Boost include
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

///Graph include
#include "graph.hpp"
#include <boost/graph/graphviz.hpp>

///Paramter include
#include "Parameter.hpp"

///Utility include
#include <boost/date_time/posix_time/posix_time.hpp>

///XML include
#if HAVE_POLIXML_BUILT
#include "xml_helper.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_document.hpp"
#endif

/**
 * @name function graph selector
 */
//@{
/// Data line selector
#define STD_SELECTOR 1 << 0
/// Clock line selector
#define FEEDBACK_SELECTOR 1 << 1
//@}

FunctionInfo::FunctionInfo() :
   nodeID(0)
{}

FunctionEdgeInfo::FunctionEdgeInfo()
{}

CallGraphsCollection::CallGraphsCollection(const CallGraphInfoRef call_graph_info, const ParameterConstRef _parameters) :
   graphs_collection(call_graph_info, _parameters)
{}

CallGraphsCollection::~CallGraphsCollection()
{}

CallGraph::CallGraph(const CallGraphsCollectionRef call_graphs_collection, const int _selector) :
   graph(call_graphs_collection.get(), _selector)
{}

CallGraph::CallGraph(const CallGraphsCollectionRef call_graphs_collection, const int _selector, const std::unordered_set<vertex> &_vertices) :
   graph(call_graphs_collection.get(), _selector, _vertices)
{}

CallGraph::~CallGraph()
{}

void CallGraph::WriteDot(const std::string& file_name) const
{
   const std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory);
   if (!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name;
   const VertexWriterConstRef function_writer(new FunctionWriter(this));
   const EdgeWriterConstRef function_edge_writer(new FunctionEdgeWriter(this));
   InternalWriteDot<const FunctionWriter, const FunctionEdgeWriter>(full_name, function_writer, function_edge_writer);
}

FunctionWriter::FunctionWriter(const CallGraph * call_graph) :
   VertexWriter(call_graph, 0),
   behaviors(call_graph->CGetCallGraphInfo()->behaviors)
{}

void FunctionWriter::operator()(std::ostream & out, const vertex & v) const
{
   THROW_ASSERT(behaviors.find(Cget_node_info<FunctionInfo, graph>(v, *printing_graph)->nodeID) != behaviors.end(), "Function " + boost::lexical_cast<std::string>(Cget_node_info<FunctionInfo, graph>(v, *printing_graph)->nodeID) + " not found");
   const FunctionBehaviorRef FB = behaviors.find(Cget_node_info<FunctionInfo, graph>(v, *printing_graph)->nodeID)->second;
   out << "[shape=box, label=\"" << FB->CGetBehavioralHelper()->get_function_name();
   const std::set<unsigned int>& mem_nodeID = FB->get_function_mem();
   if (mem_nodeID.size())
   {
      out << "\\nMEMORY:";
      for(std::set<unsigned int>::const_iterator l = mem_nodeID.begin(); l != mem_nodeID.end(); ++l)
      {
         std::string label = FB->CGetBehavioralHelper()->PrintVariable(*l);
         add_escape(label, "\"");
	 out << "\\n";
         out << label;
      }
   }
   out << "\"]" ;
}

FunctionEdgeWriter::FunctionEdgeWriter(const CallGraph * call_graph) :
   EdgeWriter(call_graph, 0),
   behaviors(call_graph->CGetCallGraphInfo()->behaviors)
{}

FunctionEdgeWriter::~FunctionEdgeWriter()
{}

void FunctionEdgeWriter::operator()(std::ostream& out, const EdgeDescriptor& e) const
{
   const std::set<unsigned int> & direct_call_points = Cget_edge_info<FunctionEdgeInfo, graph>(e, *printing_graph)->direct_call_points;
   const std::set<unsigned int> & indirect_call_points = Cget_edge_info<FunctionEdgeInfo, graph>(e, *printing_graph)->indirect_call_points;
   const std::set<unsigned int> & function_addresses = Cget_edge_info<FunctionEdgeInfo, graph>(e, *printing_graph)->function_addresses;
   std::string color;
   if (STD_SELECTOR & printing_graph->GetSelector(e))
	   color = "blue";
   else if (FEEDBACK_SELECTOR & printing_graph->GetSelector(e))
      color = "red";
   else
      THROW_ERROR(std::string("InconsistentDataStructure"));

   out << "[color=" << color << ", label=\"";
   if (direct_call_points.size())
   {
      out << "DIRECT: ";
      for (const auto & call : direct_call_points)
      {
         out << "\\n" << call;
      }
   }
   if (indirect_call_points.size())
   {
      if (direct_call_points.size())
         out << "\\n";
      out << "INDIRECT: ";
      for (const auto & call : indirect_call_points)
      {
         out << "\\n" << call;
      }
   }
   if (function_addresses.size())
   {
      if (direct_call_points.size() or indirect_call_points.size())
         out << "\\n";
      out << "TAKE ADDRESS: ";
      for (const auto & call : function_addresses)
      {
         out << "\\n" << call;
      }
   }
   out << "\"]";
}

