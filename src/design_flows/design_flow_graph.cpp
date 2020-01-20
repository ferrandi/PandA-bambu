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
 * @file design_flow_graph.cpp
 * @brief Base class for design_flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "design_flow_graph.hpp"
#include "Parameter.hpp"                      // for OPT_dot_directory
#include "custom_map.hpp"                     // for _Rb_tree_const_iter...
#include "design_flow_step.hpp"               // for DesignFlowStep_Status
#include "exceptions.hpp"                     // for THROW_UNREACHABLE
#include <boost/filesystem/operations.hpp>    // for create_directories
#include <boost/graph/adjacency_list.hpp>     // for adjacency_list, source
#include <boost/graph/filtered_graph.hpp>     // for source, target
#include <boost/iterator/iterator_facade.hpp> // for operator!=, operator++
#include <ostream>                            // for operator<<, ostream
#include <utility>                            // for pair

DesignFlowStepInfo::DesignFlowStepInfo(const DesignFlowStepRef _design_flow_step, const bool _unnecessary) : design_flow_step(_design_flow_step), status(_unnecessary ? DesignFlowStep_Status::UNNECESSARY : DesignFlowStep_Status::UNEXECUTED)
{
}

DesignFlowDependenceInfo::DesignFlowDependenceInfo() = default;

DesignFlowDependenceInfo::~DesignFlowDependenceInfo() = default;

DesignFlowGraphsCollection::DesignFlowGraphsCollection(const ParameterConstRef _parameters) : graphs_collection(GraphInfoRef(new DesignFlowGraphInfo()), _parameters)
{
}

DesignFlowGraphsCollection::~DesignFlowGraphsCollection() = default;

vertex DesignFlowGraphsCollection::AddDesignFlowStep(const DesignFlowStepRef design_flow_step, const bool unnecessary)
{
   const DesignFlowStepInfoRef info(new DesignFlowStepInfo(design_flow_step, unnecessary));
   const vertex new_vertex = AddVertex(RefcountCast<NodeInfo>(info));
   signature_to_vertex[design_flow_step->GetSignature()] = new_vertex;
   return new_vertex;
}

vertex DesignFlowGraphsCollection::GetDesignFlowStep(const std::string& signature) const
{
   if(signature_to_vertex.find(signature) != signature_to_vertex.end())
   {
      return signature_to_vertex.find(signature)->second;
   }
   else
   {
      return NULL_VERTEX;
   }
}

const int DesignFlowGraph::DEPENDENCE_SELECTOR = 1;

const int DesignFlowGraph::PRECEDENCE_SELECTOR = 2;

const int DesignFlowGraph::AUX_SELECTOR = 4;

const int DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR = 8;

DesignFlowGraph::DesignFlowGraph(const DesignFlowGraphsCollectionRef design_flow_graphs_collection, const int _selector) : graph(design_flow_graphs_collection.get(), _selector)
{
}

DesignFlowGraph::~DesignFlowGraph() = default;

vertex DesignFlowGraph::GetDesignFlowStep(const std::string& signature) const
{
   return dynamic_cast<DesignFlowGraphsCollection*>(collection)->GetDesignFlowStep(signature);
}

void DesignFlowGraph::WriteDot(const std::string& file_name, const int) const
{
   const std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory) + "/design_flow/";
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name + ".dot";
   VertexWriterConstRef design_flow_step_writer(new DesignFlowStepWriter(this));
   EdgeWriterConstRef design_flow_edge_writer(new DesignFlowEdgeWriter(this));
   InternalWriteDot<const DesignFlowStepWriter, const DesignFlowEdgeWriter>(full_name, design_flow_step_writer, design_flow_edge_writer);
}

#ifndef NDEBUG
void DesignFlowGraph::WriteDot(const std::string& file_name, const CustomMap<size_t, CustomMap<vertex, DesignFlowStep_Status>>& vertex_history, const CustomMap<size_t, CustomUnorderedMapStable<EdgeDescriptor, int>>& edge_history,
                               const CustomMap<vertex, std::string>& vertex_names, const size_t writing_step_counter) const
{
   const std::string output_directory = collection->parameters->getOption<std::string>(OPT_dot_directory) + "/design_flow/";
   if(!boost::filesystem::exists(output_directory))
      boost::filesystem::create_directories(output_directory);
   const std::string full_name = output_directory + file_name + ".dot";
   VertexWriterConstRef design_flow_step_writer(new DesignFlowStepWriter(this, vertex_history.find(writing_step_counter)->second, vertex_names));
   EdgeWriterConstRef design_flow_edge_writer(new DesignFlowEdgeWriter(this, vertex_history.find(writing_step_counter)->second, edge_history.find(writing_step_counter)->second));
   InternalWriteDot<const DesignFlowStepWriter, const DesignFlowEdgeWriter>(full_name, design_flow_step_writer, design_flow_edge_writer);
}
#endif

DesignFlowStepWriter::DesignFlowStepWriter(const DesignFlowGraph* design_flow_graph, const CustomMap<vertex, DesignFlowStep_Status>& _vertex_history, const CustomMap<vertex, std::string>& _actor_names, const int _detail_level)
    : VertexWriter(design_flow_graph, _detail_level), vertex_history(_vertex_history), actor_names(_actor_names)
{
}

DesignFlowStepWriter::~DesignFlowStepWriter() = default;

void DesignFlowStepWriter::operator()(std::ostream& out, const vertex& v) const
{
   out << "[";
   const DesignFlowStepInfoConstRef design_flow_step_info = dynamic_cast<const DesignFlowGraph*>(printing_graph)->CGetDesignFlowStepInfo(v);
   if(vertex_history.size())
   {
      if(vertex_history.find(v) == vertex_history.end())
      {
         out << "color=white,label=\"\"";
      }
      else
      {
         switch(vertex_history.find(v)->second)
         {
            case DesignFlowStep_Status::ABORTED:
            {
               out << " style=filled, fillcolor=red, fontcolor=white,";
               break;
            }
            case DesignFlowStep_Status::EMPTY:
            {
               out << " style=filled, fillcolor=darkgreen, fontcolor=white, ";
               break;
            }
            case DesignFlowStep_Status::NONEXISTENT:
            {
               THROW_UNREACHABLE("Status of a step is nonexitent");
               break;
            }
            case DesignFlowStep_Status::SKIPPED:
            {
               out << " style=filled, fillcolor=black, fontcolor=white,";
               break;
            }
            case DesignFlowStep_Status::SUCCESS:
            {
               out << " style=filled, fillcolor=darkgreen, fontcolor=white, ";
               break;
            }
            case DesignFlowStep_Status::UNCHANGED:
            {
               out << " style=filled, fillcolor=gold, fontcolor=white, ";
               break;
            }
            case DesignFlowStep_Status::UNEXECUTED:
            {
               break;
            }
            case DesignFlowStep_Status::UNNECESSARY:
            {
               out << "style=dashed,";
               break;
            }
            default:
            {
               THROW_UNREACHABLE("");
            }
         }
         out << "label=\"" << actor_names.find(v)->second << "\"";
      }
   }
   else
   {
      if(design_flow_step_info->design_flow_step->IsComposed())
      {
         out << " shape=box3d,";
      }
      switch(design_flow_step_info->status)
      {
         case DesignFlowStep_Status::ABORTED:
         {
            out << " style=filled, fillcolor=red, fontcolor=white,";
            break;
         }
         case DesignFlowStep_Status::EMPTY:
         {
            out << " style=filled, fillcolor=darkgreen, fontcolor=white, ";
            break;
         }
         case DesignFlowStep_Status::NONEXISTENT:
         {
            THROW_UNREACHABLE("Status of a step is nonexitent");
            break;
         }
         case DesignFlowStep_Status::SKIPPED:
         {
            out << " style=filled, fillcolor=black, fontcolor=white,";
            break;
         }
         case DesignFlowStep_Status::SUCCESS:
         {
            out << " style=filled, fillcolor=darkgreen, fontcolor=white, ";
            break;
         }
         case DesignFlowStep_Status::UNCHANGED:
         {
            out << " style=filled, fillcolor=gold, fontcolor=white, ";
            break;
         }
         case DesignFlowStep_Status::UNEXECUTED:
         {
            break;
         }
         case DesignFlowStep_Status::UNNECESSARY:
         {
            out << "style=dashed,";
            break;
         }
         default:
         {
            THROW_UNREACHABLE("");
         }
      }
      design_flow_step_info->design_flow_step->WriteDot(out);
   }
   out << "]";
}

DesignFlowEdgeWriter::DesignFlowEdgeWriter(const DesignFlowGraph* design_flow_graph, const CustomMap<vertex, DesignFlowStep_Status>& _vertex_history, const CustomUnorderedMapStable<EdgeDescriptor, int>& _edge_history, const int _detail_level)
    : EdgeWriter(design_flow_graph, _detail_level), vertex_history(_vertex_history), edge_history(_edge_history)
{
}

void DesignFlowEdgeWriter::operator()(std::ostream& out, const EdgeDescriptor& edge) const
{
   out << "[";
   const vertex source = boost::source(edge, *printing_graph);
   const vertex target = boost::target(edge, *printing_graph);
   if(edge_history.size())
   {
      if(edge_history.find(edge) == edge_history.end())
      {
         out << "color=white";
      }
      else
      {
         const DesignFlowStep_Status source_status = vertex_history.find(source)->second;
         const DesignFlowStep_Status target_status = vertex_history.find(target)->second;
         const bool source_executed = source_status == DesignFlowStep_Status::EMPTY or source_status == DesignFlowStep_Status::SKIPPED or source_status == DesignFlowStep_Status::SUCCESS or source_status == DesignFlowStep_Status::UNCHANGED;
         const bool target_executed = target_status == DesignFlowStep_Status::EMPTY or target_status == DesignFlowStep_Status::SKIPPED or target_status == DesignFlowStep_Status::SUCCESS or target_status == DesignFlowStep_Status::UNCHANGED;
         const bool source_unnecessary = source_status == DesignFlowStep_Status::UNNECESSARY;
         const bool target_unnecessary = target_status == DesignFlowStep_Status::UNNECESSARY;
         const int edge_selector = edge_history.find(edge)->second;
         if(DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR & selector & edge_selector)
         {
            out << "color=red3,";
         }
         else if(source_executed and target_executed)
         {
            out << "color=darkgreen, ";
         }
         if((DesignFlowGraph::PRECEDENCE_SELECTOR & selector & edge_selector) or target_unnecessary or source_unnecessary)
         {
            out << "style=dashed";
         }
      }
   }
   else
   {
      const DesignFlowStepInfoConstRef source_info = dynamic_cast<const DesignFlowGraph*>(printing_graph)->CGetDesignFlowStepInfo(source);
      const DesignFlowStepInfoConstRef target_info = dynamic_cast<const DesignFlowGraph*>(printing_graph)->CGetDesignFlowStepInfo(target);
      const bool source_executed =
          source_info->status == DesignFlowStep_Status::EMPTY or source_info->status == DesignFlowStep_Status::SKIPPED or source_info->status == DesignFlowStep_Status::SUCCESS or source_info->status == DesignFlowStep_Status::UNCHANGED;
      const bool target_executed =
          target_info->status == DesignFlowStep_Status::EMPTY or target_info->status == DesignFlowStep_Status::SKIPPED or target_info->status == DesignFlowStep_Status::SUCCESS or target_info->status == DesignFlowStep_Status::UNCHANGED;
      const bool source_unnecessary = source_info->status == DesignFlowStep_Status::UNNECESSARY;
      const bool target_unnecessary = target_info->status == DesignFlowStep_Status::UNNECESSARY;
      if(DesignFlowGraph::DEPENDENCE_FEEDBACK_SELECTOR & selector & printing_graph->GetSelector(edge))
      {
         out << "color=red3,";
      }
      else if(source_executed and target_executed)
      {
         out << "color=darkgreen, ";
      }
      if((DesignFlowGraph::PRECEDENCE_SELECTOR & selector & printing_graph->GetSelector(edge)) or target_unnecessary or source_unnecessary)
      {
         out << "style=dashed";
      }
   }
   out << "]";
}
