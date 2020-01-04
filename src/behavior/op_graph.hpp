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
 * @file op_graph.hpp
 * @brief Data structures used in operations graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef OP_GRAPH_HPP
#define OP_GRAPH_HPP

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"
#include "config_HAVE_UNORDERED.hpp"

/// Superclasses include
#include "cdfg_edge_info.hpp"
#include "graph.hpp"
#include "typed_node_info.hpp"

/// behavior include
#include "function_behavior.hpp"

#include <boost/graph/graph_traits.hpp> // for graph_traits<>::...
#include <iosfwd>                       // for ostream
#include <limits>                       // for numeric_limits
#include <list>                         // for list
#include <set>                          // for set
#include <string>                       // for string

/// utility includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"
#include "strong_typedef.hpp" // for UINT_STRONG_TYPEDEF

CONSTREF_FORWARD_DECL(BehavioralHelper);
enum class FunctionBehavior_VariableAccessType;
CONSTREF_FORWARD_DECL(hls);
class OpVertexSet;

UINT_STRONG_TYPEDEF(MemoryAddress);
/// constant used to represent tree node index of entry operation
#define ENTRY_ID (std::numeric_limits<unsigned int>::max())
/// constant used to represent tree node index of exit operation
#define EXIT_ID (std::numeric_limits<unsigned int>::max() - 1)

/**
 * @name Node name and type definition for the cdfg node.
 */
//@{
/**
 * constant identifying a node of opaque type
 */
#define TYPE_OPAQUE 1 << 0

/**
 * constant identifying the node type of a EXTERNAL operation (a function call)
 */
#define TYPE_EXTERNAL 1 << 4

/**
 * constant identifying the node type of an IF operation.
 */
#define TYPE_IF 1 << 5

/**
 * constant identifying the node type of a SWITCH operation.
 */
#define TYPE_SWITCH 1 << 6

/**
 * constant string identifying the node type of an WHILE operation.
 */
#define TYPE_WHILE 1 << 7

/**
 * constant string identifying the node type of an WHILE operation.
 */
#define TYPE_FOR 1 << 8

/**
 * constant string identifying the node type of an DO operation.
 */
#define TYPE_DO 1 << 9

/**
 * constant string identifying the node type of an ASSIGN operation.
 */
#define TYPE_ASSIGN 1 << 10

/**
 * constant string identifying a type for a no operation. Only used for operations associated with empty basic blocks.
 */
#define TYPE_NOP 1 << 11

/**
 * constant string identifying an operation node of type PHI
 */
#define TYPE_PHI 1 << 12

/**
 * constant string identifying an operation node of type return expr
 */
#define TYPE_RET 1 << 13

/**
 * Constant string identifying an operation that has to be removed
 */
#define TYPE_TO_BE_REMOVED 1 << 14

/**
 * A vertex is of type TYPE_LABEL when it is a target of a goto expression.
 * Used to define the first vertex of a basic block header of a loop.
 */
#define TYPE_LABEL 1 << 15

/**
 * A vertex is of type TYPE_GOTO when it is associated with a goto expression.
 * Mainly used in loops and in computed goto expressions.
 */
#define TYPE_GOTO 1 << 16

/**
 * constant string identifying an operation node of type virtual phi-nodes
 */
#define TYPE_VPHI 1 << 17

/**
 * Constant string identifying an operation that is a variable initialization
 */
#define TYPE_INIT 1 << 18

/**
 * Constant string identifying a memory load operation
 */
#define TYPE_LOAD 1 << 19

/**
 * Constant string identifying a memory store operation
 */
#define TYPE_STORE 1 << 20

/**
 * A vertex is of type TYPE_MEMCPY when it is associated with a assignment between struct/union.
 */
#define TYPE_MEMCPY 1 << 21

/**
 * A vertex is of type TYPE_WAS_GIMPLE_PHI when it is comes from a split of phi nodes
 */
#define TYPE_WAS_GIMPLE_PHI 1 << 22

/**
 * A vertex of type FIRST_OP if it is the first operation of the application
 */
#define TYPE_FIRST_OP 1 << 23

/**
 * A vertex of type LAST_OP if it is the last operation of the application
 */
#define TYPE_LAST_OP 1 << 24

/**
 * constant identifying the a multi-way if
 */
#define TYPE_MULTIIF 1 << 25

/**
 * Constant identifying an atomic operation
 */
#define TYPE_ATOMIC 1 << 26

/**
 * Constant identifying a predicated operation
 */
#define TYPE_PREDICATED 1 << 27

//@}

/**
 * constant string identifying the operation performed by an assignment.
 */
#define ASSIGN "ASSIGN"
#if 0
/**
* constant string identifying the operation performed by a store in memory.
*/
#define STORE "STORE"
#define PREDICATED_STORE "PREDICATED_STORE"
/**
* constant string identifying the operation performed by a load from memory.
*/
#define LOAD "LOAD"
#define PREDICATED_LOAD "PREDICATED_LOAD"
#endif
/**
 * constant string identifying the operation performed by an assignment.
 */
#define ASSERT_EXPR "assert_expr"

/**
 * constant string identifying the operation performed by an extract_bit_expr.
 */
#define EXTRACT_BIT_EXPR "extract_bit_expr"

/**
 * constant string identifying the operation performed by an extract_bit_expr.
 */
#define LUT_EXPR "lut_expr"

/**
 * constant string identifying the operation performed by a READ_COND.
 */
#define READ_COND "READ_COND"

/**
 * constant string identifying the operation performed by a MULTI_READ_COND.
 */
#define MULTI_READ_COND "MULTI_READ_COND"

/**
 * constant string identifying the operation performed by a SWITCH_COND.
 */
#define SWITCH_COND "SWITCH_COND"

/**
 * constant string identifying the operation performed by a GIMPLE_LABEL.
 */
#define GIMPLE_LABEL "gimple_label"

/**
 * constant string identifying the operation performed by a GIMPLE_GOTO.
 */
#define GIMPLE_GOTO "gimple_goto"

/**
 * constant string identifying a no operation. Only used for operations associated with empty basic blocks.
 */
#define NOP "NOP"

/**
 * constant string identifying the operation performed by a gimple_return.
 */
#define GIMPLE_RETURN "gimple_return"

/**
 * constant string identifying the operation performed by a gimple_return.
 */
#define GIMPLE_NOP "gimple_nop"

/**
 * constant string identifying the operation performed by a gimple_phi.
 */
#define GIMPLE_PHI "gimple_phi"

/**
 * constant string identifying the operation performed by a gimple_asm.
 */
#define GIMPLE_ASM "gimple_asm"

/**
 * constant string identifying the operation performed by a GIMPLE_PRAGMA.
 */
#define GIMPLE_PRAGMA "gimple_pragma"

/**
 * constant string identifying the operation performed when two objects are memcopied.
 */
#define MEMCPY "__internal_bambu_memcpy"

/**
 * constant string identifying the operation performed when two objects are memcompared.
 */
#define MEMCMP "memcmp"

/**
 * constant string identifying the operation performed when two objects are memsetted.
 */
#define MEMSET "__internal_bambu_memset"

/**
 * constant string identifying the operation performed when a vector concatenation is considered.
 */
#define VECT_CONCATENATION "VECT_CONCATENATION"

/**
 * constant string identifying the addressing operation.
 */
#define ADDR_EXPR "addr_expr"

/**
 * constant string identifying some conversion expressions
 */
#define NOP_EXPR "nop_expr"

/**
 * constant string identifying integer to float conversions
 */
#define FLOAT_EXPR "float_expr"

/**
 * constant string identifying float to integer conversions
 */
#define FIX_TRUNC_EXPR "fix_trunc_expr"

/**
 * constant string identifying some conversion expressions
 */
#define CONVERT_EXPR "convert_expr"

/**
 * constant string identifying view convert expressions
 */
#define VIEW_CONVERT_EXPR "view_convert_expr"

/// constant defining the builtin wait call intrinsic function
#define BUILTIN_WAIT_CALL "__builtin_wait_call"

/**
 * Information associated with a generic operation node.
 */
struct OpNodeInfo : public TypedNodeInfo
{
   /// set of cited variables (i.e., variables which are included in the c printing of this statement)
   CustomSet<unsigned int> cited_variables;

   /// set of scalar ssa accessed in this node
   CustomMap<FunctionBehavior_VariableType, CustomMap<FunctionBehavior_VariableAccessType, CustomSet<unsigned int>>> variables;

#if HAVE_EXPERIMENTAL
   /// set of memory locations dynamically accessed in this node
   CustomMap<FunctionBehavior_VariableAccessType, CustomSet<MemoryAddress>> dynamic_memory_locations;
#endif

   /// Set of actual parameters of called function (used in pthread backend
   std::list<unsigned int> actual_parameters;

   /// The tree node associated with this vertex
   tree_nodeRef node;

   /// Store the index of called functions
   CustomUnorderedSet<unsigned int> called;

   /// Store the index of the basic block which this operation vertex belongs to
   unsigned int bb_index;

   /// Store the index of the control equivalent region
   unsigned int cer;

   /**
    * Constructor
    */
   OpNodeInfo();

   /**
    * Destructor
    */
   ~OpNodeInfo() override;

   /**
    * Initialize variable maps
    */
   void Initialize();

   /**
    * Return a set of accessed scalar variables
    * @param variable_type is the type of variables to be considered
    * @param access_type is the type of accesses to be considered
    */
   const CustomSet<unsigned int>& GetVariables(const FunctionBehavior_VariableType variable_type, const FunctionBehavior_VariableAccessType access_type) const;

#if HAVE_EXPERIMENTAL
   /**
    * Return a set of accessed dynamid data memory location
    * @param access_type is the type of accesses to be considered
    */
   const CustomSet<MemoryAddress>& GetDynamicMemoryLocations(const FunctionBehavior_VariableAccessType access_type) const;
#endif

#if HAVE_BAMBU_BUILT || HAVE_TUCANO_BUILT
   /**
    * Return the operation associated with the vertex
    */
   const std::string GetOperation() const;
#endif

   /**
    * Return the node id of the operation associated with the vertex
    */
   unsigned int GetNodeId() const;

   /**
    * Print the content of this node
    * @param stream is the stream on which this node has to be printed
    * @param behavioral_helper is the helper associated with the function
    * @param dotty_format specifies if the output has to be formatted for a dotty label
    */
   void Print(std::ostream& stream, const BehavioralHelperConstRef behavioral_helper, const bool dotty_format) const;
};
typedef refcount<OpNodeInfo> OpNodeInfoRef;
typedef refcount<const OpNodeInfo> OpNodeInfoConstRef;

/**
 * Macro returning the index of the basic block which the node belongs to
 * @param vertex_index is the index of the cdfg node.
 */
#define GET_BB_INDEX(data, vertex_index) Cget_node_info<OpNodeInfo>(vertex_index, *(data))->bb_index

/**
 * Macro returning the control equivalent region of the node
 * @param var_index is the NodeID of the variable
 */
#define GET_CER(data, vertex_index) Cget_node_info<OpNodeInfo>(vertex_index, *(data))->cer

/**
 * Selectors used only in operation graphs; numbers continue from cdfg_edge_info.hpp
 */
/// Data flow graph edge selector between computed on scalars
#define DFG_SCA_SELECTOR 1 << 7
/// Data flow graph edge selector between computed on aggregates
#define DFG_AGG_SELECTOR 1 << 8
/// Data flow graph edge selector
#define DFG_SELECTOR (DFG_SCA_SELECTOR | DFG_AGG_SELECTOR)
/// Data flow graph edge selector between computed on scalars
#define FB_DFG_SCA_SELECTOR 1 << 9
/// Data flow graph edge selector between computed on aggregates
#define FB_DFG_AGG_SELECTOR 1 << 10
/// Feedback Data flow graph edge selector
#define FB_DFG_SELECTOR (FB_DFG_SCA_SELECTOR | FB_DFG_AGG_SELECTOR)
/// Data flow graph with feedback edges
#define FDFG_SELECTOR (DFG_SELECTOR | FB_DFG_SELECTOR)

/// Anti-dependence graph edge selector computed on scalar
#define ADG_SCA_SELECTOR 1 << 11
/// Anti-dependence graph edge selector computed on aggregates
#define ADG_AGG_SELECTOR 1 << 12
/// Anti-dependence graph edge selector
#define ADG_SELECTOR (ADG_SCA_SELECTOR | ADG_AGG_SELECTOR)
/// Feedback Anti-dependence graph edge selector computed on scalar
#define FB_ADG_SCA_SELECTOR 1 << 13
/// Feedback Anti-dependence graph edge selector computed on aggregates
#define FB_ADG_AGG_SELECTOR 1 << 14
/// Feedback anti-dependence graph edge selector
#define FB_ADG_SELECTOR (FB_ADG_SCA_SELECTOR | FB_ADG_AGG_SELECTOR)
/// Anti-dependence graph selector with feedback edges
#define FADG_SELECTOR (ADG_SELECTOR | FB_ADG_SELECTOR)

/// Output-dependence graph edge selector computed on scalars
#define ODG_SCA_SELECTOR 1 << 15
/// Output-dependence graph edge selector computed on aggregates
#define ODG_AGG_SELECTOR 1 << 16
/// Output-dependence graph edge selector
#define ODG_SELECTOR (ODG_SCA_SELECTOR | ODG_AGG_SELECTOR)
/// Feedback Output-dependence graph edge selector computed on scalars
#define FB_ODG_SCA_SELECTOR 1 << 17
/// Feedback Output-dependence graph edge selector computed on aggregates
#define FB_ODG_AGG_SELECTOR 1 << 18
/// Feedback Output-dependence graph edge selector
#define FB_ODG_SELECTOR (FB_ODG_SCA_SELECTOR | FB_ODG_AGG_SELECTOR)
/// Output-dependence graph selector with feedback edges
#define FODG_SELECTOR (ODG_SELECTOR | FB_ODG_SELECTOR)

/// Control and Data dependence graph edge selector
#define SDG_SELECTOR (CDG_SELECTOR | DFG_SELECTOR)
/// Control and Data dependence graph and dependence edge selector
#define FSDG_SELECTOR (FCDG_SELECTOR | FDFG_SELECTOR)
/// Reducted sdg with feedback edges
#define RSDG_SELECTOR (TRED_SELECTOR)

/// Control and Data dependence and antidependence graph edge selector with feedback edges
#define FSADG_SELECTOR (CDG_SELECTOR | DFG_SELECTOR | ADG_SELECTOR | FB_CDG_SELECTOR | FB_DFG_SELECTOR | FB_ADG_SELECTOR)
/// Control and Data dependence and antidependence graph edge selector
#define SAODG_SELECTOR (CDG_SELECTOR | DFG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR)
/// data dependence antidependence and feedback graph edge selector
#define FADFG_SELECTOR (DFG_SELECTOR | ADG_SELECTOR)

/// Control edge in a speculation graph
#define CSG_SELECTOR 1 << 19
/// Speculation graph
#define SG_SELECTOR (CSG_SELECTOR | DFG_SELECTOR | ADG_SELECTOR | ODG_SELECTOR | FLG_SELECTOR)

/// Reduced PDG edge selector
#define RPDG_SELECTOR 1 << 20

/// Flow edge selector
#define FLG_SELECTOR 1 << 21

/// Feedback flow edge selector
#define FB_FLG_SELECTOR 1 << 22

/// Debug selector
#define DEBUG_SELECTOR 1 << 23

/**
 * The info associated with an edge of operation graph
 */
class OpEdgeInfo : public CdfgEdgeInfo
{
 public:
   /**
    * Constructor
    */
   OpEdgeInfo();

   /**
    * Destructor
    */
   ~OpEdgeInfo() override;

   /**
    * Function returning true when the edge is a then flow edge
    */
   bool FlgEdgeT() const;

   /**
    * Function returning true when the edge is an else flow edge
    */
   bool FlgEdgeF() const;
};
/// Refcount definition for OpEdgeInfo
typedef refcount<OpEdgeInfo> OpEdgeInfoRef;
typedef refcount<const OpEdgeInfo> OpEdgeInfoConstRef;

/**
 * information associated with the whole graph
 */
struct OpGraphInfo : public GraphInfo
{
   /**
    * Constructor
    */
   OpGraphInfo();

   /**
    * Destructor();
    */
   ~OpGraphInfo() override;

   /// Index identifying the entry vertex
   vertex entry_vertex;

   /// Index identifying the exit vertex
   vertex exit_vertex;

   /// The behavioral helper
   const BehavioralHelperConstRef BH;

   /// For each statement, the vertex in which it is contained
   CustomMap<unsigned int, vertex> tree_node_to_operation;

   /**
    * Constructor
    * @param BH is the helper of the function associated with this graph
    */
   explicit OpGraphInfo(const BehavioralHelperConstRef BH);
};

/// Refcount definition for OpGraphInfo
typedef refcount<OpGraphInfo> OpGraphInfoRef;
typedef refcount<const OpGraphInfo> OpGraphInfoConstRef;

#if HAVE_UNORDERED
/**
 * A set of operation vertices
 */
class OpVertexSet : public CustomUnorderedSet<vertex>
{
 public:
   /**
    * Constructor
    */
   explicit OpVertexSet(const OpGraphConstRef op_graph);
};

/**
 * Map from operation vertices to value
 */
template <typename value>
class OpVertexMap : public CustomUnorderedMap<vertex, value>
{
 public:
   /**
    * Constructor
    */
   explicit OpVertexMap(const OpGraphConstRef) : CustomUnorderedMap<vertex, value>()
   {
   }
};

class OpEdgeSet : public CustomUnorderedSet<EdgeDescriptor>
{
 public:
   /**
    * Constructor
    */
   explicit OpEdgeSet(const OpGraphConstRef op_graph);
};
#else
class OpVertexSorter : std::binary_function<vertex, vertex, bool>
{
 private:
   /// The operation graph to which vertices belong
   /// Note: this should be const, but can not because of assignment operator
   OpGraphConstRef op_graph;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to which vertices belong
    */
   explicit OpVertexSorter(const OpGraphConstRef op_graph);

   /**
    * Compare position of two vertices
    * @param x is the first step
    * @param y is the second step
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(const vertex x, const vertex y) const;
};

/**
 * A set of operation vertices
 */
class OpVertexSet : public std::set<vertex, OpVertexSorter>
{
 public:
   /**
    * Constructor
    */
   explicit OpVertexSet(const OpGraphConstRef op_graph);
};

/**
 * Map from operation vertices to value
 */
template <typename value>
class OpVertexMap : public std::map<vertex, value, OpVertexSorter>
{
 public:
   /**
    * Constructor
    */
   explicit OpVertexMap(const OpGraphConstRef op_graph) : std::map<vertex, value, OpVertexSorter>(OpVertexSorter(op_graph))
   {
   }
};

class OpEdgeSorter : std::binary_function<EdgeDescriptor, EdgeDescriptor, bool>
{
 private:
   /// The operation graph to which vertices belong
   /// Note: this should be const, but can not because of assignment operator
   OpGraphConstRef op_graph;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to which vertices belong
    */
   explicit OpEdgeSorter(const OpGraphConstRef op_graph);

   /**
    * Compare position of two edges
    * @param x is the first edge
    * @param y is the second edge
    * @return true if x < y
    */
   bool operator()(const EdgeDescriptor x, const EdgeDescriptor y) const;
};

class OpEdgeSet : public std::set<EdgeDescriptor, OpEdgeSorter>
{
 public:
   /**
    * Constructor
    */
   explicit OpEdgeSet(const OpGraphConstRef op_graph);
};
#endif

/**
 * This structure defines graphs where nodes are operations
 */
class OpGraphsCollection : public graphs_collection
{
 protected:
   /// The set of operations
   OpVertexSet operations;

 public:
   /**
    * Empty Constructror
    * @param info is the info associated with the graph
    * @param parameters is the set of input parameters
    */
   OpGraphsCollection(const OpGraphInfoRef info, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~OpGraphsCollection() override;

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
         return InternalAddEdge(source, target, selector, EdgeInfoRef(new OpEdgeInfo()));
   }

   /**
    * Add a vertex to this graph with a property
    * @param info is the property to be associated with the new vertex
    * @return the added vertex
    */
   boost::graph_traits<boost_graphs_collection>::vertex_descriptor AddVertex(const NodeInfoRef info) override;

   /**
    * Remove a vertex from this graph
    * @param v is the vertex to be removed
    */
   void RemoveVertex(boost::graph_traits<boost_graphs_collection>::vertex_descriptor v) override;

   /**
    * Return the vertices belonging to the graph
    */
   const OpVertexSet CGetOperations() const;

   /**
    * Remove all the edges and vertices from the graph
    */
   void Clear();
};

/// Refcount definition for OpGraphsCollectionRef
typedef refcount<OpGraphsCollection> OpGraphsCollectionRef;

/**
 * Class used to describe a particular graph with operations as nodes
 */
struct OpGraph : public graph
{
   /// Friend declaration of schedule to allow dot writing
   friend class Schedule;

 public:
   /**
    * Standard constructor.
    * @param g is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    */
   OpGraph(const OpGraphsCollectionRef g, int selector);

   /**
    * Sub-graph constructor.
    * @param g is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    * @param sub is the set of vertices on which the graph is filtered.
    */
   OpGraph(const OpGraphsCollectionRef g, int selector, const CustomUnorderedSet<boost::graph_traits<OpGraphsCollection>::vertex_descriptor>& sub);

   /**
    * Destructor
    */
   ~OpGraph() override;

   /**
    * Writes this graph in dot format
    * @param file_name is the file where the graph has to be printed
    * @param detail_level is the detail level of the printed graph
    */
   void WriteDot(const std::string& file_name, const int detail_level = 0) const;

#if HAVE_HLS_BUILT
   /**
    * Write this graph in dot format with timing information
    * @param file_name is the file where the graph has to be printed
    * @param HLS is the high level synthesis structure
    * @param critical_paths is the set of operations belonging to critical paths
    */
   void WriteDot(const std::string& file_name, const hlsConstRef HLS, const CustomSet<unsigned int> critical_paths) const;
#endif

   /**
    * Returns the info associated with a node
    * @param node is the operation to be considered
    * @return the associated property
    */
   inline OpNodeInfoRef GetOpNodeInfo(const vertex node)
   {
      return RefcountCast<OpNodeInfo>(graph::GetNodeInfo(node));
   }

   /**
    * Returns the info associated with a node
    * @param node is the operation to be considered
    * @return the associated property
    */
   inline const OpNodeInfoConstRef CGetOpNodeInfo(const vertex node) const
   {
      return RefcountCast<const OpNodeInfo>(graph::CGetNodeInfo(node));
   }

   /**
    * Returns the info associated with an edge
    * @param edge is the edge to be analyzed
    * @return the associated property
    */
   inline const OpEdgeInfoConstRef CGetOpEdgeInfo(const EdgeDescriptor edge) const
   {
      return RefcountCast<const OpEdgeInfo>(graph::CGetEdgeInfo(edge));
   }

   /**
    * Returns the property associated with the graph
    * @return the property associated with the graph
    */
   inline OpGraphInfoRef GetOpGraphInfo()
   {
      return RefcountCast<OpGraphInfo>(GetGraphInfo());
   }

   /**
    * Returns the property associated with the graph
    * @return the graph property
    */
   inline const OpGraphInfoConstRef CGetOpGraphInfo() const
   {
      return RefcountCast<const OpGraphInfo>(CGetGraphInfo());
   }

   /**
    * Returns the property associated with the graph
    * @return the graph property
    */
   inline OpGraphInfoRef CGetOpGraphInfo()
   {
      return RefcountCast<OpGraphInfo>(GetGraphInfo());
   }

   /**
    * Given a set of vertices, this function computes the edges
    * which have the target in the set and the source outside: these edges are returned
    * in the form vertex,vertex. Actually the map returned is indexed by the source vertex
    * and contains the set of vertices, inside toCheck, which are targets of the edges
    * starting from the vertex key of the map.
    * @param toCheck is the set of vertices to be considered as taarget
    * @param edgeType is the type of edges to be considered
    */
   CustomUnorderedMap<vertex, OpVertexSet> GetSrcVertices(const OpVertexSet& toCheck, int edgeType) const;

   /**
    * Return the vertices belonging to the graph
    */
   const OpVertexSet CGetOperations() const;

   /**
    * Return the edge ingoing in a vertex
    * @param v is the vertex
    */
#if HAVE_UNORDERED
   boost::iterator_range<InEdgeIterator> CGetInEdges(const vertex v) const;
#else
   OpEdgeSet CGetInEdges(const vertex v) const;
#endif

   /**
    * Return the edge outgoing from a vertex
    * @param v is the vertex
    */
#if HAVE_UNORDERED
   boost::iterator_range<OutEdgeIterator> CGetOutEdges(const vertex v) const;
#else
   OpEdgeSet CGetOutEdges(const vertex v) const;
#endif
};
/// refcount definition of the class
typedef refcount<OpGraph> OpGraphRef;
typedef refcount<const OpGraph> OpGraphConstRef;

#endif
