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
 * @file mux_connection_binding.hpp
 * @brief Class to manage mux-based interconnections based on the FSM controller
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef MUX_CONNECTION_BINDING_HPP
#define MUX_CONNECTION_BINDING_HPP

#include "conn_binding_creator.hpp"

#include "HLS/fsm/FSMInfo.hpp"
#include "conn_binding.hpp"
#include "graph.hpp"
#include "hls_manager.hpp"

class OpGraph;
REF_FORWARD_DECL(conn_binding);

/**
 * @class mux_connection_binding
 * @ingroup interconnect
 * @brief Class managing the connection binding of datapath. It extends the standard mux_connection_binding class.
 *
 * This class provides methods to compute interconnections among datapath elements (functional units, registers,
 * ports). Different architectural style have been implemented: mux-based or bus-based. They are implemented as
 * specialization of generic interconnections.
 *
 */
class mux_connection_binding : public conn_binding_creator
{
   /// type representing a resource identifier
   using resource_id_type = std::pair<unsigned int, unsigned int>;

   /// store the registers for each resource and for each port
   std::map<resource_id_type, std::map<unsigned int, CustomOrderedSet<unsigned int>>> regs_in;
   /// store the chained storage values for each resource and for each port
   std::map<resource_id_type, std::map<unsigned int, CustomOrderedSet<unsigned int>>> chained_in;
   /// store the resource in IN for each resource and for each port
   std::map<resource_id_type, std::map<unsigned int, CustomOrderedSet<resource_id_type>>> module_in;

   /// variable used to assign a unique id to sparse logic
   unsigned int id;

   /// store the current phi use. Used during the analysis of phi nodes
   unsigned int cur_phi_ir_var;

   /// cache connection type
   enum cacheType
   {
      i_assign = 0,
      uu_conv,
      ui_conv,
      iu_conv,
      ii_conv
   };
   /// connection cache
   std::map<std::tuple<unsigned int, cacheType, const HLS_manager::io_binding_type>, generic_objRef> connCache;

   /**
    * Performs specialization of interconnections using mux architecture.
    */
   unsigned int mux_interconnection();

   /**
    * Computes logic for inputs. The connection can become directed or by multiplexer. If multiplexers are
    * allocated, then decoding logic is created.
    * @param src is the set of references to generic_obj, sources of connections
    * @param tgt is the reference to connection target
    * @param op is i-th operand of target element, where source is attached
    * @param port_index is the i-th port index of the given target port.
    * @param iteration identifies the iteration for replicated connections
    * @return number of multiplexer allocated
    */
   unsigned int input_logic(const conn_binding::ConnectionSources& src, const generic_objRef tgt, unsigned int op,
                            unsigned int port_index, unsigned int iteration);

   /**
    * Determine the actual interconnection
    */
   void determine_connection(gc_vertex_descriptor op, const HLS_manager::io_binding_type& var, generic_objRef fu_obj,
                             unsigned int port_num, unsigned int port_index, const OpGraph& data,
                             unsigned int precision, unsigned int alignment, FSMInfo::state_descriptor state_src,
                             FSMInfo::state_descriptor state_tgt, unsigned int src_phi_bb_index);

   /**
    * Compute the bitsize given a io_binding type
    */
   unsigned long long object_bitsize(const ir_managerRef IRM, const HLS_manager::io_binding_type& obj) const;

   /**
    *  create the connection object and update the unique table
    */
   void create_single_conn(gc_vertex_descriptor op, generic_objRef fu_obj_src, generic_objRef fu_obj,
                           unsigned int port_num, unsigned int port_index, unsigned int ir_var, unsigned int precision,
                           const bool is_not_a_phi, FSMInfo::state_descriptor state_src,
                           FSMInfo::state_descriptor state_tgt);

   /**
    * connect the fu_obj with the associated registers.
    */
   void connect_to_registers(gc_vertex_descriptor op, const OpGraph& data, generic_objRef fu_obj, unsigned int port_num,
                             unsigned int port_index, unsigned int ir_var, unsigned long long precision,
                             const bool is_not_a_phi, FSMInfo::state_descriptor state_src,
                             FSMInfo::state_descriptor state_tgt, unsigned int src_phi_bb_index);

   unsigned int extract_parm_decl(unsigned int ir_var, const ir_managerRef IRM);

   void connect_pipelined_registers(FSMInfo::state_descriptor state, const OpGraph& data);

   unsigned int address_precision(unsigned int precision, gc_vertex_descriptor op, const OpGraph& data,
                                  const ir_managerRef IRM);

   bool isConstantObj(unsigned int ir_node_index, const ir_managerRef IRM);

 public:
   mux_connection_binding(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                          const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;

   /**
    * Creates the connections inside the architecture
    */
   void create_connections();
};
#endif
