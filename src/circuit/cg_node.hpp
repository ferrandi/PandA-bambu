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
 * @file cg_node.hpp
 * @brief Node, edge and graph description of the graph associated with a structural description.
 *
 * @author Matteo Barbati <matteo.mbarbati@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef CG_NODE_HPP
#define CG_NODE_HPP

#include "edge_info.hpp"       // for EdgeInfo
#include "graph.hpp"           // for graph, vertex
#include "graph_info.hpp"      // for GraphInfo
#include "refcount.hpp"        // for REF_FORWARD_DECL
#include "typed_node_info.hpp" // for TypedNodeInfo
#include <ostream>             // for operator<<, ostream
#include <string>              // for operator<<, string
#include <utility>

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(structural_object);
//@}

/// Data line selector
#define DATA_SELECTOR 1
/// Clock line selector
#define CLOCK_SELECTOR 2
/// Channel line  selector
#define CHANNEL_SELECTOR 4
/// All lines selector
#define ALL_LINES_SELECTOR DATA_SELECTOR | CLOCK_SELECTOR | CHANNEL_SELECTOR
/// All but clock lines selector
#define PURE_DATA_SELECTOR DATA_SELECTOR | CHANNEL_SELECTOR

/**
 * Information associated with a circuit graph node.
 */
struct cg_node_info : public TypedNodeInfo
{
   /**
    * Definition of the internal graph associated with the node.
    */
   structural_objectRef reference;

   bool is_critical;
   /**
    * Print the information associated with the current node of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream& os, int detail_level = 0) const override
   {
      TypedNodeInfo::print(os, detail_level);
      os << " " << reference << std::endl;
   }

   /// Constructor
   cg_node_info() : TypedNodeInfo(), is_critical(false)
   {
   }
};

#define GET_REFERENCE(data, node_index) Cget_node_info<cg_node_info>(node_index, *(data))->reference

#define GET_CRITICAL(data, node_index) Cget_node_info<cg_node_info>(node_index, *(data))->is_critical

/**
 * Information associated with a circuit graph edge.
 */
struct cg_edge_info : public EdgeInfo
{
   /// port from which the edge is generated
   structural_objectRef from_port;
   /// destination port/channel to the edge is attached
   structural_objectRef to_port;
   /// flag to check if the edge is involved into the critical path
   bool is_critical;
   /**
    * Print the information associated with the current node of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const;

   /// Constructor
   cg_edge_info() : is_critical(false)
   {
   }
};

/**
 * Helper macro returning the from port
 * @param data is the graph.
 * @param edge_index is the index of the edge.
 */
#define GET_FROM_PORT(data, edge_index) Cget_edge_info<cg_edge_info>(edge_index, *(data))->from_port

/**
 * Helper macro returning if the edge is timing critical
 * @param data is the graph.
 * @param edge_index is the index of the edge.
 */
#define GET_EDGE_CRITICAL(data, edge_index) Cget_node_info<cg_edge_info>(edge_index, *(data))->is_critical

/**
 * Add a from port to the edge.
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 * @param from is the from port.
 */
#define EDGE_ADD_FROM_PORT(data, edge_index, from) get_edge_info<cg_edge_info>(edge_index, *(data))->from_port = from

/**
 * Helper macro returning the to port/channel
 * @param data is the graph.
 * @param edge_index is the index of the edge.
 */
#define GET_TO_PORT(data, edge_index) Cget_edge_info<cg_edge_info>(edge_index, *(data))->to_port

/**
 * Add a to port/channel to the edge.
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 * @param from is the from port.
 */
#define EDGE_ADD_TO_PORT(data, edge_index, _to) get_edge_info<cg_edge_info>(edge_index, *(data))->to_port = _to

/**
 * Set the edge as critical
 * @param data is the graph.
 * @param edge_index is the index of the cdfg edge.
 */
#define EDGE_SET_CRITICAL(data, edge_index, critical) get_edge_info<cg_edge_info>(edge_index, *(data))->is_critical = critical

/**
 * Information associated with the whole graph of a circuit.
 */
struct cg_graph_info : public GraphInfo
{
   /// primary input node.
   /// gen and io port are associated with the entry node
   vertex Entry;

   /// primary output node
   vertex Exit;

   std::string Entry_name;
   std::string Exit_name;

   /**
    * Print the information associated with the current node of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const
   {
      os << "Entry " << Entry_name << "Exit " << Exit_name << std::endl;
   }

   /**
    * Constructor
    */
   cg_graph_info(vertex en, std::string en_name, vertex ex, std::string ex_name) : Entry(en), Exit(ex), Entry_name(std::move(en_name)), Exit_name(std::move(ex_name))
   {
   }

   /**
    * Empty constructor
    */
   cg_graph_info() : Entry_name(ENTRY), Exit_name(EXIT)
   {
   }

   /**
    * Destructor
    */
   ~cg_graph_info() override = default;
};

class cg_edge_writer
{
 public:
   explicit cg_edge_writer(const graph* _g);
   void operator()(std::ostream& out, const EdgeDescriptor& e) const;

 private:
   const graph* g;
};

class cg_label_writer
{
 public:
   explicit cg_label_writer(const graph* _g);

   void operator()(std::ostream& out, const vertex& v) const;

 private:
   const graph* g;
};

#endif
