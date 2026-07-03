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
 * @file design_flow_graph.cpp
 * @brief Base class for design_flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
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
   auto v = boost::add_vertex(DesignFlowStepInfoRef(new DesignFlowStepInfo(design_flow_step, unnecessary)), *this);
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
      boost::add_edge(src, tgt, type, *this);
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

void DesignFlowGraph::writeDot(std::filesystem::path file_name) const
{
   file_name.concat(".dot");
   const DesignFlowStepWriter design_flow_step_writer(this);
   const DesignFlowEdgeWriter design_flow_edge_writer(this);
   graph_base::writeDot(file_name, design_flow_step_writer, design_flow_edge_writer);
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
   step_info->design_flow_step->writeDot(out);
   out << "]";
}

DesignFlowEdgeWriter::DesignFlowEdgeWriter(const DesignFlowGraph* g) : m_g(g)
{
}

void DesignFlowEdgeWriter::operator()(std::ostream& out, const edge_descriptor& edge) const
{
   out << "[";
   const auto source = m_g->source(edge);
   const auto target = m_g->target(edge);

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
