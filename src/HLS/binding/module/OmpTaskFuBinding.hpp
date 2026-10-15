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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file OmpTaskFuBinding.hpp
 * @brief
 *
 *
 * @author Kaixi Matteo Chen  <kaiximatteo.chen@mail.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef OMP_TASK_FU_BINDING_HPP
#define OMP_TASK_FU_BINDING_HPP

/// This mirrors the OpenMP task opt plugin configuration
#define REMOVE_OUTLINED_GEP

/// If the computation shares the same base ptr accross every EM
#define BASE_PTR_SHARED

#include "cg_graph.hpp"
#include "custom_map.hpp"
#include "custom_set.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "refcount.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include <cstdint>
#include <list>
#include <map>
#include <tuple>
#include <vector>

// Forward declaration for ir_node pointer type.
REF_FORWARD_DECL(ir_node);

/// Binds OpenMP task functional units to hardware modules for HLS.

class OmpTaskFuBinding : public fu_binding
{
 private:
   /// Enumerates parameter types for task input arguments.
   enum task_input_kind
   {
      TASK_INPUT_PTR = 0,                      ///< Pointer type.
      TASK_INPUT_COMPILE_TIME_CONST_INT32 = 1, ///< Compile-time constant 32-bit integer.
   };

   /// Structure representing a task node.
   struct task_node
   {
      unsigned long in_degree;  ///< Number of incoming dependencies.
      unsigned long out_degree; ///< Number of outcoming dependencies.
      std::string task_name;    ///< Name of the task.
      std::vector<std::tuple<task_input_kind, unsigned, unsigned, unsigned>> inputs; ///< Input arguments for the task.
   };

   /// Enumerates port roles for the Execution Manager (EM).
   enum task_execution_port
   {
      EM_ACPORTS = 0,
      EM_REQ_PORT = 1,
      EM_DONE_PORT = 2,
      EM_RMDONE_PORT = 3,
      EM_BASE_ADDR_PORT = 4,
      EM_PARAM_OUT_PORT = 5,
      EM_COUNT = 6
   };

   /// Enumerates port roles for the Resource Manager (RM).
   enum task_resource_port
   {
      RM_FU_DONE_PORT = 0,
      RM_FU_REQ_PORT = 1,
      RM_REQ_PORT = 2,
      RM_DONE_PORT = 3,
      RM_PARAM_IN_PORT = 4,
      RM_PARAM_OUT_PORT = 5,
      RM_COUNT = 6
   };

   /// Structure representing a connection between two ports.
   struct task_connection
   {
      std::pair<std::string, so_kind> src;  ///< Source port.
      std::pair<std::string, so_kind> dest; ///< Destination port.
   };

   /// Structure representing a connection between two ports.
   struct indexed_task_connection
   {
      std::tuple<std::string, so_kind, unsigned> src;  ///< Source port.
      std::tuple<std::string, so_kind, unsigned> dest; ///< Destination port.
   };

   /// Structure representing port parameters.
   struct task_port
   {
      std::string name;
      std::string type;
      unsigned long long type_size;
      unsigned long size; ///< Size of the port.
      bool force_vector_port;
      port_o::port_direction direction; ///< task_port direction (IN/OUT).
   };

   /// HLS parameters and context.
   struct task_binding_info
   {
      HLS_managerRef HLSMgr;                            ///< Reference to the HLS manager.
      structural_managerRef SM;                         ///< Reference to the structural manager.
      std::vector<structural_objectRef> modules;        ///< List of generated modules.
      std::vector<structural_objectRef> lambda_modules; ///< List of lambda modules.
      CustomUnorderedMap<std::string, structural_objectRef> lambda_modules_map; ///< List of lambda modules references.
      CustomUnorderedMap<std::string, std::vector<std::string>> lambda_params_ports; ///< Lambda params in ports.
      CustomUnorderedMap<std::string, structural_objectRef> rm_modules; ///< List of resource manager manager modules.
      CustomUnorderedMap<std::string, structural_objectRef> em_modules; ///< List of resource execution manager modules.
      std::vector<structural_objectRef> gen_mem_interface_modules;      ///< Memory interface modules.
      std::map<structural_objectRef, std::vector<structural_objectRef>, jms_sorter>
          connections;                   ///< Map of module connections.
      structural_objectRef owner_module; ///< Owner module reference.
      structural_objectRef circuit;      ///< Circuit module reference.
      structural_objectRef final_join;
      hlsRef HLS;          ///< HLS object reference.
      unsigned* unique_id; ///< Pointer to a unique identifier.
      unsigned mem_channel_num;
      bool use_banked_mem;
   } hls_params;

   /// Naming dictionary structure.
   struct task_names
   {
      std::string mem_arbitrer_name;        ///< Name for the memory arbitrer.
      std::string simple_mem_arbiter_name;  ///< Name for the simple memory arbiter.
      std::string banked_mem_name;          ///< Name for the banked memory interface
      std::string lambda_prefix;            ///< Prefix for lambda modules.
      std::string duplicated_fu_name_token; ///< Token to recognize duplicated FUs.
   };

   /// Specialized naming dictionary.
   struct specialized_task_names
   {
      std::string mem_arbitrer_name; ///< Specialized memory arbitrer name.
   } specialized_name_dict;

   /// Map of task nodes identified by a node ID.
   std::map<std::string, task_node> nodes;

   /// Map a top module input id to an increasing index (parsed from the input XML)
   CustomUnorderedMap<unsigned long, unsigned long> top_module_inputs_map;

   /// Linearized node list
   std::vector<std::pair<std::string, unsigned>> nodes_list;

   /// Utility mapping from input FU name to Bambu mapped FU name
   CustomUnorderedMap<std::string, std::string> task_entry_name_bambu_to_original_dict;

   /// Set of task body identifiers.
   CustomUnorderedSet<std::string> task_bodies;

   /// Map associating task nodes to resource manager allocation.
   std::map<std::string, std::vector<std::string>> rm_allocation;

   /// Map associating the original FU to its duplicates.
   CustomUnorderedMap<std::string, std::vector<structural_objectRef>> duplicated_fus;

   /// Map associating task node IDs to constant input connections for the execution manager.
   CustomUnorderedMap<std::string, std::vector<std::pair<unsigned, structural_objectRef>>> em_const_inputs;

   /// Adjacency list representing the task graph.
   std::map<std::string, std::vector<std::string>> adjency_list;

   /// Dictionary for <functional unit name, ports name> for a given EM node
   CustomUnorderedMap<std::string, std::pair<std::string, std::array<std::string, static_cast<size_t>(EM_COUNT)>>>
       execution_manager_name_dict;

   /// Dictionary for <functional unit name, ports name> for a given RM node
   CustomUnorderedMap<std::string,
                      std::pair<std::string, std::array<std::vector<std::string>, static_cast<size_t>(RM_COUNT)>>>
       resource_manager_name_dict;

   /// Number of closing nodes (with no successors)
   unsigned closing_nodes;

   /// Parses the XML configuration file.
   void parse_xml(const std::string& file_name);

   /// Generates constant inputs for Resource Manager modules.
   void generate_const_rm_inputs();

   /// Allocates execution manager (EM) ports to resource manager (RM) ports.
   void allocate_em_to_rm();

   /// Parses lambda module definitions from configuration.
   void parse_lambda_modules();

   /// Adds a new module_o.
   structural_objectRef add_module(const std::string& fu_name, const std::string& module_name);

   /// Generates a generic module with specified ports.
   enum task_module_kind
   {
      TASK_JOIN,
      TASK_RESOURCE_MANAGER,
      TASK_EXECUTION_MANAGER
   };

   void generate_generic_module(const std::string& name, const std::vector<task_port>& ports,
                                const std::vector<unsigned>& offsets, task_module_kind kind);

   /// Creates the Execution Manager (EM) modules.
   void create_lib_execution_managers();
   void write_execution_manager(structural_managerRef component, const std::vector<task_port>& ports,
                                const std::vector<unsigned>& const_offsets);

   /// Creates and adds to the library the Resource Manager (RM) modules.
   void create_lib_resource_managers();
   void write_resource_manager(structural_managerRef component, const std::vector<task_port>& ports,
                               const std::vector<unsigned>& /*const_offsets*/);

   /// Generates vectorised and resized connections for one-to-one port mapping.
   std::pair<structural_objectRef, structural_objectRef>
   generate_resized_connection(const std::string& source_port_id, const std::string& dest_port_id,
                               structural_objectRef src, structural_objectRef dest, const unsigned long long size,
                               so_kind source_port_type, so_kind dest_port_type, unsigned dest_port_num,
                               bool specialize = true, bool resize = true);

   /// Establishes a one-to-one connection based on the provided connection details.
   void perform_connection_one_to_one(const std::pair<structural_objectRef, structural_objectRef>& to_connect);

   /// Establishes a one-to-many connection based on the provided connection details.
   void
   perform_connection_one_to_many(const std::pair<structural_objectRef, std::vector<structural_objectRef>>& to_connect);

   /// Generates vectorised and resized connections for one-to-many port mapping.
   std::pair<structural_objectRef, std::vector<structural_objectRef>>
   generate_vectorised_and_resized_connections_one_to_many(const std::string& source_port_id,
                                                           const std::string& dest_port_id, structural_objectRef src,
                                                           const std::vector<structural_objectRef>& dest,
                                                           const unsigned long long size, so_kind source_port_type,
                                                           so_kind dest_port_type, unsigned dest_port_num);

   /// Collects a port from a module with optional vector specialization and resizing.
   structural_objectRef collect_port(structural_objectRef module_obj, const std::string& p_name,
                                     const unsigned long long size, so_kind p_type, const unsigned p_num,
                                     bool specialize = true, bool resize = true) const;

   /// Collects a specific port from a list of modules.
   void collect_ports(const std::vector<structural_objectRef>& modules, const std::string& p_name,
                      const unsigned long long size, so_kind p_type, const unsigned p_num,
                      std::vector<structural_objectRef>& collection) const;

   /// Generates the memory arbitrer module_o.
   void generate_mem_interface();

   /// Generates the resource modules.
   void generate_rms();

   /// Generates the execution modules.
   void generate_ems();

   /// Creates the final join module_o.
   void create_final_join();
   void write_final_join(structural_managerRef component, const std::vector<task_port>& ports,
                         const std::vector<unsigned>& /*const_offsets*/);

   /// Generates the final join module_o.
   void generate_final_join();

   /// Generates the topological computational graph.
   void generate_topo_graph();

   /// Internalizes a list of ports within a module_o.
   void internalize_ports(const std::vector<std::tuple<std::string, so_kind>>& ports, structural_objectRef owner);

   /// Forward internalized task ports, preserving any ports already owned by the parent.
   void forward_ports(const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>>& ports,
                      const structural_objectRef& from, const structural_objectRef& to, so_kind port_type,
                      unsigned int port_count, bool memory_port);

 public:
   /// Constructor.
   OmpTaskFuBinding(const HLS_managerConstRef _HLSMgr, const unsigned int function_id,
                    const ParameterConstRef parameters);

   /// Predefined naming dictionary for modules.
   static const struct task_names name_dict;

   /// Constant representing pointer size (32 bits).
   static const unsigned ptr_size;

   /// Bambu autogenerated port prefix name
   static const std::string lambda_module_param_port_prefix;

   /// Utility flag
   static const int not_overwrite;

   /// Prefix used for pointer parameters.
   static const std::string ptr_prefix;

   /// Prefix for compile-time constant 32-bit integers.
   static const std::string const_int32_prefix;

   /// List of ports that are to be internalized within modules.
   static const std::vector<std::tuple<std::string, so_kind>> ports_to_internalize;
   static const std::vector<std::tuple<std::string, so_kind>> mem_in_ports;

   /// Directed typed port list for memory ports. Currently are single channeled
   static const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> m_ports;

   static const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> start_done_ports;

   /// Directed typed port list for Execution Manager (EM) ports.
   static const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> em_ports;

   /// Directed typed port list for Resource Manager (RM) ports.
   static const std::vector<std::tuple<std::string, so_kind, port_o::port_direction>> rm_ports;

   /// task_port connection list between memory arbitrer and owner module (both IN and OUT).
   static const std::vector<task_connection> mem_arbitrer_totask_graph_ports;

   /// task_port connection list between memory arbitrer and FUs (IN and OUT)
   static const std::vector<task_connection> mem_arbitrer_to_lambda_modules;

   static const int UNKNOWN_NUM_SIZE;

   /// task_port connection list for RM to FU.
   static const std::vector<std::pair<int, task_connection>> resource_manager_to_lambda_modules;

   /// task_port connection list for FU to RM.
   static const std::vector<indexed_task_connection> lambda_to_resource_manager;

   /// task_port connection list for EM to RM.
   /// @note
   /// EM has a port vector o representing all the ptrs outputs to the RM
   /// RM has a port_vector_o for each input param type:
   /// if we have 2 params from 2 EM, EM output will be [param1, param2] and RM input will be [param1EM1, param1EM2] -
   /// [param2EM1, param2EM2]
   static const std::vector<indexed_task_connection> execution_manager_to_resource_manager;

   /// task_port connection list for RM to EM.
   static const std::vector<indexed_task_connection> resource_manager_to_execution_manager;

   /// task_port connection list for EM to EM.
   static const std::vector<indexed_task_connection> execution_manager_to_execution_manager;

   /// Constant prefix for execution manager nodes
   static std::string em_name_prefix;

   /// Constant prefix for resource manager nodes
   static std::string rm_name_prefix;

   /// Constant name for final join node
   static std::string final_join_name;

   /// Parameter port size
   /// TODO: make this dynamic
   static unsigned param_port_size;

   /// Manages the configuration of external global ports.
   void manage_extern_global_port([[maybe_unused]] const HLS_managerRef, [[maybe_unused]] const hlsRef,
                                  [[maybe_unused]] const structural_managerRef SM,
                                  [[maybe_unused]] structural_objectRef port_in, [[maybe_unused]] unsigned int dir,
                                  [[maybe_unused]] structural_objectRef circuit,
                                  [[maybe_unused]] unsigned int num) final;

   /// Manages memory ports in a parallel chained configuration.
   void manage_memory_ports_parallel_chained(const HLS_managerRef, const structural_managerRef SM,
                                             const std::list<structural_objectRef>& memory_modules,
                                             const structural_objectRef circuit, const hlsRef HLS,
                                             unsigned int& unique_id) final;
};

#endif