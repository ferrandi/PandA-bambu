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
 * @file basic_block.hpp
 * @brief Class specification of the basic_block structure.
 *
 * This structure is used to represent graphs where nodes are basic_block.
 * A basic block is a sequence of instructions with only one entry and
 * only one exit. If any of the instructions are executed, they
 * will be all executed, and in sequence from first to last.
 * Jumps, if any, start a block, and jumps end a block.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef BASIC_BLOCK_HPP
#define BASIC_BLOCK_HPP
#include "cdfg_edge_info.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "graph_info.hpp"
#include "node_info.hpp"
#include "refcount.hpp"

#include <cstddef>
#include <list>
#include <string>
#include <utility>

#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_UNORDERED.hpp"

CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(FunctionBehavior);
CONSTREF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(bloc);
class BasicBlocksGraphConstructor;

/**
 * Selectors used only in basic block graphs; numbers continue from cdfg_edge_info.hpp
 */
/// dominator graph edge selector
#define D_SELECTOR (1 << 7)

/// post-dominator graph edge selector
#define PD_SELECTOR (1 << 8)

/// j graph edge selector for dj graph (used during loop computation)
#define J_SELECTOR (1 << 9)

/**
 * Definition of the node_info object for the basic_block graph.
 * This object info defines the list of vertices associated with the basic block node.
 */
struct BBNodeInfo : public NodeInfo
{
   /// id of the loop to which basic block belongs to (0 if it doesn't belong to any loop)
   unsigned int loop_id{0};

   /// id of the control equivalent region
   unsigned int cer{0};

   /// Clang/LLVM info associated with this basic block
   blocRef block{nullptr};

   /// List of operation vertices associated with basic block node
   std::list<gc_vertex_descriptor> statements_list;

   BBNodeInfo() = default;

   /**
    * Constructor which uses Clang/LLVM information
    * @param _block is the block in the Clang/LLVM dump
    */
   BBNodeInfo(const blocRef& _block) : block(_block)
   {
   }

   /**
    * Adds an operation to the list of the statements
    * @param op is the operation to be added
    */
   void add_operation_node(const gc_vertex_descriptor op);

   /**
    * Returns the first operation vertex associated with the basic block
    * @return the first operation vertex of the basic block.
    */
   gc_vertex_descriptor get_first_operation() const;

   /**
    * Returns the last operation vertex associated with the basic block.
    * @return the last operation statement of the basic block.
    */
   gc_vertex_descriptor get_last_operation() const;

   /**
    * Returns true if there is no node associated with the basic block.
    * @return true if there is no node associated with the basic block
    */
   bool empty() const;

   /**
    * Returns the number of vertices of the graph representing
    * the original program, associated with the current vertex of
    * the basic block graph
    * @return size of basic block
    */
   size_t size() const;

   /**
    * Returns the index of the basic block
    */
   unsigned int get_bb_index() const;

   /**
    * Returns the live in of the basic block
    */
   const CustomOrderedSet<unsigned int>& getLiveInBbVariables() const;

   /**
    * Returns the live in of the basic block
    */
   const CustomOrderedSet<unsigned int>& getLiveOutBbVariables() const;
};

/**
 * Information associated with a basic block edge
 */
struct BBEdgeInfo : public CdfgEdgeInfo
{
   /**
    * Return the labels associated with a selector
    */
   CustomOrderedSet<unsigned int> get_labels(const int selector) const;

 private:
   friend class BasicBlocksGraphConstructor;
   friend class FunctionBehavior;

   /// edge instrumented weight
   unsigned long long epp_value{0};
};

/**
 * Information associated with the whole basic-block graph
 */
struct BBGraphInfo : public GraphInfo
{
   /// NOTE: this is equivalent to a weakrefcount since deleter should be null
   application_managerConstRef AppM{nullptr};

   /// The index of the function
   unsigned int function_index{0};

   CustomUnorderedMap<unsigned int, gc_vertex_descriptor> bb_index_map;

   /// Index identifying the entry basic block.
   gc_vertex_descriptor entry_vertex{gc_null_vertex()};

   /// Index identifying the exit basic block.
   gc_vertex_descriptor exit_vertex{gc_null_vertex()};

   BBGraphInfo() = default;

   /**
    * Constructor with profiling information
    * @param AppM is the application manager
    * @param function_index is the function behavior of the function to which belongs
    */
   BBGraphInfo(const application_managerConstRef AppM, const unsigned int function_index);
};

struct BBGraphsCollection : public graphs_collection<BBNodeInfo, BBEdgeInfo, BBGraphInfo>
{
   BBGraphsCollection(const BBGraphInfo& info) : graphs_collection<BBNodeInfo, BBEdgeInfo, BBGraphInfo>(info)
   {
   }

   virtual ~BBGraphsCollection() = default;
};

class BBGraph : public graph<BBGraphsCollection>
{
 public:
   BBGraph(const BBGraphsCollection& g, int _selector);

   BBGraph(const BBGraphsCollection& g, int _selector, const CustomUnorderedSet<vertex_descriptor>& sub);

   size_t num_bblocks() const;

   void writeDot(const std::filesystem::path& file_name, const int detail_level = 0) const;

   void writeDot(const std::filesystem::path& file_name, const CustomUnorderedSet<vertex_descriptor>& annotated,
                 const int detail_level = 0) const;
};

#if !HAVE_UNORDERED
class BBVertexSorter
{
   /// The basic block graph to which vertices belong
   /// Note: this should be const, but can not because of assignment operator
   const BBGraphsCollection* bb_graph;

 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph to which vertices belong
    */
   BBVertexSorter(const BBGraphsCollection* bb_graph);

   /**
    * Compare position of two vertices
    * @param x is the first step
    * @param y is the second step
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(const BBGraphsCollection::vertex_descriptor x, const BBGraphsCollection::vertex_descriptor y) const;
};

class BBEdgeSorter
{
   /// The basic block graph to which edges belong
   /// Note: this should be const, but can not because of assignment operator
   const BBGraphsCollection* bb_graph;

   /// The vertex sorter
   BBVertexSorter bb_sorter;

 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph to which edges belong
    */
   explicit BBEdgeSorter(const BBGraphsCollection* bb_graph);

   /**
    * Compare position of two edges
    * @param x is the first step
    * @param y is the second step
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(const BBGraphsCollection::edge_descriptor& x, const BBGraphsCollection::edge_descriptor& y) const;
};
#endif

/**
 * The key comparison function for vertices set based on levels
 */
class bb_vertex_order_by_map
{
   /// Topological sorted vertices
   const std::map<gc_vertex_descriptor, unsigned int>& ref;

 public:
   /**
    * Constructor
    * @param _ref is the map with the topological sort of vertices
    */
   explicit bb_vertex_order_by_map(const std::map<gc_vertex_descriptor, unsigned int>& _ref) : ref(_ref)
   {
   }

   /**
    * Compares position of two vertices sorted in topological order
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x precedes y in the topological order, false otherwise
    */
   bool operator()(const gc_vertex_descriptor x, const gc_vertex_descriptor y) const
   {
      return ref.find(x)->second < ref.find(y)->second;
   }
};

class BBVertexWriter : public VertexWriter<BBGraph>
{
 private:
   /// The function behavior
   const FunctionBehaviorConstRef function_behavior;

   /// The helper
   const BehavioralHelperConstRef helper;

   /// The set of vertices to be annotated
   CustomUnorderedSet<BBGraph::vertex_descriptor> annotated;

#if HAVE_HLS_BUILT
   /// The scheduling solution
   const ScheduleConstRef schedule;
#endif

 public:
   /**
    * The constructor
    * @param g is the graph to be printed
    * @param annotated is the set of the vertices to be annotated
    */
   BBVertexWriter(const BBGraph& g, CustomUnorderedSet<BBGraph::vertex_descriptor> annotated =
                                        CustomUnorderedSet<BBGraph::vertex_descriptor>());

   void operator()(std::ostream& out, BBGraph::vertex_descriptor v) const override;
};

/**
 * Class which prints the edge of a basic block graph in dot format
 */
class BBEdgeWriter : public EdgeWriter<BBGraph>
{
   /// The helper used to print the labels
   const BehavioralHelperConstRef BH;

 public:
   /**
    * Constructor
    * @param g is the bb_graph to be printed
    */
   explicit BBEdgeWriter(const BBGraph& g);

   void operator()(std::ostream& out, const BBGraph::edge_descriptor& e) const override;
};

#endif
