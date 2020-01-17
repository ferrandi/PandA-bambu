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
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BASIC_BLOCK_HPP
#define BASIC_BLOCK_HPP

#include "config_HAVE_UNORDERED.hpp"

#include "cdfg_edge_info.hpp" // for CdfgEdgeInfo
#include "edge_info.hpp"      // for EdgeInfoRef
#include "graph.hpp"          // for vertex, EdgeDesc...
#include "graph_info.hpp"     // for GraphInfo
#include "node_info.hpp"      // for NodeInfo
#include "refcount.hpp"       // for refcount, Refcou...
#include <cstddef>            // for size_t
#include <functional>         // for binary_function
#include <list>               // for list
#include <string>             // for string
#include <utility>            // for pair

#include "custom_map.hpp"
#include "custom_set.hpp"

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(application_manager);
class BasicBlocksGraphConstructor;
REF_FORWARD_DECL(bloc);
class FunctionBehavior;
CONSTREF_FORWARD_DECL(Parameter);
//@}

/**
 * Selectors used only in basic block graphs; numbers continue from cdfg_edge_info.hpp
 */
/// dominator graph edge selector
#define D_SELECTOR (1 << 7)

/// post-dominator graph edge selector
#define PD_SELECTOR (1 << 8)

/// j graph edge selector for dj graph (used during loop computation)
#define J_SELECTOR (1 << 9)

/// Path Profiling Basic Block edge selector
#define PP_SELECTOR (1 << 10)

/**
 * Definition of the node_info object for the basic_block graph.
 * This object info defines the list of vertices associated with the basic block node.
 */
struct BBNodeInfo : public NodeInfo
{
 public:
   /// id of the loop to which basic block belongs to (0 if it doesn't belong to any loop)
   unsigned int loop_id;

   /// id of the control equivalent region
   unsigned int cer;

   /// Structure associated with this basic block in the GCC tree
   blocRef block;

   /// List of operation vertices associated with basic block node
   std::list<vertex> statements_list;

   /**
    * Empty constructor
    */
   BBNodeInfo();

   /**
    * Constructor which uses gcc information
    * @param _block is the block in the gcc dump
    */
   explicit BBNodeInfo(blocRef _block);

   /**
    * Adds an operation to the list of the statements
    * @param op is the operation to be added
    */
   void add_operation_node(const vertex op);

   /**
    * Returns the first operation vertex associated with the basic block
    * @return the first operation vertex of the basic block.
    */
   vertex get_first_operation() const;

   /**
    * Returns the last operation vertex associated with the basic block.
    * @return the last operation statement of the basic block.
    */
   vertex get_last_operation() const;

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
   const CustomOrderedSet<unsigned int>& get_live_in() const;

   /**
    * Returns the live in of the basic block
    */
   const CustomOrderedSet<unsigned int>& get_live_out() const;
};
/// refcount definition of the class
typedef refcount<BBNodeInfo> BBNodeInfoRef;
typedef refcount<const BBNodeInfo> BBNodeInfoConstRef;

/**
 * Information associated with a basic block edge
 */
struct BBEdgeInfo : public CdfgEdgeInfo
{
 private:
   friend class BasicBlocksGraphConstructor;
   friend class FunctionBehavior;

   /// edge instrumented weight
   unsigned long long epp_value;

 public:
   /**
    * Constructor
    */
   BBEdgeInfo();

   /**
    * Destructor
    */
   ~BBEdgeInfo() override;

   /**
    * Function returning true when the edge is a then control dependence edge, false otherwise
    */
   bool cdg_edge_T() const;

   /**
    * Function returning true when the edge is an else control dependence edge, false otherwise
    */
   bool cdg_edge_F() const;

   /**
    * Function returning true when the edge is a then control flow edge, false otherwise
    */
   bool cfg_edge_T() const;

   /**
    * Function returning true when the edge is an else control flow edge, false otherwise
    */
   bool cfg_edge_F() const;

   /**
    * Function returning true when the edge is a control flow edge exiting from a switch, false otherwise
    */
   bool switch_p() const;

   /**
    * Return the labels associated with a selector
    */
   const CustomOrderedSet<unsigned int> get_labels(const int selector) const;

   /**
    * Function that sets the epp_edge associated with the edge
    */
   void set_epp_value(unsigned long long _epp_value);

   /**
    * Function returning the epp_value associated with the edge
    */
   unsigned long long get_epp_value() const;
};
/// refcount definition of the class
typedef refcount<BBEdgeInfo> BBEdgeInfoRef;
typedef refcount<const BBEdgeInfo> BBEdgeInfoConstRef;

/**
 * Information associated with the whole basic-block graph
 */
struct BBGraphInfo : public GraphInfo
{
   /// NOTE: this is equivalent to a weakrefcount since deleter should be null
   const application_managerConstRef AppM;

   /// The index of the function
   const unsigned int function_index;

   CustomUnorderedMap<unsigned int, vertex> bb_index_map;

   /// Index identifying the entry basic block.
   vertex entry_vertex;

   /// Index identifying the exit basic block.
   vertex exit_vertex;

   /**
    * Constructor with profiling information
    * @param AppM is the application manager
    * @param function_index is the function behavior of the function to which belongs
    */
   BBGraphInfo(const application_managerConstRef AppM, const unsigned int function_index);
};
/// refcount definition of the class
typedef refcount<BBGraphInfo> BBGraphInfoRef;
typedef refcount<const BBGraphInfo> BBGraphInfoConstRef;

/**
 * This structure defines graphs where nodes are basic_blocks.
 * Graphs defined are: control flow graph, control dependence graph, dominator tree and post-dominator tree
 * The basic_block structure and the control flow graph can be extracted directly from the raw structure when cfg
 * pass is done or built up starting from a CFG where nodes are standard vertices.
 */
class BBGraphsCollection : public graphs_collection
{
 private:
   friend class BasicBlocksGraphConstructor;

 public:
   /**
    * Constructor
    * @param bb_node_info is the info to be associated with the graph
    * @param parameters is the set of input parameters
    */
   BBGraphsCollection(const BBGraphInfoRef bb_node_info, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~BBGraphsCollection() override;

   /**
    * Add an edge with empty information associated
    * @param source is the source of the edge
    * @param target is the target of the edge
    * @param selector is the selector to be added
    * @return the created edge
    */
   inline EdgeDescriptor AddEdge(const vertex source, const vertex target, const int selector)
   {
      if(ExistsEdge(source, target))
         return AddSelector(source, target, selector);
      else
         return InternalAddEdge(source, target, selector, EdgeInfoRef(new BBEdgeInfo()));
   }
};
/// refcount definition of the class
typedef refcount<BBGraphsCollection> BBGraphsCollectionRef;
typedef refcount<const BBGraphsCollection> BBGraphsCollectionConstRef;

/**
 * Class used to describe a particular graph with basic blocks as nodes
 */
struct BBGraph : public graph
{
 public:
   /**
    * Standard constructor.
    * @param BBGraphsCollection is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    */
   BBGraph(const BBGraphsCollectionRef bb_graphs_collection, int selector);

   /**
    * Sub-graph constructor.
    * @param bb_graphs_collection is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    * @param sub is the set of vertices on which the graph is filtered.
    */
   BBGraph(const BBGraphsCollectionRef bb_graphs_collection, int selector, CustomUnorderedSet<vertex>& sub);

   /**
    * Destructor
    */
   virtual ~BBGraph() override = default;

   /**
    * Writes this graph in dot format
    * @param file_name is the file where the graph has to be printed
    * @param detail_level is the detail level of the printed graph
    */
   void WriteDot(const std::string& file_name, const int detail_level = 0) const;

   /**
    * Write this graph in dot format with some basic blocks highlightened
    * @param file_name is the name of the file
    * @param detail_level is the detail level of the printed graph
    * @param annotated is the set of the vertices to be annotated
    */
   void WriteDot(const std::string& file_name, const CustomUnorderedSet<vertex>& annotated, const int detail_level = 0) const;

   /**
    * Returns the number of basic blocks contained into the graph
    */
   size_t num_bblocks() const;

   /**
    * Return the info associated with a basic block
    * @param node is the basic block vertex to be considered
    * @return the info associated with a basic block
    */
   inline BBNodeInfoRef GetBBNodeInfo(const vertex node)
   {
      return RefcountCast<BBNodeInfo>(graph::GetNodeInfo(node));
   }

   /**
    * Return the info associated with a basic block
    * @param node is the basic block vertex to be considered
    * @return the info associated with a basic block
    */
   inline const BBNodeInfoConstRef CGetBBNodeInfo(const vertex node) const
   {
      return RefcountCast<const BBNodeInfo>(graph::CGetNodeInfo(node));
   }

   /**
    * Returns the info associated with an edge
    */
   inline BBEdgeInfoRef GetBBEdgeInfo(const EdgeDescriptor e)
   {
      return RefcountCast<BBEdgeInfo>(graph::GetEdgeInfo(e));
   }

   /**
    * Returns the info associated with an edge
    */
   inline const BBEdgeInfoConstRef CGetBBEdgeInfo(const EdgeDescriptor e) const
   {
      return RefcountCast<const BBEdgeInfo>(graph::CGetEdgeInfo(e));
   }

   /**
    * Returns the property associated with the graph
    * @return the property associated with the graph
    */
   inline BBGraphInfoRef GetBBGraphInfo()
   {
      return RefcountCast<BBGraphInfo>(graph::GetGraphInfo());
   }

   /**
    * Returns the property associated with the graph
    * @return the proprty associated with the graph
    */
   inline const BBGraphInfoConstRef CGetBBGraphInfo() const
   {
      return RefcountCast<const BBGraphInfo>(graph::CGetGraphInfo());
   }
};
/// refcount definition of the class
typedef refcount<BBGraph> BBGraphRef;
typedef refcount<const BBGraph> BBGraphConstRef;

#if !HAVE_UNORDERED
class BBVertexSorter : std::binary_function<vertex, vertex, bool>
{
 private:
   /// The basic block graph to which vertices belong
   /// Note: this should be const, but can not because of assignment operator
   BBGraphConstRef bb_graph;

 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph to which vertices belong
    */
   explicit BBVertexSorter(const BBGraphConstRef bb_graph);

   /**
    * Compare position of two vertices
    * @param x is the first step
    * @param y is the second step
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(const vertex x, const vertex y) const;
};

class BBEdgeSorter : std::binary_function<EdgeDescriptor, EdgeDescriptor, bool>
{
 private:
   /// The basic block graph to which edges belong
   /// Note: this should be const, but can not because of assignment operator
   BBGraphConstRef bb_graph;

   /// The vertex sorter
   BBVertexSorter bb_sorter;

 public:
   /**
    * Constructor
    * @param bb_graph is the basic block graph to which edges belong
    */
   explicit BBEdgeSorter(const BBGraphConstRef bb_graph);

   /**
    * Compare position of two edges
    * @param x is the first step
    * @param y is the second step
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(const EdgeDescriptor x, const EdgeDescriptor y) const;
};
#endif

/**
 * The key comparison function for vertices set based on levels
 */
class bb_vertex_order_by_map : std::binary_function<vertex, vertex, bool>
{
 private:
   /// Topological sorted vertices
   const std::map<vertex, unsigned int>& ref;

 public:
   /**
    * Constructor
    * @param ref_ is the map with the topological sort of vertices
    */
   explicit bb_vertex_order_by_map(const std::map<vertex, unsigned int>& _ref) : ref(_ref)
   {
   }

   /**
    * Compares position of two vertices sorted in topological order
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x precedes y in the topological order, false otherwise
    */
   bool operator()(const vertex x, const vertex y) const
   {
      return ref.find(x)->second < ref.find(y)->second;
   }
};
#endif
