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
 * @file function_behavior.hpp
 * @brief A brief description of the C++ Header File
 *
 * Here goes a detailed description of the file
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef FUNCTION_BEHAVIOR_HPP
#define FUNCTION_BEHAVIOR_HPP

#include "algorithms/loops_detection/loops_fwd.hpp"
#include "basic_block.hpp"
#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_HOST_PROFILING_BUILT.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "graph.hpp"
#include "operations_graph_constructor.hpp"
#include "refcount.hpp"
#include <deque>
#include <iosfwd>
#include <typeindex>

class BasicBlocksGraphConstructor;
class OpGraph;
class OpGraphsCollection;
class OpVertexSet;
CONSTREF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(BehavioralHelper);
CONSTREF_FORWARD_DECL(OMPInfo);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(EpdGraph);
REF_FORWARD_DECL(EpdGraphsCollection);
REF_FORWARD_DECL(extended_pdg_constructor);
REF_FORWARD_DECL(FunctionBehavior);
REF_FORWARD_DECL(OMPInfo);
REF_FORWARD_DECL(ParallelRegionsGraph);
REF_FORWARD_DECL(ParallelRegionsGraphConstructor);
REF_FORWARD_DECL(ParallelRegionsGraphsCollection);
REF_FORWARD_DECL(ir_node);

#if HAVE_HOST_PROFILING_BUILT
CONSTREF_FORWARD_DECL(ProfilingInformation);
REF_FORWARD_DECL(ProfilingInformation);
#endif

template <typename Graph, bool ComputePostDominators>
class dominance;
class ParallelRegionsGraphsCollection;
class sequence_info;
class xml_element;
enum class MemoryAllocation_ChannelsType;
enum class MemoryAllocation_Policy;
using ir_class = unsigned int;

/// Struct representing memory information
struct memory_access
{
   unsigned int node_id;

   unsigned int base_address;

   unsigned int offset;

   memory_access(unsigned int _node_id, unsigned int _base_address, unsigned int _offset = 0);
};
using memory_accessRef = refcount<memory_access>;

struct OMPInfo
{
   const unsigned int fork_call_id;
   const unsigned int ncore;
   const unsigned int core_id;
   unsigned int kmp_t_nproc;
   const unsigned int context_count;
   unsigned long long int mem_page_size;
   std::stack<unsigned int> critical;
   const unsigned int local_idx;
   const unsigned int global_idx;
   const OMPInfoConstRef parent;

   OMPInfo(unsigned int _local_idx, unsigned int _context_count, unsigned int _core_id, unsigned int _ncore,
           unsigned int _fork_call_id, const OMPInfoConstRef& parent, unsigned int _kmp_t_nproc);

   static unsigned int make_global(unsigned int idx, unsigned int ncore, OMPInfoConstRef parent);
};

/**
 *
 */
class FunctionBehavior
{
   friend class BasicBlocksCfgComputation;
   friend class BasicBlocksProfiling;
   friend class BBCdgComputation;
   friend class BBOrderComputation;
   friend class HostProfiling;
   friend class instr_sequences_detection;
   friend class loops_computation;
   friend class OpCdgComputation;
   friend class OpOrderComputation;
   friend struct loop_regions_computation;

   /// Behavioral helper associated with this behavioral_graph_manager
   const BehavioralHelperRef helper;

   /// Global graph storing CFG, dominators and post-dominators. The nodes of this graph are basic blocks.
   const std::unique_ptr<BBGraphsCollection> bb_graphs_collection;

   /// Global graph storing CFG, DFG, FCFG, FDFG, SDG, FSDG, CDG. The nodes of this graph are operations.
   const std::unique_ptr<OpGraphsCollection> op_graphs_collection;

#if HAVE_HOST_PROFILING_BUILT
   /// Profiling information about this function
   const ProfilingInformationRef profiling_information;
#endif

   /// Map operation vertex to position in topological order in control flow graph; in the sorting then part vertices
   /// come before else part ones
   std::map<gc_vertex_descriptor, unsigned int> map_levels;

   /// Map basic block vertex to position in topological order in control flow graph; in the sorting then part vertices
   /// come before else part ones
   std::map<gc_vertex_descriptor, unsigned int> bb_map_levels;

   /// list of operations vertices sorted by topological order in control flow graph; in the sorting then part vertices
   /// come before else part ones
   std::deque<gc_vertex_descriptor> deque_levels;

   /// list of operations vertices sorted by topological order in control flow graph; in the sorting then part vertices
   /// come before else part ones
   std::deque<gc_vertex_descriptor> bb_deque_levels;

   /// Loops of the function
   LoopsRef loops;

   /// this set represents the memory variables accessed by the function
   CustomOrderedSet<unsigned int> mem_nodeID;

   /// store memory objects which can be indirectly addressed through a dynamic address computation
   CustomOrderedSet<unsigned int> dynamic_address;

   /// this set represents the parameters that have to be copied from the caller
   CustomOrderedSet<unsigned int> parm_decl_copied;

   /// this set represents the actual parameters that has to be loaded into the formal parameter from the actual
   /// parameter
   CustomOrderedSet<unsigned int> parm_decl_loaded;

   /// this set represents the formal parameters that has to be stored into the formal parameter from the actual
   /// parameter
   CustomOrderedSet<unsigned int> parm_decl_stored;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// the function dereference a pointer initialized with constant address.
   bool dereference_unknown_address;

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

   /// true when functional pipelining uses STP style
   bool is_stallable_pipelined_function;

   /// initiation time of the pipelined function
   unsigned initiation_time;

   /// Function scope channels number
   unsigned int _channels_number;

   /// Function scope channels type
   MemoryAllocation_ChannelsType _channels_type;

   MemoryAllocation_Policy _allocation_policy;

   bool _omp_core;

   OMPInfoRef _omp_info;

   FunctionBehavior(const FunctionBehavior&) = delete;

 public:
   /**
    * Constructor
    * @param AppM is the application manager
    * @param _helper is the helper associated with the function
    * @param parameters is the set of input parameters
    */
   FunctionBehavior(const application_managerConstRef AppM, const BehavioralHelperRef _helper,
                    const ParameterConstRef parameters);

   /**
    * Declaration of enum representing the type of graph
    */
   enum graph_type
   {
      CFG,     /**< Control flow graph */
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
      FFLSAODG,    /**< System dependence + anti-dependence + output dependence graph + flow graph with feedback */
      FLAODDG,     /**< Anti-dependence + data dependence + output dependence + flow graph */
      FFLAODDG,    /**< Anti dependence + data dependence + output dependence + flow graph with feedback */
      SG,          /**< Speculation graph */
      AGG_VIRTUALG /**< Anti + Data flow graph dependence between aggregates */
   };

   /**
    * Declaration of enum representing the type of bb_graph
    */
   enum bb_graph_type
   {
      BB,            /**< Basic block control flow graph */
      FBB,           /**< Basic block control flow graph with feedback*/
      CDG_BB,        /**< Basic block control dependence graph */
      DOM_TREE,      /**< Basic block dominator tree */
      POST_DOM_TREE, /**< Basic block post-dominator tree */
      DJ             /**< DJ basic block graph (used for loop computation) */
   };

   /// reference to the operations graph constructor
   const std::unique_ptr<operations_graph_constructor> ogc;

   /// reference to the basic block graph constructor
   const std::unique_ptr<BasicBlocksGraphConstructor> bbgc;

   /// This class stores dominator information.
   std::unique_ptr<dominance<BBGraph, false>> dominators;

   /// This class stores post-dominator information.
   std::unique_ptr<dominance<BBGraph, true>> post_dominators;

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

   void add_level(gc_vertex_descriptor v, unsigned int index);

   /**
    * Return the vector of vertex index sorted in topological order.
    */
   const std::deque<gc_vertex_descriptor>& get_levels() const;

   /**
    * Return the map of vertex index sorted in topological order.
    */
   const std::map<gc_vertex_descriptor, unsigned int>& get_map_levels() const;

   void add_bb_level(gc_vertex_descriptor v, unsigned int index);

   /**
    * Return the vector of bb vertex index sorted in topological order.
    */
   const std::deque<gc_vertex_descriptor>& get_bb_levels() const;

   /**
    * Return the map of bb vertex index sorted in topological order.
    */
   const std::map<gc_vertex_descriptor, unsigned int>& get_bb_map_levels() const;

   const OpGraphsCollection& GetOpGraphsCollection() const;

   /**
    * This method returns the operation graphs.
    * @param gt is the type of the graph to be returned
    * @return the pointer to the graph.
    */
   OpGraph GetOpGraph(FunctionBehavior::graph_type gt) const;

   /**
    * This method returns the operation graph having as vertices the vertices of subset
    * @param gt is the type of the graph to be returned
    * @param statements is the set of subgraph vertices
    * @return the refcount to the subgraph
    */
   OpGraph GetOpGraph(FunctionBehavior::graph_type gt,
                      const CustomUnorderedSet<gc_vertex_descriptor>& statements) const;

   const BBGraphsCollection& GetBBGraphsCollection() const;

   /**
    * This method returns the basic block graphs.
    * @param gt is the type of the bb_graph to be returned
    * @return the bb_graph.
    */
   BBGraph GetBBGraph(FunctionBehavior::bb_graph_type gt = FunctionBehavior::BB) const;

   /**
    * Function that prints the class behavioral_manager.
    */
   void print(std::ostream& os) const;

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
      {
         s->print(os);
      }
      return os;
   }

   LoopsConstRef getConstLoops() const;

   LoopsRef getLoops() const;

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
    * remove all variables from the dynamic address set
    */
   void clean_dynamic_address();

   /**
    * Checks if a variable has been allocated in memory
    */
   bool is_variable_mem(unsigned int node_id) const;

   /**
    * Returns the set of memory variables
    */
   const CustomOrderedSet<unsigned int>& get_function_mem() const;

   /// clean the function mem data structure
   void clean_function_mem();

   /**
    * Returns the set of variables for which a dynamic address computation maybe required
    */
   const CustomOrderedSet<unsigned int>& get_dynamic_address() const;

   /**
    * Returns the set of parameters to be copied
    */
   const CustomOrderedSet<unsigned int>& get_parm_decl_copied() const;

   /**
    * @brief clean_parm_decl_copied clean parm_decl_copied data structure
    */
   void clean_parm_decl_copied();

   /**
    * Returns the set of the actual parameters that has to be loaded into the formal parameter
    */
   const CustomOrderedSet<unsigned int>& get_parm_decl_loaded() const;

   /**
    * @brief clean_parm_decl_loaded clean parm_decl_loaded data structure
    */
   void clean_parm_decl_loaded();

   /**
    * Returns the set of the formal parameters that has to be stored into the formal parameter
    */
   const CustomOrderedSet<unsigned int>& get_parm_decl_stored() const;

   /**
    * @brief clean_parm_decl_stored clean parm_decl_stored data structure
    */
   void clean_parm_decl_stored();

   /**
    * Set the use of dereference of unknown address.
    */
   inline void set_dereference_unknown_addr(bool f)
   {
      dereference_unknown_address = f;
   }

   /**
    * Return true if the function has dereference of unknown address.
    */
   inline bool get_dereference_unknown_addr() const
   {
      return dereference_unknown_address;
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
      return state_variables.count(node_id);
   }

   /**
    * @brief clean_state_variable initialize the state variable data structure
    */
   void clean_state_variable()
   {
      state_variables.clear();
   }

   /**
    * @brief get_state_variables
    * @return the state variables data structure
    */
   const CustomOrderedSet<unsigned int>& get_state_variables()
   {
      return state_variables;
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

   bool is_function_pipelined() const
   {
      return pipeline_enabled;
   }

   void disable_function_pipelining()
   {
      pipeline_enabled = false;
   }

   void enable_function_pipelining()
   {
      pipeline_enabled = true;
   }

   void disable_stp()
   {
      is_stallable_pipelined_function = false;
   }

   bool is_stp() const
   {
      return is_stallable_pipelined_function;
   }

   unsigned int get_initiation_time() const
   {
      return initiation_time;
   }

   /**
    * Check if a path from first_operation to second_operation exists in control flow graph (without feedback)
    * @param first_operation is the first operation to be considered
    * @param second_operation is the second operation to be considered
    * @return true if there is a path from first_operation to second_operation in flcfg
    */
   bool CheckReachability(const gc_vertex_descriptor first_operation,
                          const gc_vertex_descriptor second_operation) const;

   /**
    * Check if a path from the first basic block to the second basic block exists in control flow graph (without
    * feedback)
    * @param first_basic_block is the first basic block to be considered
    * @param second_basic_block is the second operation to be considered
    * @return true if there is a path from first_basic_block to second_basic_block in flcfg
    */
   bool CheckBBReachability(const gc_vertex_descriptor first_basic_block,
                            const gc_vertex_descriptor second_basic_block) const;

   /**
    * Check if a path from first_operation to second_operation exists in control flow graph with feedback
    * @param first_operation is the first operation to be considered
    * @param second_operation is the second operation to be considered
    * @return true if there is a path from first_operation to second_operation in fcfg
    */
   bool CheckFeedbackReachability(const gc_vertex_descriptor first_operation,
                                  const gc_vertex_descriptor second_operation) const;

   /**
    * Check if a path from the first basic block to the second basic block exists in control flow graph with feedback
    * @param first_basic_block is the first basic block to be considered
    * @param second_basic_block is the second operation to be considered
    * @return true if there is a path from first_basic_block to second_basic_block in flcfg
    */
   bool CheckBBFeedbackReachability(const gc_vertex_descriptor first_basic_block,
                                    const gc_vertex_descriptor second_basic_block) const;

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

   unsigned int GetChannelsNumber() const;

   void SetChannelsNumber(unsigned int val);

   MemoryAllocation_ChannelsType GetChannelsType() const;

   void SetChannelsType(MemoryAllocation_ChannelsType val);

   MemoryAllocation_Policy GetMemoryAllocationPolicy() const;

   void SetMemoryAllocationPolicy(MemoryAllocation_Policy val);

   bool IsOMPCore() const;

   void SetOMPCore(bool val);

   OMPInfoRef GetOMPInfo() const;

   void SetOMPInfo(OMPInfoRef omp_info);

   std::filesystem::path GetDotPath() const;
};
#endif
