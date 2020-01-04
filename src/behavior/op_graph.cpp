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
 * @file op_graph.cpp
 * @brief Data structures used in operations graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "op_graph.hpp"

#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "behavioral_writer_helper.hpp"
#include "exceptions.hpp" // for THROW_ASSERT, THROW...
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"
#include <boost/filesystem/operations.hpp> // for create_directories
#include <boost/tuple/tuple.hpp>           // for tie
#include <fstream>
#include <utility> // for pair

/// Utility include

OpEdgeInfo::OpEdgeInfo() = default;

OpEdgeInfo::~OpEdgeInfo() = default;

bool OpEdgeInfo::FlgEdgeT() const
{
   if(labels.find(FLG_SELECTOR) == labels.end())
      return false;
   return labels.find(FLG_SELECTOR)->second.find(T_COND) != labels.find(FLG_SELECTOR)->second.end();
}

bool OpEdgeInfo::FlgEdgeF() const
{
   if(labels.find(FLG_SELECTOR) == labels.end())
      return false;
   return labels.find(FLG_SELECTOR)->second.find(F_COND) != labels.find(FLG_SELECTOR)->second.end();
}

OpNodeInfo::OpNodeInfo() : node(tree_nodeRef()), bb_index(0), cer(0)
{
   Initialize();
}

void OpNodeInfo::Initialize()
{
   /// This is necessary to be sure that the set exists (even if empty)
   variables[FunctionBehavior_VariableType::SCALAR][FunctionBehavior_VariableAccessType::USE] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::SCALAR][FunctionBehavior_VariableAccessType::DEFINITION] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::SCALAR][FunctionBehavior_VariableAccessType::OVER] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::SCALAR][FunctionBehavior_VariableAccessType::ADDRESS] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::MEMORY][FunctionBehavior_VariableAccessType::USE] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::MEMORY][FunctionBehavior_VariableAccessType::DEFINITION] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::MEMORY][FunctionBehavior_VariableAccessType::OVER] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::MEMORY][FunctionBehavior_VariableAccessType::ADDRESS] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::VIRTUAL][FunctionBehavior_VariableAccessType::USE] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::VIRTUAL][FunctionBehavior_VariableAccessType::DEFINITION] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::VIRTUAL][FunctionBehavior_VariableAccessType::OVER] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::VIRTUAL][FunctionBehavior_VariableAccessType::ADDRESS] = CustomSet<unsigned int>();
#if HAVE_EXPERIMENTAL
   variables[FunctionBehavior_VariableType::AGGREGATE][FunctionBehavior_VariableAccessType::USE] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::AGGREGATE][FunctionBehavior_VariableAccessType::DEFINITION] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::AGGREGATE][FunctionBehavior_VariableAccessType::OVER] = CustomSet<unsigned int>();
   variables[FunctionBehavior_VariableType::AGGREGATE][FunctionBehavior_VariableAccessType::ADDRESS] = CustomSet<unsigned int>();
   dynamic_memory_locations[FunctionBehavior_VariableAccessType::USE] = CustomSet<MemoryAddress>();
   dynamic_memory_locations[FunctionBehavior_VariableAccessType::DEFINITION] = CustomSet<MemoryAddress>();
   dynamic_memory_locations[FunctionBehavior_VariableAccessType::OVER] = CustomSet<MemoryAddress>();
#endif
}

OpNodeInfo::~OpNodeInfo() = default;

const CustomSet<unsigned int>& OpNodeInfo::GetVariables(const FunctionBehavior_VariableType variable_type, const FunctionBehavior_VariableAccessType access_type) const
{
   return variables.find(variable_type)->second.find(access_type)->second;
}

#if HAVE_EXPERIMENTAL
const CustomSet<MemoryAddress>& OpNodeInfo::GetDynamicMemoryLocations(const FunctionBehavior_VariableAccessType access_type) const
{
   return dynamic_memory_locations.find(access_type)->second;
}
#endif

#if HAVE_BAMBU_BUILT || HAVE_TUCANO_BUILT
const std::string OpNodeInfo::GetOperation() const
{
   if(vertex_name == ENTRY)
      return ENTRY;
   if(vertex_name == EXIT)
      return EXIT;
   if(vertex_name.find("_#empty_") != std::string::npos)
   {
      return NOP;
   }
   THROW_ASSERT(node, "");
   THROW_ASSERT(GetPointer<const gimple_node>(GET_NODE(node)), "Node is not a gimple_node but a " + GET_NODE(node)->get_kind_text());
   return GetPointer<const gimple_node>(GET_NODE(node))->operation;
}
#endif

unsigned int OpNodeInfo::GetNodeId() const
{
   if(node)
      return node->index;
   else if(vertex_name == ENTRY)
      return ENTRY_ID;
   else if(vertex_name == EXIT)
      return EXIT_ID;
   THROW_UNREACHABLE("");
   return 0;
}

void PrintVariablesList(std::ostream& stream, const std::string& name, const CustomSet<unsigned int> variables, const BehavioralHelperConstRef behavioral_helper, const bool dotty_format)
{
   if(variables.size())
      stream << name << ":" << (dotty_format ? "\\n" : "\n");

   for(const auto& variable : variables)
      stream << behavioral_helper->PrintVariable(variable) << "(" << variable << ")" << (dotty_format ? "\\n" : "\n");
}

void PrintMemoriesList(std::ostream& stream, const std::string& name, const CustomSet<MemoryAddress> variables, const BehavioralHelperConstRef, const bool dotty_format)
{
   if(variables.size())
      stream << name << ":" << (dotty_format ? "\\n" : "\n");
   for(const auto& variable : variables)
      stream << from_strongtype_cast<int>(variable) << (dotty_format ? "\\n" : "\n");
}

void PrintVariablesLists(std::ostream& stream, const std::string& name, const CustomMap<FunctionBehavior_VariableAccessType, CustomSet<unsigned int>> variables, const BehavioralHelperConstRef behavioral_helper, const bool dotty_format)
{
   for(const auto& local_variables : variables)
   {
      if(local_variables.second.size())
      {
         stream << name << ":" << (dotty_format ? "\\n" : "\n");
         PrintVariablesList(stream, "USES", variables.find(FunctionBehavior_VariableAccessType::USE)->second, behavioral_helper, dotty_format);
         PrintVariablesList(stream, "DEFS", variables.find(FunctionBehavior_VariableAccessType::DEFINITION)->second, behavioral_helper, dotty_format);
         PrintVariablesList(stream, "OVERS", variables.find(FunctionBehavior_VariableAccessType::OVER)->second, behavioral_helper, dotty_format);
         PrintVariablesList(stream, "ADDR", variables.find(FunctionBehavior_VariableAccessType::ADDRESS)->second, behavioral_helper, dotty_format);
         break;
      }
   }
}

void PrintMemoriesLists(std::ostream& stream, const std::string& name, const CustomMap<FunctionBehavior_VariableAccessType, CustomSet<MemoryAddress>> variables, const BehavioralHelperConstRef behavioral_helper, const bool dotty_format)
{
   for(const auto& local_variables : variables)
   {
      if(local_variables.second.size())
      {
         stream << name << ":" << (dotty_format ? "\\n" : "\n");
         PrintMemoriesList(stream, "USES", variables.find(FunctionBehavior_VariableAccessType::USE)->second, behavioral_helper, dotty_format);
         PrintMemoriesList(stream, "DEFS", variables.find(FunctionBehavior_VariableAccessType::DEFINITION)->second, behavioral_helper, dotty_format);
         PrintMemoriesList(stream, "OVERS", variables.find(FunctionBehavior_VariableAccessType::OVER)->second, behavioral_helper, dotty_format);
         break;
      }
   }
}

void OpNodeInfo::Print(std::ostream& stream, const BehavioralHelperConstRef behavioral_helper, bool dotty_format) const
{
   PrintVariablesList(stream, "source code variables", cited_variables, behavioral_helper, dotty_format);
   PrintVariablesLists(stream, "SCALARS", variables.find(FunctionBehavior_VariableType::SCALAR)->second, behavioral_helper, dotty_format);
#if HAVE_EXPERIMENTAL
   PrintVariablesLists(stream, "AGGREGATE", variables.find(FunctionBehavior_VariableType::AGGREGATE)->second, behavioral_helper, dotty_format);
#endif
   PrintVariablesLists(stream, "MEMORY", variables.find(FunctionBehavior_VariableType::MEMORY)->second, behavioral_helper, dotty_format);
   PrintVariablesLists(stream, "VIRTUAL", variables.find(FunctionBehavior_VariableType::VIRTUAL)->second, behavioral_helper, dotty_format);
#if HAVE_EXPERIMENTAL
   PrintMemoriesLists(stream, "DYNAMIC MEMORY", dynamic_memory_locations, behavioral_helper, dotty_format);
#endif
}

OpGraphInfo::OpGraphInfo(const BehavioralHelperConstRef _BH) : entry_vertex(NULL_VERTEX), exit_vertex(NULL_VERTEX), BH(_BH)
{
}

OpGraphInfo::~OpGraphInfo() = default;

OpGraphsCollection::OpGraphsCollection(const OpGraphInfoRef _info, const ParameterConstRef _parameters)
    : graphs_collection(RefcountCast<GraphInfo>(_info), _parameters), operations(OpGraphConstRef(new OpGraph(OpGraphsCollectionRef(this, null_deleter()), 0)))
{
}

OpGraphsCollection::~OpGraphsCollection() = default;

const OpVertexSet OpGraphsCollection::CGetOperations() const
{
   return operations;
}

void OpGraphsCollection::RemoveVertex(boost::graph_traits<boost_graphs_collection>::vertex_descriptor v)
{
   operations.erase(v);
   graphs_collection::RemoveVertex(v);
}

boost::graph_traits<boost_graphs_collection>::vertex_descriptor OpGraphsCollection::AddVertex(const NodeInfoRef info)
{
   const auto new_vertex = graphs_collection::AddVertex(info);
   operations.insert(new_vertex);
   return new_vertex;
}

void OpGraphsCollection::Clear()
{
   operations.clear();
   graphs_collection::clear();
}

#if HAVE_UNORDERED
OpVertexSet::OpVertexSet(const OpGraphConstRef) : CustomUnorderedSet<vertex>()
{
}

OpEdgeSet::OpEdgeSet(const OpGraphConstRef) : CustomUnorderedSet<EdgeDescriptor>()
{
}

#else
OpVertexSorter::OpVertexSorter(const OpGraphConstRef _op_graph) : op_graph(_op_graph)
{
}

bool OpVertexSorter::operator()(const vertex x, const vertex y) const
{
   return GET_NAME(op_graph, x) < GET_NAME(op_graph, y);
}

OpVertexSet::OpVertexSet(OpGraphConstRef _op_graph) : std::set<vertex, OpVertexSorter>(OpVertexSorter(_op_graph))
{
}

OpEdgeSorter::OpEdgeSorter(const OpGraphConstRef _op_graph) : op_graph(_op_graph)
{
}

bool OpEdgeSorter::operator()(const EdgeDescriptor x, const EdgeDescriptor y) const
{
   if(x != y)
   {
      return GET_NAME(op_graph, boost::source(x, *op_graph)) < GET_NAME(op_graph, boost::source(y, *op_graph));
   }
   return GET_NAME(op_graph, boost::target(x, *op_graph)) < GET_NAME(op_graph, boost::target(y, *op_graph));
}

OpEdgeSet::OpEdgeSet(OpGraphConstRef _op_graph) : std::set<EdgeDescriptor, OpEdgeSorter>(OpEdgeSorter(_op_graph))
{
}
#endif

OpGraph::OpGraph(OpGraphsCollectionRef _op_graphs_collection, int _selector) : graph(_op_graphs_collection.get(), _selector)
{
}

OpGraph::OpGraph(const OpGraphsCollectionRef _op_graphs_collection, int _selector, const CustomUnorderedSet<boost::graph_traits<OpGraphsCollection>::vertex_descriptor>& _sub) : graph(_op_graphs_collection.get(), _selector, _sub)
{
}

OpGraph::~OpGraph() = default;

void OpGraph::WriteDot(const std::string& file_name, const int detail_level) const
{
   const BehavioralHelperConstRef helper = CGetOpGraphInfo()->BH;
   std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory) + "/" + helper->get_function_name() + "/";
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name;
   const VertexWriterConstRef op_label_writer(new OpWriter(this, detail_level));
   const EdgeWriterConstRef op_edge_property_writer(new OpEdgeWriter(this));
   InternalWriteDot<const OpWriter, const OpEdgeWriter>(full_name, op_label_writer, op_edge_property_writer);
}

CustomUnorderedMap<vertex, OpVertexSet> OpGraph::GetSrcVertices(const OpVertexSet& toCheck, int edgeType) const
{
   null_deleter null;
   OpGraphConstRef thisRef(this, null);
   CustomUnorderedMap<vertex, OpVertexSet> retVal;
   OpVertexSet::const_iterator vertIter, vertIterEnd;
   for(vertIter = toCheck.begin(), vertIterEnd = toCheck.end(); vertIter != vertIterEnd; ++vertIter)
   {
      InEdgeIterator inE, inEEnd;
      for(boost::tie(inE, inEEnd) = boost::in_edges(*vertIter, *this); inE != inEEnd; inE++)
      {
         int origEdgeType = GetSelector(*inE);
         if((edgeType & origEdgeType) != 0)
         {
            const vertex src = boost::source(*inE, *this);
            if(retVal.find(src) == retVal.end())
            {
               retVal.insert(std::pair<vertex, OpVertexSet>(src, OpVertexSet(thisRef)));
            }
            retVal.find(src)->second.insert(*vertIter);
         }
      }
   }
   return retVal;
}

const OpVertexSet OpGraph::CGetOperations() const
{
   const auto ret_value = dynamic_cast<OpGraphsCollection*>(collection)->CGetOperations();
   return ret_value;
}

#if HAVE_UNORDERED
boost::iterator_range<InEdgeIterator> OpGraph::CGetInEdges(const vertex v) const
{
   return boost::make_iterator_range(boost::in_edges(v, *this));
}
#else
OpEdgeSet OpGraph::CGetInEdges(const vertex v) const
{
   OpEdgeSet ret_value(OpGraphConstRef(this, null_deleter()));
   OpVertexSorter op_vertex_sorter(OpGraphConstRef(this, null_deleter()));
   InEdgeIterator ie, ie_end;
   for(boost::tie(ie, ie_end) = boost::in_edges(v, *this); ie != ie_end; ie++)
   {
      ret_value.insert(*ie);
   }
   return ret_value;
}
#endif

#if HAVE_UNORDERED
boost::iterator_range<OutEdgeIterator> OpGraph::CGetOutEdges(const vertex v) const
{
   return boost::make_iterator_range(boost::out_edges(v, *this));
}
#else
OpEdgeSet OpGraph::CGetOutEdges(const vertex v) const
{
   OpEdgeSet ret_value(OpGraphConstRef(this, null_deleter()));
   OpVertexSorter op_vertex_sorter(OpGraphConstRef(this, null_deleter()));
   OutEdgeIterator oe, oe_end;
   for(boost::tie(oe, oe_end) = boost::out_edges(v, *this); oe != oe_end; oe++)
   {
      ret_value.insert(*oe);
   }
   return ret_value;
}
#endif

#if HAVE_HLS_BUILT
void OpGraph::WriteDot(const std::string& file_name, const hlsConstRef HLS, const CustomSet<unsigned int> critical_paths) const
{
   const BehavioralHelperConstRef helper = CGetOpGraphInfo()->BH;
   std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory) + "/" + helper->get_function_name() + "/";
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name;
   const VertexWriterConstRef op_label_writer(new TimedOpWriter(this, HLS, critical_paths));
   const EdgeWriterConstRef op_edge_property_writer(new TimedOpEdgeWriter(this, HLS, critical_paths));
   InternalWriteDot<const TimedOpWriter, const TimedOpEdgeWriter>(full_name, op_label_writer, op_edge_property_writer);
}
#endif
