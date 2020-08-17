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
 * @file function_behavior.hpp
 * @brief A brief description of the C++ Header File
 *
 * Here goes a detailed description of the file
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef FUNCTION_BEHAVIOR_HPP
#define FUNCTION_BEHAVIOR_HPP

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include <deque>      // for deque
#include <functional> // for binary_function
#include <iosfwd>     // for ostream, size_t
#include <typeindex>  // for hash

#include "custom_map.hpp"
#include "custom_set.hpp"

/// Behavior include (because of enums)
#if HAVE_EXPERIMENTAL
#include "epd_graph.hpp"
#include "parallel_regions_graph.hpp"
#endif
/// Graph include
#include "graph.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(BasicBlocksGraphConstructor);
REF_FORWARD_DECL(BBGraph);
CONSTREF_FORWARD_DECL(BBGraph);
REF_FORWARD_DECL(BBGraphsCollection);
REF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(EpdGraphsCollection);
REF_FORWARD_DECL(EpdGraph);
REF_FORWARD_DECL(extended_pdg_constructor);
REF_FORWARD_DECL(Loops);
REF_FORWARD_DECL(ParallelRegionsGraph);
REF_FORWARD_DECL(ParallelRegionsGraphConstructor);
REF_FORWARD_DECL(level_constructor);
REF_FORWARD_DECL(OpGraph);
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(OpGraphsCollection);
REF_FORWARD_DECL(operations_graph_constructor);
class OpVertexSet;
REF_FORWARD_DECL(ParallelRegionsGraphsCollection);
REF_FORWARD_DECL(Parameter);
#if HAVE_HOST_PROFILING_BUILT
CONSTREF_FORWARD_DECL(ProfilingInformation);
REF_FORWARD_DECL(ProfilingInformation);
#endif
REF_FORWARD_DECL(tree_node);
CONSTREF_FORWARD_DECL(Loops);

template <typename Graph>
class dominance;
class ParallelRegionsGraphsCollection;
class sequence_info;
class xml_element;
typedef unsigned int tree_class;
//@}

/// Struct representing memory information
struct memory_access
{
   unsigned int node_id;

   unsigned int base_address;

   unsigned int offset;

   memory_access(unsigned int _node_id, unsigned int _base_address, unsigned int _offset = 0);
};
typedef refcount<memory_access> memory_accessRef;

/// The access type to a variable
enum class FunctionBehavior_VariableAccessType
{
   UNKNOWN = 0,
   ADDRESS,
   USE,
   DEFINITION,
   OVER,
   ARG
};

/// The possible type of a variable
enum class FunctionBehavior_VariableType
{
   UNKNOWN = 0,
#if HAVE_EXPERIMENTAL
   AGGREGATE,
#endif
   MEMORY,
   SCALAR,
   VIRTUAL
};

#if NO_ABSEIL_HASH

/**
 * Definition of hash function for FunctionBehavior_VariableAccessType
 */
namespace std
{
   template <>
   struct hash<FunctionBehavior_VariableAccessType> : public unary_function<FunctionBehavior_VariableAccessType, size_t>
   {
      size_t operator()(FunctionBehavior_VariableAccessType variable_access_type) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(variable_access_type));
      }
   };
} // namespace std

/**
 * Definition of hash function for FunctionBehavior_VariableType
 */
namespace std
{
   template <>
   struct hash<FunctionBehavior_VariableType> : public unary_function<FunctionBehavior_VariableType, size_t>
   {
      size_t operator()(FunctionBehavior_VariableType variable_access_type) const
      {
         hash<int> hasher;
         return hasher(static_cast<int>(variable_access_type));
      }
   };
} // namespace std

#endif
/**
 *
 */
class FunctionBehavior
{
 private:
   /**
    * @name some friend classes
    */
   //@{
   friend class BasicBlocksProfiling;
   friend class BBCdgComputation;
   friend class OpCdgComputation;
   friend class hpp_profiling;
   friend class loops_computation;
   friend class instr_sequences_detection;
   friend struct loop_regions_computation;
   friend class LoopsAnalysisZebu;
   friend class LoopsProfiling;
   friend class probability_path;
   friend class HostProfiling;
   friend class read_profiling_data;
   friend class tp_profiling;
   friend class BBOrderComputation;
   friend class OpOrderComputation;
   friend class BasicBlocksCfgComputation;
   //@}

   /// Behavioral helper associated with this behavioral_graph_manager
   const BehavioralHelperRef helper;

   /// Global graph storing CFG, dominators and post-dominators. The nodes of this graph are basic blocks.
   const BBGraphsCollectionRef bb_graphs_collection;

   /// Global graph storing CFG, DFG, FCFG, FDFG, SDG, FSDG, CDG. The nodes of this graph are operations.
   const OpGraphsCollectionRef op_graphs_collection;

   /// The basic block CFG.
   const BBGraphRef bb;

   /// The basic block Control Flow Graph extended with edges that impose that basic block inside a loop are executed before what follows the loop
   const BBGraphRef extended_bb;

   /// The control dependence graph among basic blocks
   const BBGraphRef cdg_bb;

   /// The dj graph (used for loop computation)
   const BBGraphRef dj;

   /// The dominator tree of the CFG on basic blocks.
   const BBGraphRef dt;

   /// The basic block CFG with feedback
   const BBGraphRef fbb;

   /// The post-dominator tree of the CFG on basic blocks.
   const BBGraphRef pdt;

   /// The support basic block graph for path profiling
   const BBGraphRef ppg;

   /// The control flow graph.
   const OpGraphRef cfg;

   /// The extended control flow graph
   const OpGraphRef extended_cfg;

   /// The control flow graph with feedback.
   const OpGraphRef fcfg;

   /// The anti-dependencies graph.
   const OpGraphRef adg;

   /// The anti-dependencies graph with feedback
   const OpGraphRef fadg;

   /// The control dependence graph.
   const OpGraphRef cdg;

   /// The control dependence graph with feedback
   const OpGraphRef fcdg;

   /// The data flow graph.
   const OpGraphRef dfg;

   /// The data flow graph with feedback.
   const OpGraphRef fdfg;

   /// The flow edge operation graph
   const OpGraphRef flg;

   /// The output-dependencies flow graph.
   const OpGraphRef odg;

   /// The output-dependencies flow graph with feedback.
   const OpGraphRef fodg;

   /// The anti-dependence + data dependence + output dependence + flow graph
   const OpGraphRef flaoddg;

   /// The anti-dependence + data dependence + output dependence + flow graph with freedback
   const OpGraphRef fflaoddg;

   /// The system dependence, antidependence, output dependence and flow graph.
   const OpGraphRef flsaodg;

#ifndef NDEBUG
   /// The system dependence, antidependence, output dependence, flow and debug graph.
   const OpGraphRef flsaoddg;
#endif

   /// The system dependence, antidependence, output dependence and flow graph with feedback;
   const OpGraphRef fflsaodg;

   /// The system dependence, antidependence and output dependence graph.
   const OpGraphRef saodg;

   /// The system dependence, antidependence and output dependence graph with feedback.
   const OpGraphRef fsaodg;

   /// The system dependence graph
   const OpGraphRef sdg;

   /// The system dependence graph with feedback
   const OpGraphRef fsdg;

#if HAVE_EXPERIMENTAL
   /// The reduced program dependence graph
   const OpGraphRef rpdg;
#endif

   /// The speculation graph
   const OpGraphRef sg;

#if HAVE_EXPERIMENTAL
   /// bulk graph for extended PDG
   const EpdGraphsCollectionRef epdg_bulk;

   /// extended PDG with only control edges
   EpdGraphRef cepdg;

   /// extended PDG with only data edges
   EpdGraphRef depdg;

   /// extended PDG with only control and data edges
   EpdGraphRef cdepdg;

   /// extended PDG with only control, data edges and control flow
   EpdGraphRef cdcfepdg;

   /// extended PDG (acyclic version)
   EpdGraphRef epdg;

   /// extended PDG with feedback edges
   EpdGraphRef fepdg;

   /// represents activation functions (acyclic version)
   EpdGraphRef afg;

   /// represents activation functions
   EpdGraphRef fafg;

   /// bulk graph for the PRG
   const ParallelRegionsGraphsCollectionRef prg_bulk;

   /// parallel regions graph (Basic Block version)
   const ParallelRegionsGraphRef prg;
#endif

   /// Anti + Data flow graph on aggregates
   const OpGraphRef agg_virtualg;

#if HAVE_HOST_PROFILING_BUILT
   /// Profiling information about this function
   const ProfilingInformationRef profiling_information;
#endif

   /// Map operation vertex to position in topological order in control flow graph; in the sorting then part vertices come before else part ones
   std::map<vertex, unsigned int> map_levels;

   /// Map basic block vertex to position in topological order in control flow graph; in the sorting then part vertices come before else part ones
   std::map<vertex, unsigned int> bb_map_levels;

   /// list of operations vertices sorted by topological order in control flow graph; in the sorting then part vertices come before else part ones
   std::deque<vertex> deque_levels;

   /// list of operations vertices sorted by topological order in control flow graph; in the sorting then part vertices come before else part ones
   std::deque<vertex> bb_deque_levels;

   /// Loops of the function
   LoopsRef loops;

   /// this set represents the memory variables accessed by the function
   CustomOrderedSet<unsigned int> mem_nodeID;

   /// store memory objects which can be indirectly addressed through a dynamic address computation
   CustomOrderedSet<unsigned int> dynamic_address;

   /// this set represents the parameters that have to be copied from the caller
   CustomOrderedSet<unsigned int> parm_decl_copied;

   /// this set represents the actual parameters that has to be loaded into the formal parameter from the actual parameter
   CustomOrderedSet<unsigned int> parm_decl_loaded;

   /// this set represents the formal parameters that has to be stored into the formal parameter from the actual parameter
   CustomOrderedSet<unsigned int> parm_decl_stored;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// the function dereference a pointer initialized with constant address.
   bool dereference_unknown_address;

   /// true when at least one pointer conversion happen
   bool pointer_type_conversion;

   bool unaligned_accesses;

   /// The version of basic block intermediate representation
   unsigned int bb_version;

   /// Version of the bitvalue information
   unsigned int bitvalue_version;

   /// when true at least one global variable is used
   bool has_globals;

   /// function calls undefined function passing pointers
   bool has_undefined_function_receiveing_pointers;

   /// set of global variables
   CustomOrderedSet<unsigned int> state_variables;

   /// true when pipelining is enabled for the function
   bool pipeline_enabled;

   /// true when the requested pipeline does not include unbounded functions
   bool simple_pipeline;

   /// used only for stallable pipelines
   int initiation_time;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param _helper is the helper associated with the function
    * @param parameters is the set of input paramters
    */
   FunctionBehavior(const application_managerConstRef AppM, const BehavioralHelperRef _helper, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~FunctionBehavior();

   /**
    * Declaration of enum representing the type of graph
    */
   enum graph_type
   {
      CFG,     /**< Control flow graph */
      ECFG,    /**< Extended control flow graph */
      FCFG,    /**< Control flow graph with feedback */
      CDG,     /**< Control dependence graph */
      FCDG,    /**< Control dependence graph with feedback */
      DFG,     /**< Data flow graph */
      FDFG,    /**< Data flow graph with feedback */
      ADG,     /**< Anti-dependence graph */
      FADG,    /**< Anti-dependence graph with feedback */
      ODG,     /**< Output dependence graph */
      FODG,    /**< Output dependence graph with feedback */
      FLG,     /**< Flow edges graph */
      SDG,     /**< System dependence graph */
      FSDG,    /**< System dependence graph with feedback */
      SAODG,   /**< System dependence + anti-dependence graph + output dependence graph */
      FSAODG,  /**< System dependence + anti-dependence graph + output dependence graph with feedback */
      FLSAODG, /**< System dependence + anti-dependence + output dependence graph + flow graph */
#ifndef NDEBUG
      FLSAODDG, /**< System dependence + anti-dependence + output dependence graph + flow graph + debug graph*/
#endif
      FFLSAODG, /**< System dependence + anti-dependence + output dependence graph + flow graph with feedback */
      FLAODDG,  /**< Anti-dependence + data dependence + output dependence + flow graph */
      FFLAODDG, /**< Anti dependence + data dependence + output dependence + flow graph with feedback */
      SG,       /**< Speculation graph */
#if HAVE_EXPERIMENTAL
      RPDG, /**< Reduced Program Dependence graph */
#endif
      AGG_VIRTUALG /**< Anti + Data flow graph dependence between aggregates */
   };

   /**
    * Declaration of enum representing the type of bb_graph
    */
   enum bb_graph_type
   {
      BB,            /**< Basic block control flow graph */
      FBB,           /**< Basic block control flow graph with feedback*/
      EBB,           /**< Basic block control flow graph with edges imposing that basic block inside a loop are executed before what follows the loop*/
      CDG_BB,        /**< Basic block control dependence graph */
      DOM_TREE,      /**< Basic block dominator tree */
      POST_DOM_TREE, /**< Basic block post-dominator tree */
      PPG,           /**< Support basic block for path profiling */
      DJ             /**< DJ basic block graph (used for loop computation) */
   };

   /// Mutual exclusion between basic blocks (based on control flow graph with flow edges)
   CustomUnorderedMapStable<vertex, CustomUnorderedSet<vertex>> bb_reachability;

   /// Reachability between basic blocks based on control flow graph with feedback
   CustomUnorderedMapStable<vertex, CustomUnorderedSet<vertex>> feedback_bb_reachability;

   /// reference to the operations graph constructor
   const operations_graph_constructorRef ogc;

   /// reference to the basic block graph constructor
   const BasicBlocksGraphConstructorRef bbgc;

#if HAVE_EXPERIMENTAL
   /// reference to the extended PDG constructor
   const extended_pdg_constructorRef epdgc;

   /// reference to the basic block PRG constructor
   const ParallelRegionsGraphConstructorRef prgc;
#endif

   /// reference to the level constructor
   const level_constructorRef lm;

   /// reference to the level constructor
   const level_constructorRef bb_lm;

   /// This class stores dominator information.
   dominance<BBGraph>* dominators;

   /// This class stores post-dominator information.
   dominance<BBGraph>* post_dominators;

   /// map between node id and the corresponding memory allocation
   std::map<unsigned int, memory_accessRef> memory_info;

   /// True when there access to packed data
   bool packed_vars;

   /**
    * Returns the helper associated with the function
    */
   BehavioralHelperRef GetBehavioralHelper();

   /**
    * Returns the helper associated with the function
    */
   const BehavioralHelperConstRef CGetBehavioralHelper() const;

   /**
    * Returns the set of local variables
    */
   CustomOrderedSet<unsigned int> get_local_variables(const application_managerConstRef AppM) const;

   /**
    * Return the vector of vertex index sorted in topological order.
    */
   const std::deque<vertex>& get_levels() const;

   /**
    * Return the vector of bb vertex index sorted in topological order.
    */
   const std::deque<vertex>& get_bb_levels() const;

   /**
    * Return the map of vertex index sorted in topological order.
    */
   const std::map<vertex, unsigned int>& get_map_levels() const;

   /**
    * Return the map of bb vertex index sorted in topological order.
    */
   const std::map<vertex, unsigned int>& get_bb_map_levels() const;

   /**
    * This method returns the operation graphs.
    * @param gt is the type of the graph to be returned
    * @return the pointer to the graph.
    */
   OpGraphRef GetOpGraph(FunctionBehavior::graph_type gt);

   /**
    * This method returns the operation graphs.
    * @param gt is the type of the graph to be returned
    * @return the pointer to the graph.
    */
   const OpGraphConstRef CGetOpGraph(FunctionBehavior::graph_type gt) const;

   /**
    * This method returns the operation graph having as vertices the vertices of subset
    * @param gt is the type of the graph to be returned
    * @param subset is the set of subgraph vertices
    * @return the refcount to the subgraph
    */
   const OpGraphConstRef CGetOpGraph(FunctionBehavior::graph_type gt, const OpVertexSet& subset) const;

   /**
    * This method returns the basic block graphs.
    * @param gt is the type of the bb_graph to be returned
    * @return the pointer to the bb_graph.
    */
   BBGraphRef GetBBGraph(FunctionBehavior::bb_graph_type gt = FunctionBehavior::BB);

   /**
    * This method returns the basic block graphs.
    * @param gt is the type of the bb_graph to be returned
    * @return the pointer to the bb_graph.
    */
   const BBGraphConstRef CGetBBGraph(FunctionBehavior::bb_graph_type gt = FunctionBehavior::BB) const;

#if HAVE_EXPERIMENTAL
   /**
    * This method returns the extended PDG graph.
    * @param type is the type of the graph to be returned
    * @return the corresponding EpdGraph.
    */
   EpdGraphRef GetEpdGraph(EpdGraph::Type type);

   /**
    * This method returns the extended PDG graph.
    * @param type is the type of the graph to be returned
    * @return the corresponding EpdGraph.
    */
   const EpdGraphRef CGetEpdGraph(EpdGraph::Type type) const;

   /**
    * This method returns the PRG.
    * @param type is the type of the graph to be returned
    * @return the pointer to the corresponding ParallelRegionsGraph.
    */
   const ParallelRegionsGraphRef GetPrgGraph(ParallelRegionsGraph::Type type) const;
#endif

   /**
    * Function that prints the class behavioral_manager.
    */
   void print(std::ostream& os) const;

   /**
    * Set epp associated with an edges
    * @param e is the edges
    * @param value is the value
    */
   void set_epp(EdgeDescriptor e, unsigned long long value);

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    */
   friend std::ostream& operator<<(std::ostream& os, const FunctionBehavior& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Friend definition of the << operator. Pointer version.
    * @param os is the output stream
    */
   friend std::ostream& operator<<(std::ostream& os, const FunctionBehaviorRef& s)
   {
      if(s)
         s->print(os);
      return os;
   }

   /**
    * Return the loops
    */
   const LoopsConstRef CGetLoops() const;

   /**
    * Return the loops
    */
   const LoopsRef GetLoops() const;

#if HAVE_HOST_PROFILING_BUILT
   /**
    * Return the profiling information
    * @return the profiling information
    */
   const ProfilingInformationConstRef CGetProfilingInformation() const;
#endif

   /**
    * Adds an identifier to the set of memory variables
    */
   void add_parm_decl_copied(unsigned int node_id);

   /**
    * add an actual parameter that has to be loaded into the formal parameter
    */
   void add_parm_decl_loaded(unsigned int node_id);

   /**
    * add a formal parameter that has to be initialized from the actual value
    */
   void add_parm_decl_stored(unsigned int node_id);

   /**
    * Adds a memory variable to the function
    */
   void add_function_mem(unsigned int node_id);

   /**
    * Add the node_id to the set of object for which a dynamic address computation could be used
    * @param node_id is the object stored in memory
    */
   void add_dynamic_address(unsigned int node_id);

   /**
    * remove a variable from the dynamic address set
    * @param node_id is the object stored in memory
    */
   void erase_dynamic_address(unsigned int node_id);

   /**
    * remove all variables from the dynamic address set
    */
   void erase_all_dynamic_addresses();

   /**
    * Checks if a variable has been allocated in memory
    */
   bool is_variable_mem(unsigned int node_id) const;

   /**
    * Returns the set of memory variables
    */
   const CustomOrderedSet<unsigned int>& get_function_mem() const;

   /**
    * Returns the set of variables for which a dynamic address computation maybe required
    */
   const CustomOrderedSet<unsigned int>& get_dynamic_address() const;

   /**
    * Returns the set of parameters to be copied
    */
   const CustomOrderedSet<unsigned int>& get_parm_decl_copied() const;

   /**
    * Returns the set of the actual parameters that has to be loaded into the formal parameter
    */
   const CustomOrderedSet<unsigned int>& get_parm_decl_loaded() const;

   /**
    * Returns the set of the formal parameters that has to be stored into the formal parameter
    */
   const CustomOrderedSet<unsigned int>& get_parm_decl_stored() const;

   /**
    * Set the use of dereferences of unknown address.
    */
   inline void set_dereference_unknown_addr(bool f)
   {
      dereference_unknown_address = f;
   }

   /**
    * Return true if the function has dereferences of unknown address.
    */
   inline bool get_dereference_unknown_addr() const
   {
      return dereference_unknown_address;
   }

   /**
    * Set if a pointer conversion happen
    */
   inline void set_pointer_type_conversion(bool f)
   {
      pointer_type_conversion = f;
   }

   /**
    * Return true if a pointer conversion happen
    */
   inline bool get_pointer_type_conversion() const
   {
      return pointer_type_conversion;
   }

   /**
    * Set if LOADs or STOREs perform unaligned accesses
    */
   inline void set_unaligned_accesses(bool f)
   {
      unaligned_accesses = f;
   }

   /**
    * Return true if a LOADs or STOREs perform unaligned accesses
    */
   inline bool get_unaligned_accesses() const
   {
      return unaligned_accesses;
   }

   /**
    * set if there are or not externally visible global variables
    * @param f boolean value specifying if there exist at least one externally visible global variable
    */
   inline void set_has_globals(bool f)
   {
      has_globals = f;
   }

   /**
    * helper for global variables property
    * @return true in case there exist at least one externally visible global variable
    */
   inline bool get_has_globals() const
   {
      return has_globals;
   }

   /**
    * set if there are or not undefined functions called and which has pointers passed
    * @param f boolean value specifying if there are such functions
    */
   inline void set_has_undefined_function_receiveing_pointers(bool f)
   {
      has_undefined_function_receiveing_pointers = f;
   }

   /**
    * helper for has_undefined_function_receiveing_pointers variables property
    * @return true in case there are undefined function which receives pointers as a parameter
    */
   inline bool get_has_undefined_function_receiving_pointers() const
   {
      return has_undefined_function_receiveing_pointers;
   }

   /**
    * Add a state variable: static, global or volatile variable
    */
   void add_state_variable(unsigned int node_id)
   {
      state_variables.insert(node_id);
   }

   /**
    * return true if a variable is a state variable or not
    * @param node_id is the node id of the variable
    */
   bool is_a_state_variable(unsigned int node_id) const
   {
      return state_variables.find(node_id) != state_variables.end();
   }

   /**
    * @brief update the the packed variables status
    * @param packed is true when there is at least one packed variables
    */
   void set_packed_vars(bool packed)
   {
      packed_vars = packed_vars || packed;
   }

   /**
    * @return true in case packed vars are used
    */
   bool has_packed_vars() const
   {
      return packed_vars;
   }

   bool is_pipelining_enabled() const
   {
      return pipeline_enabled;
   }

   bool build_simple_pipeline() const
   {
      if(simple_pipeline)
      {
         THROW_ASSERT(pipeline_enabled, "Simple pipeline is true but pipeline is not enabled");
      }
      return simple_pipeline;
   }

   int get_initiation_time() const
   {
      THROW_ASSERT(pipeline_enabled && !simple_pipeline, "Should not request initiation time when pipeline is not enabled or simple pipeline is requested");
      return initiation_time;
   }

   /**
    * Check if a path from first_operation to second_operation exists in control flow graph (without feedback)
    * @param first_operation is the first operation to be considered
    * @param second_operation is the second operation to be considered
    * @return true if there is a path from first_operation to second_operation in flcfg
    */
   bool CheckReachability(const vertex first_operation, const vertex second_operation) const;

   /**
    * Check if a path from the first basic block to the second basic block exists in control flow graph (without feedback)
    * @param first_basic_block is the first basic block to be considered
    * @param second_basic_block is the second operation to be considered
    * @return true if there is a path from first_basic_block to second_basic_block in flcfg
    */
   bool CheckBBReachability(const vertex first_operation, const vertex second_operation) const;

   /**
    * Check if a path from first_operation to second_operation exists in control flow graph with feedback
    * @param first_operation is the first operation to be considered
    * @param second_operation is the second operation to be considered
    * @return true if there is a path from first_operation to second_operation in fcfg
    */
   bool CheckFeedbackReachability(const vertex first_operation, const vertex second_operation) const;

   /**
    * Check if a path from the first basic block to the second basic block exists in control flow graph with feedback
    * @param first_basic_block is the first basic block to be considered
    * @param second_basic_block is the second operation to be considered
    * @return true if there is a path from first_basic_block to second_basic_block in flcfg
    */
   bool CheckBBFeedbackReachability(const vertex first_operation, const vertex second_operation) const;

   /**
    * Return the version of the basic block intermediate representation
    * @return bb_version
    */
   unsigned int GetBBVersion() const;

   /**
    * Update the version of the basic block intermediate representation
    * @return the new version
    */
   unsigned int UpdateBBVersion();

   /**
    * Return the version of the bitvalue information
    * @return bitvalue_version
    */
   unsigned int GetBitValueVersion() const;

   /**
    * Update the version of the bitvalue information
    * @return the new version
    */
   unsigned int UpdateBitValueVersion();
};

typedef refcount<FunctionBehavior> FunctionBehaviorRef;
typedef refcount<const FunctionBehavior> FunctionBehaviorConstRef;

/**
 * The key comparison function for vertices set based on levels
 */
class op_vertex_order_by_map : std::binary_function<vertex, vertex, bool>
{
 private:
   /// Topological sorted vertices
   const std::map<vertex, unsigned int>& ref;

/// Graph
#if HAVE_ASSERTS
   const graph* g;
#endif

 public:
   /**
    * Constructor
    * @param ref_ is the map with the topological sort of vertices
    * @param g_ is a graph used only for debugging purpose to print name of vertex
    */
   op_vertex_order_by_map(const std::map<vertex, unsigned int>& ref_, const graph*
#if HAVE_ASSERTS
                                                                          g_)
       : ref(ref_), g(g_)
#else
                          )
       : ref(ref_)
#endif
   {
   }

   /**
    * Compare position of two vertices in topological sorted
    * @param x is the first vertex
    * @param y is the second vertex
    * @return true if x precedes y in topological sort, false otherwise
    */
   bool operator()(const vertex x, const vertex y) const;
};

#endif
