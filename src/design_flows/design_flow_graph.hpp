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
 * @file design_flow_graph.hpp
 * @brief Classes to describe design flow graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DESIGN_FLOW_GRAPH_HPP
#define DESIGN_FLOW_GRAPH_HPP
#include "custom_map.hpp"
#include "design_flow_step.hpp"
#include "edge_info.hpp"
#include "graph.hpp"
#include "graph_info.hpp"
#include "node_info.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <iosfwd>
#include <string>

CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(DesignFlowStepInfo);
CONSTREF_FORWARD_DECL(DesignFlowInfo);
REF_FORWARD_DECL(DesignFlowStepInfo);
REF_FORWARD_DECL(DesignFlowInfo);
enum class DesignFlowStep_Status;
class SdfGraph;

class DesignFlowStepInfo
{
 public:
   /// The step corresponding to a vertex
   const DesignFlowStepRef design_flow_step;

   /// Status of a step
   DesignFlowStep_Status status;

   DesignFlowStepInfo(const DesignFlowStepRef& _design_flow_step, const bool unnecessary);
};

using DesignFlowEdge = unsigned char;

class DesignFlowInfo
{
 public:
   using vertex_descriptor =
       boost::adjacency_list_traits<boost::vecS, boost::vecS, boost::bidirectionalS>::vertex_descriptor;
   /// The entry vertex of the graph
   vertex_descriptor entry;

   /// The exit vertex of the graph
   vertex_descriptor exit;
};

class DesignFlowGraph
    : public graph_base<boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, DesignFlowStepInfoRef,
                                              DesignFlowEdge, DesignFlowInfoRef>>
{
 public:
   using self = DesignFlowGraph;
   using graph_t = graph_base<boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                                                    DesignFlowStepInfoRef, DesignFlowEdge, DesignFlowInfoRef>>;
   using vertex_descriptor = self::vertex_descriptor;
   using edge_descriptor = self::edge_descriptor;

   enum EdgeType : DesignFlowEdge
   {
      DEPENDENCE = 1,
      PRECEDENCE = 2,
      AUXILIARY = 4,
      FEEDBACK = 8
   };

   /**
    * Add a design step
    * @param design_flow_step is the step to be added
    * @param unnecessary specifiy is the step is necessary or not
    */
   vertex_descriptor AddDesignFlowStep(const DesignFlowStepRef& design_flow_step, bool unnecessary);

   /**
    * Return the vertex associated with a design step if exists, NULL_VERTEX otherwise
    * @param signature is the signature of the design step
    */
   vertex_descriptor GetDesignFlowStep(DesignFlowStep::signature_t signature) const;

   void AddDesignFlowDependence(vertex_descriptor src, vertex_descriptor tgt, DesignFlowEdge type);

   DesignFlowEdge AddType(edge_descriptor e, DesignFlowEdge type);

   DesignFlowEdge RemoveType(edge_descriptor e, DesignFlowEdge type);

   /**
    * Write this graph in dot format
    * @param file_name is the file where the graph has to be printed
    */
   void WriteDot(std::filesystem::path file_name) const;

 private:
   using graph_t::AddEdge;
   using graph_t::AddVertex;
   using graph_t::WriteDot;

   CustomUnorderedMap<DesignFlowStep::signature_t, vertex_descriptor> signature_to_vertex;
};
using DesignFlowGraphRef = refcount<DesignFlowGraph>;
using DesignFlowGraphConstRef = refcount<const DesignFlowGraph>;

namespace std
{
   template <>
   struct hash<DesignFlowGraph::edge_descriptor>
   {
      size_t operator()(DesignFlowGraph::edge_descriptor edge) const
      {
         size_t hash_value = 0;
         boost::hash_combine(hash_value, edge.m_source);
         boost::hash_combine(hash_value, edge.m_target);
         return hash_value;
      }
   };
} // namespace std

template <typename H>
H AbslHashValue(const H& h, const DesignFlowGraph::edge_descriptor& m)
{
   return H::combine(h, m.m_source, m.m_target);
}

/**
 * Functor used to write the content of the design flow step to dotty file
 */
class DesignFlowStepWriter
{
 public:
   using vertex_descriptor = DesignFlowGraph::vertex_descriptor;

   /**
    * Constructor
    * @param g is the graph to be printed
    * @param vertex_history are the vertices which have to be printed
    */
   DesignFlowStepWriter(const DesignFlowGraph* g);

   void operator()(std::ostream& out, const vertex_descriptor& v) const;

 private:
   const DesignFlowGraph* m_g;
};

/**
 * Functor used to write the content of the design flow edge to dotty file
 */
class DesignFlowEdgeWriter
{
 public:
   using edge_descriptor = DesignFlowGraph::edge_descriptor;

   /**
    * Constructor
    * @param design_flow_graph is the graph to be printed
    * @param vertex_history are the vertices which have to be printed
    * @param edge_history are the edges which have to be printed
    */
   DesignFlowEdgeWriter(const DesignFlowGraph* g);

   void operator()(std::ostream& out, const edge_descriptor& edge) const;

 private:
   const DesignFlowGraph* m_g;
};
#endif
