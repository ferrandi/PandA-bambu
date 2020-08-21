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
 * @file fu_binding.hpp
 * @brief Data structure used to store the functional-unit binding of the vertexes.
 *
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef FU_BINDING_HPP
#define FU_BINDING_HPP

#include "custom_map.hpp"
#include <iosfwd>
#include <list>

#include "graph.hpp"
#include "op_graph.hpp"

#include "refcount.hpp"
#include "utility.hpp"

/**
 * @name forward declarations
 */
//@{
CONSTREF_FORWARD_DECL(AllocationInformation);
REF_FORWARD_DECL(AllocationInformation);
class funit_obj;
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(fu_binding);
REF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(memory);
class module;
CONSTREF_FORWARD_DECL(OpGraph);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(structural_object);
CONSTREF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
//@}

struct jms_sorter
{
   bool operator()(const structural_objectRef& a, const structural_objectRef& b) const;
};

/**
 * Class managing the functional-unit binding.
 * It stores the functional-unit binding, that is, the mapping of operations in the behavioral description onto the set of
 * allocated functional units.
 */
class fu_binding
{
 protected:
   /// map between functional unit id and number of units allocated
   std::map<unsigned int, unsigned int> allocation_map;

   /// map between unit and allocated objects
   std::map<std::pair<unsigned int, unsigned int>, generic_objRef> unique_table;

   /// reverse map that associated each functional unit with the set of operations that are executed
   std::map<std::pair<unsigned int, unsigned int>, OpVertexSet> operations;

   /// operation binding
   std::map<unsigned int, generic_objRef> op_binding;

   /// allocation manager. Used to retrieve the string name of the functional units.
   AllocationInformationRef allocation_information;

   /// information about the tree datastructure
   const tree_managerConstRef TreeM;

   /// The operation graph
   const OpGraphConstRef op_graph;

   /// port assignment: ports are swapped predicate
   CustomOrderedSet<vertex> ports_are_swapped;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   int debug_level;

   /// useful to know for automatic pipelining
   bool has_resource_sharing_p;

   /**
    * Update number of allocated units. It takes effect only if new number is greather than stored one
    * @param unit is the identifier associated to functional unit
    * @param number is the new number of allocated units
    */
   void update_allocation(unsigned int unit, unsigned int number);

   /**
    * fill the memory of the array ref
    * @param TreeM is the tree_manager
    * @param init_file_a is the file where the data is written (all data stored in this file in case is_memory_splitted is false
    * @param initi_file_b is the file where the data is written (only the odd elements are store in this file and only if is_memory_splitted is true
    * @param ar is the array ref variable declaration
    * @param vec_size is the number of the element of the array
    * @param elts_size is the element size in bits
    * @param bram_bitsise is value of BRAM_BITSIZE parameter
    * @param is_memory_splitted is true when the allocated memory is splitted into two sets of BRAMs
    */
   void fill_array_ref_memory(std::ostream& init_file_a, std::ostream& init_file_b, unsigned int ar, long long int& vec_size, unsigned int& elts_size, const memoryRef mem, unsigned int bram_bitsize, bool is_memory_splitted, bool is_sds, module* fu_module);

   /**
    * Add an instance of the current port
    */
   structural_objectRef add_gate(const HLS_managerRef HLSMgr, const hlsRef HLS, const technology_nodeRef fu, const std::string& name, const OpVertexSet& operations, structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * check the module parametrization
    */
   void check_parametrization(structural_objectRef curr_gate);

   /**
    * fix port properties for proxy memory ports
    * @param memory_units is the list o memory ports
    * @param curr_gate is the current gate
    * @param var_call_sites_rel put into relation proxied variables and modules referring to such variables
    */
   void kill_proxy_memory_units(std::map<unsigned int, unsigned int>& memory_units, structural_objectRef curr_gate, std::map<unsigned int, std::list<structural_objectRef>>& var_call_sites_rel, std::map<unsigned int, unsigned int>& reverse_memory_units);

   void kill_proxy_function_units(std::map<unsigned int, std::string>& wrapped_units, structural_objectRef curr_gate, std::map<std::string, std::list<structural_objectRef>>& fun_call_sites_rel, std::map<std::string, unsigned int>& reverse_wrapped_units);

   /**
    * connect proxies with storage components
    * @param mem_obj is the relation between fu_id and structural objects
    * @param reverse_memory_units is the relation between var and functional units
    * @param var_call_sites_rel is the relation between var and call sites having a proxy as module parameter
    * @param SM is the structural manager
    */
   void manage_killing_memory_proxies(std::map<unsigned int, structural_objectRef>& mem_obj, std::map<unsigned int, unsigned int>& reverse_memory_units, std::map<unsigned int, std::list<structural_objectRef>>& var_call_sites_rel,
                                      const structural_managerRef SM, const hlsRef HLS, unsigned int& _unique_id);

   void manage_killing_function_proxies(std::map<unsigned int, structural_objectRef>& fun_obj, std::map<std::string, unsigned int>& reverse_function_units, std::map<std::string, std::list<structural_objectRef>>& fun_call_sites_rel,
                                        const structural_managerRef SM, const hlsRef HLS, unsigned int& _unique_id);

 public:
   /// The value used to identified unknown functional unit
   static const unsigned int UNKNOWN;

   /**
    * Constructor.
    * @param HLS_mgr is the HLS manager
    * @param function_id is the index of the function
    * @param parameters is the set of input parameters
    */
   fu_binding(const HLS_managerConstRef HLS_mgr, const unsigned int function_id, const ParameterConstRef parameters);

   fu_binding(const fu_binding& original);

   fu_binding& operator=(const fu_binding&) = delete;

   /**
    * Destructor.
    */
   virtual ~fu_binding();

   /**
    * @brief create_fu_binding: factory method for fu_binding
    * @param _HLSMgr
    * @param _function_id
    * @param _parameters
    * @return the correct class of fu_binding
    */
   static fu_bindingRef create_fu_binding(const HLS_managerConstRef _HLSMgr, const unsigned int _function_id, const ParameterConstRef _parameters);

   /**
    * Binds an operation vertex to a functional unit. The functional unit is identified by an id and
    * its index
    * @param v is operation vertex
    * @param id is the identifier of the functional unit
    * @param index is the functional unit index
    */
   void bind(const vertex& v, unsigned int id, unsigned int index = INFINITE_UINT);

   /**
    * Returns the functional unit assigned to the vertex.
    * @param v is the considered vertex.
    * @return the functional unit assigned with the vertex.
    */
   unsigned int get_assign(const vertex& v) const;

   /**
    * Returns the functional unit assigned to the operation.
    * @param statement_index is the considered operation.
    * @return the functional unit assigned to the operation
    */
   unsigned int get_assign(const unsigned int statement_index) const;

   /**
    * Returns the index of functional unit assigned to the vertex.
    * @param v is the considered vertex.
    * @return the index of the functional unit assigned to the vertex.
    */
   unsigned int get_index(const vertex& v) const;

   /**
    * Returns the name of the functional unit
    * @param v is the considered vertex.
    * @return the name of the functional unit assigned to the vertex.
    */
   std::string get_fu_name(vertex const& v) const;

   /**
    * Returns number of functional unit allocated
    * @param unit is the identifier associated to functional unit
    * @return the number of functional units allocated.
    */
   unsigned int get_number(unsigned int unit) const
   {
      std::map<unsigned int, unsigned int>::const_iterator it = allocation_map.find(unit);
      if(it != allocation_map.end())
         return it->second;
      else
         return 0;
   }

   /**
    * Redefinition of the [] operator. It is necessary because segfaults happen when the vertex is not into the map
    * and so the object has not been created yet. This operator can be used only to read information since it returns
    * a constant object. It's necessary because a direct manipulation of the object can create non-consistent data
    * objects.
    * @param v is the vertex you want to get the object
    * @return the constant object related to vertex
    */
   const funit_obj& operator[](const vertex& v);

   /**
    * Returns reference to funit object associated with this vertex
    * @param v is the given vertex
    * @return the associated reference
    */
   generic_objRef get(const vertex v) const;

   generic_objRef get(unsigned int name, unsigned int index)
   {
      return (unique_table.count(std::make_pair(name, index))) ? unique_table[std::make_pair(name, index)] : generic_objRef();
   }

   /**
    * Returns the set of allocated unit
    * @return the set of allocated unit
    */
   std::list<unsigned int> get_allocation_list() const;

   /**
    * return true in case the vertex has been previously assigned
    * @param v
    * @return true in case v has been previously assigned
    */
   bool is_assigned(const vertex& v) const;

   /**
    * return true in case the operation has been previously assigned
    * @param statement_index
    * @return true in case operation has been previously assigned
    */
   bool is_assigned(const unsigned int statement_index) const;

   /**
    * Instance the functional unit inside the structural representation of the datapath
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port);

   virtual void manage_extern_global_port(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, structural_objectRef port_in, unsigned int dir, structural_objectRef circuit, unsigned int num);

   /**
    * Manage the connections between memory ports
    */
   static void manage_memory_ports_chained(const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit);
   virtual void manage_memory_ports_parallel_chained(const HLS_managerRef HLSMgr, const structural_managerRef SM, const std::list<structural_objectRef>& memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int& unique_id);

   /**
    * Return the operations that are executed by the given functional unit
    */
   OpVertexSet get_operations(unsigned int unit, unsigned int index) const;

   /**
    * Specialize the functional unit based on variables associated with the corresponding operations
    */
   void specialise_fu(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj, unsigned int fu, const OpVertexSet& operations, unsigned int ar);

   /**
    * Specialize a memory unit
    */
   void specialize_memory_unit(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef fu_obj, unsigned int ar, std::string& base_address, unsigned long long rangesize, bool is_doubled, bool is_memory_splitted, bool is_sparse_memory,
                               bool is_sds);

   static void write_init(const tree_managerConstRef TreeM, tree_nodeRef var_node, tree_nodeRef init_node, std::vector<std::string>& init_file, const memoryRef mem, unsigned int element_precision);

   virtual bool manage_module_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, const structural_objectRef curr_gate, unsigned int num);

   virtual void join_merge_split(const structural_managerRef SM, const hlsRef HLS, std::map<structural_objectRef, std::list<structural_objectRef>, jms_sorter>& primary_outs, const structural_objectRef circuit, unsigned int& unique_id);

   /**
    * specify if vertex v have or not its ports swapped
    * @param v is the vertex
    * @param condition is true when ports are swapped, false otherwise
    */
   void set_ports_are_swapped(vertex v, bool condition);

   /**
    * Check if vertex v has its ports swapped or not
    * @param v is the vertex
    * @return true when ports of v are swapped, false otherwise
    */
   bool get_ports_are_swapped(vertex v) const
   {
      return ports_are_swapped.find(v) != ports_are_swapped.end();
   }

   /// return true in case at least one resource is shared
   bool has_resource_sharing() const
   {
      return has_resource_sharing_p;
   }
};

/**
 * RefCount type definition of the fu_binding class structure
 */
typedef refcount<fu_binding> fu_bindingRef;
typedef refcount<const fu_binding> fu_bindingConstRef;

#endif
