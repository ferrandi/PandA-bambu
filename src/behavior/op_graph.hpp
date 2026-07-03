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
 * @file op_graph.hpp
 * @brief Data structures used in operations graph
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef OP_GRAPH_HPP
#define OP_GRAPH_HPP
#include "cdfg_edge_info.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "graph.hpp"
#include "graph_info.hpp"
#include "refcount.hpp"
#include "typed_node_info.hpp"

#include <boost/graph/graph_traits.hpp>

#include <iosfwd>
#include <limits>
#include <list>
#include <set>
#include <string>

#include "config_HAVE_HLS_BUILT.hpp"
#include "config_HAVE_UNORDERED.hpp"

class OpGraph;
class OpGraphsCollection;
class OpVertexSet;
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(hls);
REF_FORWARD_DECL(ir_node);

/// constant used to represent IR node index of entry operation
#define ENTRY_ID (std::numeric_limits<unsigned int>::max())
/// constant used to represent IR node index of exit operation
#define EXIT_ID (std::numeric_limits<unsigned int>::max() - 1)

/**
 * constant identifying a node of opaque type
 */
#define TYPE_OPAQUE 1 << 0

/**
 * constant identifying the node type of a EXTERNAL operation (a function call)
 */
#define TYPE_EXTERNAL 1 << 4

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
 * A vertex is of type TYPE_WAS_PHI_STMT when it is comes from a split of phi nodes
 */
#define TYPE_WAS_PHI_STMT 1 << 22

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
 * Constant identifying if a TYPE_EXTERNAL write  or read memory
 */
#define TYPE_RW 1 << 28

/**
 * constant string identifying the operation performed by an assignment.
 */
#define ASSIGN "ASSIGN"

/**
 * constant string identifying the operation performed by an extract_bit_node.
 */
#define EXTRACT_BIT_NODE "extract_bit_node"

/**
 * constant string identifying the operation performed by an extract_bit_node.
 */
#define LUT_NODE "lut_node"

/**
 * constant string identifying the operation performed by a MULTI_READ_COND.
 */
#define MULTI_READ_COND "MULTI_READ_COND"

/**
 * constant string identifying a no operation. Only used for operations associated with empty basic blocks.
 */
#define NOP "NOP"

/**
 * constant string identifying the operation performed by a return_stmt.
 */
#define RETURN_STMT "return_stmt"

/**
 * constant string identifying the operation performed by a return_stmt.
 */
#define NOP_STMT "nop_stmt"

/**
 * constant string identifying the operation performed by a phi_stmt.
 */
#define PHI_STMT "phi_stmt"

/**
 * constant string identifying the operation performed when two objects are memcopied.
 */
#define MEMCPY "memcpy"

/**
 * constant string identifying the operation performed when two objects are memcompared.
 */
#define MEMCMP "memcmp"

/**
 * constant string identifying the operation performed when two objects are memsetted.
 */
#define MEMSET "memset"

/**
 * constant string identifying the operation performed when a vector concatenation is considered.
 */
#define VECT_CONCATENATION "VECT_CONCATENATION"

/**
 * constant string identifying the addressing operation.
 */
#define ADDR_NODE "addr_node"

/**
 * constant string identifying some conversion expressions
 */
#define NOP_NODE "nop_node"

/**
 * constant string identifying integer to float conversions
 */
#define ITOFP_NODE "itofp_node"

/**
 * constant string identifying float to integer conversions
 */
#define FPTOI_NODE "fptoi_node"

/**
 * constant string identifying view convert expressions
 */
#define BITCAST_NODE "bitcast_node"

/// constant defining the builtin wait call intrinsic function
#define BUILTIN_WAIT_CALL "__builtin_wait_call"

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
#define FSADG_SELECTOR \
   (CDG_SELECTOR | DFG_SELECTOR | ADG_SELECTOR | FB_CDG_SELECTOR | FB_DFG_SELECTOR | FB_ADG_SELECTOR)
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

/// The access type to a variable
enum class VariableAccessType : int
{
   UNKNOWN = 0,
   ADDRESS,
   USE,
   DEFINITION,
   OVER,
   ARG
};

/// The possible type of a variable
enum class VariableType : int
{
   UNKNOWN = 0,
   MEMORY,
   SCALAR,
   VIRTUAL
};

/**
 * Information associated with a generic operation node.
 */
struct OpNodeInfo : public TypedNodeInfo
{
   /// set of cited variables (i.e., variables which are included in the c printing of this statement)
   CustomSet<unsigned int> cited_variables;

   /// Set of actual parameters of called function (used in pthread backend
   std::list<unsigned int> actual_parameters;

   /// The IR node associated with this vertex
   ir_nodeRef node;

   /// Store the index of called functions
   CustomSet<unsigned int> called;

   /// Store the index of the basic block which this operation vertex belongs to
   unsigned int bb_index;

   /// Store the index of the control equivalent region
   unsigned int cer;

   OpNodeInfo();

   /**
    * Initialize variable maps
    */
   void Initialize();

   void AddVariable(VariableType variable_type, VariableAccessType access_type, unsigned int var);

   /**
    * Return a set of accessed scalar variables
    * @param variable_type is the type of variables to be considered
    * @param access_type is the type of accesses to be considered
    */
   const CustomSet<unsigned int>& getVariables(VariableType variable_type, VariableAccessType access_type) const;

   /**
    * Return the operation associated with the vertex
    */
   const std::string GetOperation() const;

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

 private:
   /// set of scalar ssa accessed in this node
   std::vector<CustomSet<unsigned int>> variables;
};

/**
 * The info associated with an edge of operation graph
 */
struct OpEdgeInfo : public CdfgEdgeInfo
{
};

/**
 * information associated with the whole graph
 */
struct OpGraphInfo : public GraphInfo
{
   /// Index identifying the entry vertex
   gc_vertex_descriptor entry_vertex{gc_null_vertex()};

   /// Index identifying the exit vertex
   gc_vertex_descriptor exit_vertex{gc_null_vertex()};

   /// The behavioral helper
   BehavioralHelperConstRef BH{nullptr};

   /// For each statement, the vertex in which it is contained
   CustomMap<unsigned int, gc_vertex_descriptor> ir_node_to_operation;

   /// For each ssa var, the vertex defining it
   CustomMap<unsigned int, gc_vertex_descriptor> SSA2Def;

   OpGraphInfo() = default;

   /**
    * Constructor
    * @param _BH is the helper of the function associated with this graph
    */
   OpGraphInfo(const BehavioralHelperConstRef& _BH) : BH(_BH)
   {
   }

   void clear();
};

#if HAVE_UNORDERED
/**
 * A set of operation vertices
 */
class OpVertexSet : public CustomUnorderedSet<gc_vertex_descriptor>
{
 public:
   explicit OpVertexSet(const OpGraphsCollection* op_graph);
};

/**
 * Map from operation vertices to value
 */
template <typename value>
class OpVertexMap : public CustomUnorderedMap<gc_vertex_descriptor, value>
{
 public:
   explicit OpVertexMap(const OpGraphsCollection*) : CustomUnorderedMap<gc_vertex_descriptor, value>()
   {
   }
};

class OpEdgeSet : public CustomUnorderedSet<gc_edge_descriptor>
{
 public:
   explicit OpEdgeSet(const OpGraphsCollection* op_graph);
};
#else
class OpVertexSorter
{
   /// The operation graph to which vertices belong
   /// Note: this should be const, but can not because of assignment operator
   const OpGraphsCollection* op_graph;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to which vertices belong
    */
   OpVertexSorter(const OpGraphsCollection* op_graph);

   /**
    * Compare position of two vertices
    * @param x is the first step
    * @param y is the second step
    * @return true if x is necessary and y is unnecessary
    */
   bool operator()(gc_vertex_descriptor x, gc_vertex_descriptor y) const;
};

/**
 * A set of operation vertices
 */
class OpVertexSet : public std::set<gc_vertex_descriptor, OpVertexSorter>
{
 public:
   explicit OpVertexSet(const OpGraphsCollection* op_graph);
};

/**
 * Map from operation vertices to value
 */
template <typename value>
class OpVertexMap : public std::map<gc_vertex_descriptor, value, OpVertexSorter>
{
 public:
   explicit OpVertexMap(const OpGraphsCollection* op_graph)
       : std::map<gc_vertex_descriptor, value, OpVertexSorter>(OpVertexSorter(op_graph))
   {
   }
};

class OpEdgeSorter
{
 private:
   /// The operation graph to which vertices belong
   /// Note: this should be const, but can not because of assignment operator
   const OpGraphsCollection* op_graph;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to which vertices belong
    */
   explicit OpEdgeSorter(const OpGraphsCollection* op_graph);

   /**
    * Compare position of two edges
    * @param x is the first edge
    * @param y is the second edge
    * @return true if x < y
    */
   bool operator()(const gc_edge_descriptor& x, const gc_edge_descriptor& y) const;
};

class OpEdgeSet : public std::set<gc_edge_descriptor, OpEdgeSorter>
{
 public:
   explicit OpEdgeSet(const OpGraphsCollection* op_graph);
};
#endif

/**
 * This structure defines graphs where nodes are operations
 */
class OpGraphsCollection : public graphs_collection<OpNodeInfo, OpEdgeInfo, OpGraphInfo>
{
 protected:
   /// The set of operations
   OpVertexSet operations;

 public:
   /**
    * Empty Constructror
    * @param info is the info associated with the graph
    */
   OpGraphsCollection(const OpGraphInfo& info);

   vertex_descriptor AddVertex(const OpNodeInfo& info) override;

   /**
    * Remove a vertex from this graph
    * @param v is the vertex to be removed
    */
   void RemoveVertex(vertex_descriptor v) override;

   /**
    * Return the vertices belonging to the graph
    */
   OpVertexSet CGetOperations() const;

   /**
    * Remove all the edges and vertices from the graph
    */
   void Clear();
};

/**
 * Class used to describe a particular graph with operations as nodes
 */
struct OpGraph : public graph<OpGraphsCollection>
{
   /// Friend declaration of schedule to allow dot writing
   friend class Schedule;

 public:
   /**
    * Standard constructor.
    * @param _op_graphs_collection is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    */
   OpGraph(const OpGraphsCollection& _op_graphs_collection, int selector);

   /**
    * Sub-graph constructor.
    * @param _op_graphs_collection is the bulk graph.
    * @param selector is the selector used to filter the bulk graph.
    * @param sub is the set of vertices on which the graph is filtered.
    */
   OpGraph(const OpGraphsCollection& _op_graphs_collection, int selector,
           const CustomUnorderedSet<vertex_descriptor>& sub);

   /**
    * Writes this graph in dot format
    * @param file_name is the file where the graph has to be printed
    * @param detail_level is the detail level of the printed graph
    */
   void writeDot(const std::filesystem::path& file_name, const int detail_level = 0) const;

#if HAVE_HLS_BUILT
   /**
    * Write this graph in dot format with timing information
    * @param file_name is the file where the graph has to be printed
    * @param HLS is the high level synthesis structure
    * @param critical_paths is the set of operations belonging to critical paths
    */
   void writeDot(const std::filesystem::path& file_name, const hlsConstRef& HLS,
                 const CustomSet<unsigned int>& critical_paths) const;
#endif

   /**
    * Given a set of vertices, this function computes the edges
    * which have the target in the set and the source outside: these edges are returned
    * in the form vertex,vertex. Actually the map returned is indexed by the source vertex
    * and contains the set of vertices, inside toCheck, which are targets of the edges
    * starting from the vertex key of the map.
    * @param toCheck is the set of vertices to be considered as taarget
    * @param edgeType is the type of edges to be considered
    */
   CustomUnorderedMap<vertex_descriptor, OpVertexSet> GetSrcVertices(const OpVertexSet& toCheck, int edgeType) const;

   /**
    * Return the vertices belonging to the graph
    */
   OpVertexSet CGetOperations() const;

   /**
    * Return the edge ingoing in a vertex
    * @param v is the vertex
    */
#if HAVE_UNORDERED
   boost::iterator_range<InEdgeIterator> CGetInEdges(vertex_descriptor v) const;
#else
   OpEdgeSet CGetInEdges(vertex_descriptor v) const;
#endif

   /**
    * Return the edge outgoing from a vertex
    * @param v is the vertex
    */
#if HAVE_UNORDERED
   boost::iterator_range<OutEdgeIterator> CGetOutEdges(vertex_descriptor v) const;
#else
   OpEdgeSet CGetOutEdges(vertex_descriptor v) const;
#endif
};

struct OpVertexWriter : public VertexWriter<OpGraph>
{
   /**
    * Constructor
    * @param operation_graph is the operation graph
    * @param detail_level is the level of detail:
    *  0 - print operation
    *  1 - print operation and accessed variables
    */
   OpVertexWriter(const OpGraph& operation_graph, const int detail_level);

   virtual ~OpVertexWriter() override = default;

   virtual void operator()(std::ostream& out, OpGraph::vertex_descriptor v) const override;
};

/**
 * Edge writer for operation graph
 */
struct OpEdgeWriter : public EdgeWriter<OpGraph>
{
   /**
    * Constructor
    * @param operation_graph is the operation graph
    */
   OpEdgeWriter(const OpGraph& operation_graph);

   virtual ~OpEdgeWriter() override = default;

   virtual void operator()(std::ostream& out, const OpGraph::edge_descriptor& e) const override;
};

#if HAVE_HLS_BUILT
class TimedOpVertexWriter : public OpVertexWriter
{
 protected:
   /// The HLS data structure
   const hlsConstRef HLS;

   /// The set of operations belonging to critical_paths
   CustomSet<unsigned int> critical_paths;

 public:
   /**
    * Constructor
    * @param op_graph is the operation graph to be printed
    * @param HLS is the HLS data structure
    * @param critical_paths is the set of operations belonging to critical paths
    */
   TimedOpVertexWriter(const OpGraph& op_graph, const hlsConstRef HLS, CustomSet<unsigned int> critical_paths);

   void operator()(std::ostream& out, OpGraph::vertex_descriptor v) const override;
};

class TimedOpEdgeWriter : public OpEdgeWriter
{
 protected:
   /// The HLS data structure
   const hlsConstRef HLS;

   /// The set of operations belonging to critical_paths
   CustomSet<unsigned int> critical_paths;

 public:
   /**
    * Constructor
    * @param _g is the operation graph
    * @param HLS is the HLS data structure
    * @param critical_paths is the set of operations belonging to critical paths
    */
   TimedOpEdgeWriter(const OpGraph& operation_graph, const hlsConstRef HLS, CustomSet<unsigned int> critical_paths);

   void operator()(std::ostream& out, const OpGraph::edge_descriptor& e) const override;
};
#endif

/// Helpers

inline const CustomSet<unsigned int>& getVariables(const OpGraph& data, OpGraph::vertex_descriptor op,
                                                   const VariableType variable_type,
                                                   const VariableAccessType access_type)
{
   return data.CGetNodeInfo(op).getVariables(variable_type, access_type);
}
inline const CustomSet<unsigned int>& getVariablesScalarDef(const OpGraph& data, OpGraph::vertex_descriptor op)
{
   return data.CGetNodeInfo(op).getVariables(VariableType::SCALAR, VariableAccessType::DEFINITION);
}
inline const CustomSet<unsigned int>& getVariablesScalarUse(const OpGraph& data, OpGraph::vertex_descriptor op)
{
   return data.CGetNodeInfo(op).getVariables(VariableType::SCALAR, VariableAccessType::USE);
}

OpGraph::vertex_descriptor getDefOp(const OpGraph& data, unsigned int var);

#endif
