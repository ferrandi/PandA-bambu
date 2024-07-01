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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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

#include "Parameter.hpp"
#include "custom_map.hpp"
#include "design_flow_step.hpp"
#include "exceptions.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <filesystem>
#include <ostream>
#include <utility>

DesignFlowStepInfo::DesignFlowStepInfo(const DesignFlowStepRef& _design_flow_step, const bool _unnecessary)
    : design_flow_step(_design_flow_step),
      status(_unnecessary ? DesignFlowStep_Status::UNNECESSARY : DesignFlowStep_Status::UNEXECUTED)
{
}

DesignFlowGraph::vertex_descriptor DesignFlowGraph::AddDesignFlowStep(const DesignFlowStepRef& design_flow_step,
                                                                      bool unnecessary)
{
   THROW_ASSERT(design_flow_step, "Design flow step pointer must be initialized");
   auto v = graph_t::AddVertex(DesignFlowStepInfoRef(new DesignFlowStepInfo(design_flow_step, unnecessary)));
   signature_to_vertex[design_flow_step->GetSignature()] = v;
   return v;
}

DesignFlowGraph::vertex_descriptor DesignFlowGraph::GetDesignFlowStep(DesignFlowStep::signature_t signature) const
{
   auto it = signature_to_vertex.find(signature);
   return it != signature_to_vertex.end() ? it->second : DesignFlowGraph::null_vertex();
}

void DesignFlowGraph::AddDesignFlowDependence(vertex_descriptor src, vertex_descriptor tgt, DesignFlowEdge type)
{
   const auto [e, found] = boost::edge(src, tgt, *this);
   if(found)
   {
      AddType(e, type);
   }
   else
   {
      graph_t::AddEdge(src, tgt, type);
   }
}

DesignFlowEdge DesignFlowGraph::AddType(edge_descriptor e, DesignFlowEdge type)
{
   return GetEdgeInfo(e) |= type;
}

DesignFlowEdge DesignFlowGraph::RemoveType(edge_descriptor e, DesignFlowEdge type)
{
   auto& etype = GetEdgeInfo(e);
   return etype = static_cast<DesignFlowEdge>(etype & ~type);
}

void DesignFlowGraph::WriteDot(std::filesystem::path file_name) const
{
   std::filesystem::create_directories(file_name.parent_path());
   file_name.concat(".dot");
   const DesignFlowStepWriter design_flow_step_writer(this);
   const DesignFlowEdgeWriter design_flow_edge_writer(this);
   graph_t::WriteDot(file_name, design_flow_step_writer, design_flow_edge_writer);
}

DesignFlowStepWriter::DesignFlowStepWriter(const DesignFlowGraph* g) : m_g(g)
{
}

void DesignFlowStepWriter::operator()(std::ostream& out, const vertex_descriptor& v) const
{
   out << "[";
   const auto step_info = m_g->CGetNodeInfo(v);

   if(step_info->design_flow_step->IsComposed())
   {
      out << " shape=box3d,";
   }
   switch(step_info->status)
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
   step_info->design_flow_step->WriteDot(out);
   out << "]";
}

DesignFlowEdgeWriter::DesignFlowEdgeWriter(const DesignFlowGraph* g) : m_g(g)
{
}

void DesignFlowEdgeWriter::operator()(std::ostream& out, const edge_descriptor& edge) const
{
   out << "[";
   const auto source = boost::source(edge, *m_g);
   const auto target = boost::target(edge, *m_g);

   const auto source_info = m_g->CGetNodeInfo(source);
   const auto target_info = m_g->CGetNodeInfo(target);
   const bool source_executed =
       source_info->status == DesignFlowStep_Status::EMPTY || source_info->status == DesignFlowStep_Status::SKIPPED ||
       source_info->status == DesignFlowStep_Status::SUCCESS || source_info->status == DesignFlowStep_Status::UNCHANGED;
   const bool target_executed =
       target_info->status == DesignFlowStep_Status::EMPTY || target_info->status == DesignFlowStep_Status::SKIPPED ||
       target_info->status == DesignFlowStep_Status::SUCCESS || target_info->status == DesignFlowStep_Status::UNCHANGED;
   const bool source_unnecessary = source_info->status == DesignFlowStep_Status::UNNECESSARY;
   const bool target_unnecessary = target_info->status == DesignFlowStep_Status::UNNECESSARY;
   if(DesignFlowGraph::FEEDBACK == m_g->CGetEdgeInfo(edge))
   {
      out << "color=red3,";
   }
   else if(source_executed && target_executed)
   {
      out << "color=darkgreen, ";
   }
   if((DesignFlowGraph::PRECEDENCE == m_g->CGetEdgeInfo(edge)) || target_unnecessary || source_unnecessary)
   {
      out << "style=dashed";
   }

   out << "]";
}
