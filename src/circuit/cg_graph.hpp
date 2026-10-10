/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file cg_graph.hpp
 * @brief Node, edge and graph description of the graph associated with a structural description.
 *
 * @author Matteo Barbati <matteo.mbarbati@gmail.com>
 *
 */
#ifndef CG_NODE_HPP
#define CG_NODE_HPP

#include "edge_info.hpp"
#include "graph.hpp"
#include "graph_info.hpp"
#include "refcount.hpp"
#include "typed_node_info.hpp"

#include <string>

REF_FORWARD_DECL(structural_object);

/// Data line selector
#define DATA_SELECTOR 1
/// Clock line selector
#define CLOCK_SELECTOR 2
/// All lines selector
#define ALL_LINES_SELECTOR (DATA_SELECTOR | CLOCK_SELECTOR)
/// All but clock lines selector
#define PURE_DATA_SELECTOR DATA_SELECTOR

/**
 * Information associated with a circuit graph node.
 */
struct CGNodeInfo : public TypedNodeInfo
{
   /**
    * Definition of the internal graph associated with the node.
    */
   structural_objectRef reference{nullptr};

   bool is_critical{false};

   /**
    * Print the information associated with the current node of the graph.
    * @param os is the output stream.
    * @param detail_level selects the amount of detail to print
    */
   void print(std::ostream& os, int detail_level = 0) const override
   {
      TypedNodeInfo::print(os, detail_level);
      os << " " << reference << std::endl;
   }
};

/**
 * Information associated with a circuit graph edge.
 */
struct CGEdgeInfo : public EdgeInfo
{
   /// port from which the edge is generated
   structural_objectRef from_port{nullptr};

   /// destination port/channel to the edge is attached
   structural_objectRef to_port{nullptr};

   /// flag to check if the edge is involved into the critical path
   bool is_critical{false};

   /**
    * Print the information associated with the current node of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const;
};

/**
 * Information associated with the whole graph of a circuit.
 */
struct CGGraphInfo : public GraphInfo
{
   /// primary input node.
   /// gen and io port are associated with the entry node
   gc_vertex_descriptor Entry{gc_null_vertex()};

   /// primary output node
   gc_vertex_descriptor Exit{gc_null_vertex()};

   std::string Entry_name{ENTRY};
   std::string Exit_name{EXIT};

   /**
    * Print the information associated with the current node of the graph.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const
   {
      os << "Entry " << Entry_name << "Exit " << Exit_name << std::endl;
   }

   CGGraphInfo()
   {
   }

   CGGraphInfo(gc_vertex_descriptor en, const std::string& en_name, gc_vertex_descriptor ex, const std::string& ex_name)
       : Entry(en), Exit(ex), Entry_name(en_name), Exit_name(ex_name)
   {
   }
};

using CGGraphsCollection = graphs_collection<CGNodeInfo, CGEdgeInfo, CGGraphInfo>;

using CGGraph = graph<CGGraphsCollection>;

struct CGVertexWriter : public VertexWriter<CGGraph>
{
   CGVertexWriter(const CGGraph& g) : VertexWriter<CGGraph>(g, 0)
   {
   }

   void operator()(std::ostream& out, CGGraph::vertex_descriptor v) const;
};

struct CGEdgeWriter : public EdgeWriter<CGGraph>
{
   CGEdgeWriter(const CGGraph& g) : EdgeWriter<CGGraph>(g, 0)
   {
   }

   void operator()(std::ostream& out, const CGGraph::edge_descriptor& e) const;
};

#endif
