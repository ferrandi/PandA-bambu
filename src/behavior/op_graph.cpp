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
 * @file op_graph.cpp
 * @brief Data structures used in operations graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#include "op_graph.hpp"

#include "Parameter.hpp"
#include "allocation_information.hpp"
#include "behavioral_helper.hpp"
#include "exceptions.hpp"
#include "function_behavior.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "schedule.hpp"
#include "var_pp_functor.hpp"

#include <boost/tuple/tuple.hpp>

#include <filesystem>
#include <fstream>
#include <utility>

namespace
{
   void PrintVariablesList(std::ostream& stream, const std::string& name, const CustomSet<unsigned int> variables,
                           const BehavioralHelperConstRef behavioral_helper, const bool dotty_format)
   {
      if(variables.size())
      {
         stream << name << ":" << (dotty_format ? "\\n" : "\n");
      }

      for(const auto& variable : variables)
      {
         stream << behavioral_helper->PrintVariable(variable) << "(" << variable << ")"
                << (dotty_format ? "\\n" : "\n");
      }
   }

} // namespace

OpNodeInfo::OpNodeInfo() : node(nullptr), bb_index(0), cer(0)
{
   Initialize();
}

void OpNodeInfo::Initialize()
{
   variables.clear();
   variables.resize(static_cast<int>(VariableType::VIRTUAL) * static_cast<int>(VariableAccessType::ARG));
}

static size_t _compute_idx(VariableType variable_type, VariableAccessType access_type)
{
   return (static_cast<size_t>(variable_type) - 1) * static_cast<size_t>(VariableAccessType::ARG) +
          (static_cast<size_t>(access_type) - 1);
}

void OpNodeInfo::AddVariable(VariableType variable_type, VariableAccessType access_type, unsigned int var)
{
   THROW_ASSERT(variable_type != VariableType::UNKNOWN, "unexpected condition");
   THROW_ASSERT(access_type != VariableAccessType::UNKNOWN, "unexpected condition");
   variables.at(_compute_idx(variable_type, access_type)).insert(var);
}

const CustomSet<unsigned int>& OpNodeInfo::getVariables(const VariableType variable_type,
                                                        const VariableAccessType access_type) const
{
   THROW_ASSERT(variable_type != VariableType::UNKNOWN, "unexpected condition");
   THROW_ASSERT(access_type != VariableAccessType::UNKNOWN, "unexpected condition");
   return variables.at(_compute_idx(variable_type, access_type));
}

const std::string OpNodeInfo::GetOperation() const
{
   if(vertex_name == ENTRY)
   {
      return ENTRY;
   }
   if(vertex_name == EXIT)
   {
      return EXIT;
   }
   if(vertex_name.find("_#empty_") != std::string::npos)
   {
      return NOP;
   }
   THROW_ASSERT(node, "");
   THROW_ASSERT(GetPointer<const node_stmt>(node), "Node is not a node_stmt but a " + node->get_kind_text());
   return GetPointerS<const node_stmt>(node)->operation;
}

unsigned int OpNodeInfo::GetNodeId() const
{
   if(node)
   {
      return node->index;
   }
   else if(vertex_name == ENTRY)
   {
      return ENTRY_ID;
   }
   else if(vertex_name == EXIT)
   {
      return EXIT_ID;
   }
   THROW_UNREACHABLE("");
   return 0;
}

void OpNodeInfo::Print(std::ostream& stream, const BehavioralHelperConstRef behavioral_helper, bool dotty_format) const
{
   PrintVariablesList(stream, "source code variables", cited_variables, behavioral_helper, dotty_format);
   static const std::map<VariableType, std::string> vts = {
       {VariableType::SCALAR, "SCALARS"}, {VariableType::MEMORY, "MEMORY"}, {VariableType::VIRTUAL, "VIRTUAL"}};
   static const std::map<VariableAccessType, std::string> vats = {
       {VariableAccessType::USE, "USES"},
       {VariableAccessType::DEFINITION, "DEFINITIONS"},
       {VariableAccessType::OVER, "OVERS"},
       {VariableAccessType::ADDRESS, "ADDRS"},
   };
   for(const auto& [vt, name] : vts)
   {
      stream << name << ":" << (dotty_format ? "\\n" : "\n");
      for(const auto& [vat, aname] : vats)
      {
         PrintVariablesList(stream, aname, getVariables(vt, vat), behavioral_helper, dotty_format);
      }
   }
}

void OpGraphInfo::clear()
{
   entry_vertex = gc_null_vertex();
   exit_vertex = gc_null_vertex();
   ir_node_to_operation.clear();
   SSA2Def.clear();
}

OpGraphsCollection::OpGraphsCollection(const OpGraphInfo& _info)
    : graphs_collection<OpNodeInfo, OpEdgeInfo, OpGraphInfo>(_info), operations(this)
{
}

OpGraphsCollection::vertex_descriptor OpGraphsCollection::AddVertex(const OpNodeInfo& info)
{
   const auto new_vertex = graphs_collection<OpNodeInfo, OpEdgeInfo, OpGraphInfo>::AddVertex(info);
   operations.insert(new_vertex);
   return new_vertex;
}

void OpGraphsCollection::RemoveVertex(vertex_descriptor v)
{
   operations.erase(v);
   graphs_collection<OpNodeInfo, OpEdgeInfo, OpGraphInfo>::RemoveVertex(v);
}

OpVertexSet OpGraphsCollection::CGetOperations() const
{
   return operations;
}

void OpGraphsCollection::Clear()
{
   operations.clear();
   GetGraphInfo().clear();
   Base::clear();
}

#if HAVE_UNORDERED
OpVertexSet::OpVertexSet(const OpGraphsCollection*) : CustomUnorderedSet<OpGraph::vertex_descriptor>()
{
}

OpEdgeSet::OpEdgeSet(const OpGraphsCollection*) : CustomUnorderedSet<OpGraph::edge_descriptor>()
{
}

#else
OpVertexSorter::OpVertexSorter(const OpGraphsCollection* _op_graph) : op_graph(_op_graph)
{
}

bool OpVertexSorter::operator()(OpGraph::vertex_descriptor x, OpGraph::vertex_descriptor y) const
{
   return op_graph->CGetNodeInfo(x).vertex_name < op_graph->CGetNodeInfo(y).vertex_name;
}

OpVertexSet::OpVertexSet(const OpGraphsCollection* _op_graph)
    : std::set<OpGraph::vertex_descriptor, OpVertexSorter>(OpVertexSorter(_op_graph))
{
}

OpEdgeSorter::OpEdgeSorter(const OpGraphsCollection* _op_graph) : op_graph(_op_graph)
{
}

bool OpEdgeSorter::operator()(const OpGraph::edge_descriptor& x, const OpGraph::edge_descriptor& y) const
{
   if(x != y)
   {
      return op_graph->CGetNodeInfo(op_graph->source(x)).vertex_name <
             op_graph->CGetNodeInfo(op_graph->source(y)).vertex_name;
   }
   return op_graph->CGetNodeInfo(op_graph->target(x)).vertex_name <
          op_graph->CGetNodeInfo(op_graph->target(y)).vertex_name;
}

OpEdgeSet::OpEdgeSet(const OpGraphsCollection* _op_graph)
    : std::set<OpGraph::edge_descriptor, OpEdgeSorter>(OpEdgeSorter(_op_graph))
{
}
#endif

OpGraph::OpGraph(const OpGraphsCollection& _op_graphs_collection, int _selector)
    : graph<OpGraphsCollection>(_op_graphs_collection, _selector)
{
}

OpGraph::OpGraph(const OpGraphsCollection& _op_graphs_collection, int _selector,
                 const CustomUnorderedSet<vertex_descriptor>& _sub)
    : graph<OpGraphsCollection>(_op_graphs_collection, _selector, _sub)
{
}

void OpGraph::writeDot(const std::filesystem::path& file_name, const int detail_level) const
{
   OpVertexWriter op_label_writer(*this, detail_level);
   OpEdgeWriter op_edge_property_writer(*this);
   graph::writeDot(file_name, op_label_writer, op_edge_property_writer);
}

CustomUnorderedMap<OpGraph::vertex_descriptor, OpVertexSet> OpGraph::GetSrcVertices(const OpVertexSet& toCheck,
                                                                                    int edgeType) const
{
   CustomUnorderedMap<vertex_descriptor, OpVertexSet> retVal;
   for(const auto v : toCheck)
   {
      for(const auto& ie : in_edges(v))
      {
         int origEdgeType = GetSelector(ie);
         if((edgeType & origEdgeType) != 0)
         {
            const auto src = source(ie);
            if(retVal.find(src) == retVal.end())
            {
               retVal.insert(std::make_pair(src, OpVertexSet(&m_collection)));
            }
            retVal.find(src)->second.insert(v);
         }
      }
   }
   return retVal;
}

OpVertexSet OpGraph::CGetOperations() const
{
   return m_collection.CGetOperations();
}

#if HAVE_UNORDERED
auto OpGraph::CGetInEdges(const vertex_descriptor v) const
{
   return in_edges(v);
}
#else
OpEdgeSet OpGraph::CGetInEdges(const vertex_descriptor v) const
{
   OpEdgeSet ret_value(&m_collection);
   for(const auto& ie : in_edges(v))
   {
      ret_value.insert(ie);
   }
   return ret_value;
}
#endif

#if HAVE_UNORDERED
auto OpGraph::CGetOutEdges(const vertex_descriptor v) const
{
   return out_edges(v);
}
#else
OpEdgeSet OpGraph::CGetOutEdges(const vertex_descriptor v) const
{
   OpEdgeSet ret_value(&m_collection);
   for(const auto& oe : out_edges(v))
   {
      ret_value.insert(oe);
   }
   return ret_value;
}
#endif

OpVertexWriter::OpVertexWriter(const OpGraph& operation_graph, const int _detail_level)
    : VertexWriter(operation_graph, _detail_level)
{
}

void OpVertexWriter::operator()(std::ostream& out, OpGraph::vertex_descriptor v) const
{
   const auto& node_info = printing_graph.CGetNodeInfo(v);
   if(node_info.node_type & TYPE_MULTIIF)
   {
      out << "[color=red,shape=diamond,";
   }
   else if(node_info.node_type & TYPE_LOAD)
   {
      out << "[color=green,shape=box,";
   }
   else if(node_info.node_type & TYPE_STORE)
   {
      out << "[color=red,shape=box,";
   }
   else if(node_info.node_type & TYPE_EXTERNAL)
   {
      out << "[color=green,shape=box,";
   }
   else if(node_info.node_type & TYPE_MEMCPY)
   {
      out << "[color=burlywood,shape=diamond,";
   }
   else if(node_info.node_type & TYPE_GOTO)
   {
      out << "[color=yellow,shape=box,";
   }
   else if(node_info.node_type & TYPE_ASSIGN)
   {
      out << "[color=burlywood,shape=box,";
   }
   else if(node_info.node_type & (TYPE_ENTRY | TYPE_EXIT))
   {
      out << "[color=blue,shape=Msquare,";
   }
   else
   {
      out << "[shape=ellipse,";
   }
   out << "label=\"" << node_info.vertex_name;
#if HAVE_HLS_BUILT
   out << " - " << node_info.GetOperation();
#endif
   if(node_info.node)
   {
      const auto& BH = printing_graph.CGetGraphInfo().BH;
      out << "\\n";
      out << BH->print_vertex(printing_graph, v, std::make_unique<std_var_pp_functor>(BH), true);
      if(detail_level >= 1)
      {
         out << "\\n";
         node_info.Print(out, BH, true);
      }
   }
   out << "\"]";
}

OpEdgeWriter::OpEdgeWriter(const OpGraph& operation_graph) : EdgeWriter(operation_graph, 0)
{
}

void OpEdgeWriter::operator()(std::ostream& out, const OpGraph::edge_descriptor& e) const
{
   if((FB_CFG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=gold";
   }
   else if((CFG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[fontcolor=red3";
   }
   else if((FB_CDG_SELECTOR)&selector & printing_graph.GetSelector(e) &&
           FB_DFG_SELECTOR & selector & printing_graph.GetSelector(e))
   {
      out << "[color=gold,style=dotted";
   }
   else if((FB_CDG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=gold";
   }
   else if(((CDG_SELECTOR)&selector & printing_graph.GetSelector(e)) &&
           ((DFG_SELECTOR)&selector & printing_graph.GetSelector(e)))
   {
      out << "[color=red3,style=dotted";
   }
   else if((CDG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=red3";
   }
   else if((FB_DFG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=lightblue";
   }
   else if((DFG_SCA_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=blue, style=dotted";
   }
   else if((DFG_AGG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=blue";
   }
   else if((FB_ADG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=lawngreen";
   }
   else if((ADG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=green4";
   }
   else if((FB_ODG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=lawngreen";
   }
   else if((ODG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=green4";
   }
   else if((CDG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=red3";
   }
   else if((DFG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=blue";
   }
   else if((FB_CDG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=gold";
   }
   else if((FLG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=red3";
   }
   else if((DEBUG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=red3";
   }
   else if((CSG_SELECTOR)&selector & printing_graph.GetSelector(e) &&
           DFG_SELECTOR & selector & printing_graph.GetSelector(e))
   {
      out << "[color=pink,style=dotted";
   }
   else if((CSG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=pink";
   }
   else if((FB_FLG_SELECTOR)&selector & printing_graph.GetSelector(e))
   {
      out << "[color=pink";
   }

   const auto& edge_info = printing_graph.CGetEdgeInfo(e);

   out << ",label=\"";
   const auto& BH = printing_graph.CGetGraphInfo().BH;
   if(printing_graph.GetSelector(e) & selector & FCDG_SELECTOR)
   {
      if(edge_info.IfElseIf())
      {
         out << edge_info.PrintLabels(CDG_SELECTOR, BH);
      }
   }
   if(printing_graph.GetSelector(e) & selector & FCFG_SELECTOR)
   {
      if(edge_info.IfElseIf())
      {
         out << edge_info.PrintLabels(CFG_SELECTOR, BH);
      }
   }
   if(printing_graph.GetSelector(e) & selector & FDFG_SELECTOR)
   {
      out << edge_info.PrintLabels(DFG_SELECTOR, BH);
   }
   if(printing_graph.GetSelector(e) & selector & FADG_SELECTOR)
   {
      out << edge_info.PrintLabels(ADG_SELECTOR, BH);
   }
   if(printing_graph.GetSelector(e) & selector & FODG_SELECTOR)
   {
      out << edge_info.PrintLabels(ODG_SELECTOR, BH);
   }
   out << "\"]";
}

#if HAVE_HLS_BUILT
void OpGraph::writeDot(const std::filesystem::path& file_name, const hlsConstRef& HLS,
                       const CustomSet<unsigned int>& critical_paths) const
{
   TimedOpVertexWriter op_label_writer(*this, HLS, critical_paths);
   TimedOpEdgeWriter op_edge_property_writer(*this, HLS, critical_paths);
   graph::writeDot(file_name, op_label_writer, op_edge_property_writer);
}

TimedOpVertexWriter::TimedOpVertexWriter(const OpGraph& op_graph, const hlsConstRef _HLS,
                                         const CustomSet<unsigned int> _critical_paths)
    : OpVertexWriter(op_graph, 0), HLS(_HLS), critical_paths(_critical_paths)
{
}

void TimedOpVertexWriter::operator()(std::ostream& out, OpGraph::vertex_descriptor v) const
{
   const auto schedule = HLS->Rsch;
   const unsigned node_id = printing_graph.CGetNodeInfo(v).GetNodeId();
   if(critical_paths.find(node_id) != critical_paths.end())
   {
      out << "[color=red,";
   }
   else
   {
      out << "[";
   }
   out << "label=\"";
   out << "[" << schedule->GetStartingTime(node_id) << "---" << schedule->GetEndingTime(node_id) << "]";
   out << " - " << printing_graph.CGetNodeInfo(v).vertex_name;
   out << " - " << HLS->allocation_information->get_fu_name(HLS->allocation_information->GetFuType(node_id)).first;

   if(printing_graph.CGetNodeInfo(v).node)
   {
      const auto& BH = printing_graph.CGetGraphInfo().BH;
      out << "\\n";
      out << BH->print_vertex(printing_graph, v, std::make_unique<std_var_pp_functor>(BH), true);
      if(detail_level >= 1)
      {
         out << "\\n";
         printing_graph.CGetNodeInfo(v).Print(out, BH, true);
      }
   }
   out << "\"]";
}

TimedOpEdgeWriter::TimedOpEdgeWriter(const OpGraph& _operation_graph, const hlsConstRef _HLS,
                                     CustomSet<unsigned int> _critical_paths)
    : OpEdgeWriter(_operation_graph), HLS(_HLS), critical_paths(std::move(_critical_paths))
{
}

void TimedOpEdgeWriter::operator()(std::ostream& out, const OpGraph::edge_descriptor& e) const
{
   const auto source = printing_graph.source(e);
   const auto target = printing_graph.target(e);
   const auto source_id = printing_graph.CGetNodeInfo(source).GetNodeId();
   const auto target_id = printing_graph.CGetNodeInfo(target).GetNodeId();
   out << "[";
   if(critical_paths.find(source_id) != critical_paths.end() && critical_paths.find(target_id) != critical_paths.end())
   {
      out << "color=red,";
   }
   bool isDfgEdge = printing_graph.GetSelector(e) & (DFG_SCA_SELECTOR | FB_DFG_SCA_SELECTOR);
   auto connection_type = isDfgEdge ? HLS->allocation_information->GetConnectionTime(source_id, target_id) : 0;

   out << "label=" << connection_type;

   out << "]";
}
#endif

OpGraph::vertex_descriptor getDefOp(const OpGraph& data, unsigned int var)
{
   auto it = data.CGetGraphInfo().SSA2Def.find(var);
   THROW_ASSERT(it != data.CGetGraphInfo().SSA2Def.end(), "variable not defined");
   return it->second;
}
