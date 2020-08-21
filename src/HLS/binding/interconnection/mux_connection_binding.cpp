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
 * @file mux_connection_binding.cpp
 * @brief Implementation of mux_connection_binding class
 *
 * Implementation of mux_connection_binding class. In this class all datastructures have been filled and
 * then datapath circuit is created.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "mux_connection_binding.hpp"

/// behavioral specification datastructure
#include "behavioral_helper.hpp"
#include "function_behavior.hpp"
#include "graph.hpp"
#include "hls_manager.hpp"

/// parser/treegcc include
#include "token_interface.hpp"

/// tree_node datastructure
#include "ext_tree_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"

/// high-level synthesis datastructure
#include "hls.hpp"
/// high-level syntesis sub-tasks results
#include "conn_binding.hpp"
#include "fu_binding.hpp"
#include "liveness.hpp"
#include "memory.hpp"
#include "memory_symbol.hpp"
#include "parallel_memory_conn_binding.hpp"
#include "reg_binding.hpp"

/// Allocation datastructure
#include "allocation.hpp"

#include <iosfwd>

#include "technology_node.hpp"

#include "adder_conn_obj.hpp"
#include "commandport_obj.hpp"
#include "connection_obj.hpp"
#include "conv_conn_obj.hpp"
#include "funit_obj.hpp"
#include "multiplier_conn_obj.hpp"
#include "mux_obj.hpp"
#include "register_obj.hpp"

#include "direct_conn.hpp"
#include "mux_conn.hpp"

#include "tree_reindex.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"

#include "cpu_time.hpp"
#include "dbgPrintHelper.hpp"
#include "op_graph.hpp"
#include "utility.hpp"

/// HLS/binding/register/algorithms include
#include "weighted_clique_register.hpp"

/// HLS/binding/storage_value_information
#include "storage_value_information.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// state transition graph info
#include "math_function.hpp"
#include "state_transition_graph.hpp"
#include "state_transition_graph_manager.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#define USE_ALIGNMENT_INFO 1

static unsigned int align_to_trimmed_bits(unsigned int algn)
{
   if(algn == 8)
      return 0;
   else if(algn == 16)
      return 1;
   else if(algn == 32)
      return 2;
   else if(algn == 64)
      return 3;
   else if(algn == 128)
      return 4;
   else if(algn == 256)
      return 5;
   else
      return 0;
}

mux_connection_binding::mux_connection_binding(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : conn_binding_creator(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING), id(0), cur_phi_tree_var(0), is_PC(false)
{
   debug_level = _parameters->get_class_debug_level(GET_CLASS(*this));
}

mux_connection_binding::~mux_connection_binding() = default;

void mux_connection_binding::Initialize()
{
   HLSFunctionStep::Initialize();
   HLS->Rconn = conn_bindingRef();
   regs_in.clear();
   chained_in.clear();
   module_in.clear();
   swap_computed_table.clear();
   noswap_computed_table.clear();
   id = 0;
   cur_phi_tree_var = 0;
#if HAVE_EXPERIMENTAL
   is_PC = HLS->controller_type == HLSFlowStep_Type::PARALLEL_CONTROLLER_CREATOR;
#endif
}

/// This function is the public one to execute connection binding. First it computes virtual connection to be
/// implemented and then it allocates them based on chosen architecture
DesignFlowStep_Status mux_connection_binding::InternalExec()
{
   connCache.clear();
   long step_time = 0;
   if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
      START_TIME(step_time);
   create_connections();

   unsigned int mux = mux_interconnection();
   if(mux)
   {
      if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
         STOP_TIME(step_time);
      if(output_level <= OUTPUT_LEVEL_PEDANTIC)
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Connection Binding Information for function " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->get_function_name() + ":");
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "---Number of allocated multiplexers (2-to-1 equivalent): " + std::to_string(mux));
      INDENT_OUT_MEX(OUTPUT_LEVEL_PEDANTIC, output_level, "---Total number of bit-level multiplexers: " + STR(HLS->Rconn->determine_bit_level_mux()));
      if(output_level >= OUTPUT_LEVEL_VERY_PEDANTIC)
         HLS->Rconn->print();
      if(output_level >= OUTPUT_LEVEL_MINIMUM and output_level <= OUTPUT_LEVEL_PEDANTIC)
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "Time to perform interconnection binding: " + print_cpu_time(step_time) + " seconds");
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
      if(output_level <= OUTPUT_LEVEL_PEDANTIC)
         INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   connCache.clear();
   HLS->Rconn->cleanInternals();
   return DesignFlowStep_Status::SUCCESS;
}

generic_objRef mux_connection_binding::dynamic_multidimensional_array_handler(array_ref* ar, const vertex& op, const OpGraphConstRef data, unsigned int& base_address_index, std::vector<unsigned int>& recursive_indexes_values,
                                                                              std::vector<unsigned int>& dims, generic_objRef& global_adder, const bool is_not_a_phi)
{
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   std::vector<generic_objRef> multiplier_vector;
   generic_objRef mult;
   unsigned int tree_index = GET_INDEX_NODE(ar->op1);
   generic_objRef adder;
   unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
   bus_addr_bitsize = address_precision(bus_addr_bitsize, op, data, TreeM);
   if(GET_NODE(ar->op0)->get_kind() == array_ref_K)
   {
      /// generates needed components
      if(recursive_indexes_values.size() > 0)
      {
         adder = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
         HLS->Rconn->add_sparse_logic(adder);
         GetPointer<adder_conn_obj>(adder)->add_bitsize(bus_addr_bitsize);
      }
      for(unsigned int i = 0; i < recursive_indexes_values.size(); i++)
      {
         mult = generic_objRef(new multiplier_conn_obj("multiplier_conn_obj_" + STR(id++)));
         multiplier_vector.push_back(mult);
         HLS->Rconn->add_sparse_logic(mult);
      }
      /// updates indexes array and calls recursive function
      auto* snd_ar = GetPointer<array_ref>(GET_NODE(ar->op0));
      recursive_indexes_values.push_back(tree_index);
      generic_objRef received_adder = dynamic_multidimensional_array_handler(snd_ar, op, data, base_address_index, recursive_indexes_values, dims, global_adder, is_not_a_phi);

      if(recursive_indexes_values.size() == 1) // last recursive function (no multiplier created)
      {
         connect_array_index(tree_index, received_adder, 0, 0, bus_addr_bitsize, data, op);
         return global_adder;
      }
      connect_array_index(tree_index, multiplier_vector[0], 0, 0, bus_addr_bitsize, data, op);
      for(unsigned int i = 0; i < recursive_indexes_values.size() - 1; i++)
      {
         determine_connection(op, HLS_manager::io_binding_type(0, dims[dims.size() - 1 - i]), multiplier_vector[i], 1, 0, data, bus_addr_bitsize);
         GetPointer<multiplier_conn_obj>(multiplier_vector[i])->add_bitsize(bus_addr_bitsize);
         GetPointer<multiplier_conn_obj>(multiplier_vector[i])->set_multiplication_to_constant(dims[dims.size() - 1 - i]);

         if(i < recursive_indexes_values.size() - 2)
         {
            // links i-th multiplier outcome to i+1-th multiplier port
            create_single_conn(data, op, multiplier_vector.at(i), multiplier_vector.at(i + 1), 0, 0, 0, bus_addr_bitsize, is_not_a_phi);
         }
         else
         {
            // links last multiplier outcome to local adder
            create_single_conn(data, op, multiplier_vector.at(i), adder, 1, 0, 0, bus_addr_bitsize, is_not_a_phi);
         }
      }
      // add all required variables to the connection between adder and received_adder
      create_single_conn(data, op, adder, received_adder, 0, 0, 0, bus_addr_bitsize, is_not_a_phi);

      recursive_indexes_values.pop_back();
      return adder;
   }
   else
   {
      base_address_index = GET_INDEX_NODE(ar->op0);
      recursive_indexes_values.push_back(tree_index);

      unsigned int type_index;
      tree_helper::get_type_node(GET_NODE(ar->op0), type_index);
      tree_helper::get_array_dimensions(TreeM, type_index, dims);

      /// starts to create basic components directly linked to the offset calculator
      adder = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
      HLS->Rconn->add_sparse_logic(adder);
      GetPointer<adder_conn_obj>(adder)->add_bitsize(bus_addr_bitsize);
      for(unsigned int i = 0; i < recursive_indexes_values.size() - 1; i++)
      {
         mult = generic_objRef(new multiplier_conn_obj("multiplier_conn_obj_" + STR(id++)));
         multiplier_vector.push_back(mult);
         HLS->Rconn->add_sparse_logic(mult);
      }
      connect_array_index(tree_index, multiplier_vector[0], 0, 0, bus_addr_bitsize, data, op);
      for(unsigned int i = 0; i < dims.size() - 1 && i < multiplier_vector.size(); i++)
      {
         determine_connection(op, HLS_manager::io_binding_type(0, dims[dims.size() - 1 - i]), multiplier_vector[i], 1, 0, data, bus_addr_bitsize);
         GetPointer<multiplier_conn_obj>(multiplier_vector[i])->add_bitsize(bus_addr_bitsize);
         GetPointer<multiplier_conn_obj>(multiplier_vector[i])->set_multiplication_to_constant(dims[dims.size() - 1 - i]);
         if(i < dims.size() - 2 && i + 1 < multiplier_vector.size())
         {
            // links i-th multiplier outcome to i+1-th multiplier port
            create_single_conn(data, op, multiplier_vector[i], multiplier_vector[i + 1], 0, 0, 0, bus_addr_bitsize, is_not_a_phi);
         }
         else
         {
            // links last multiplier outcome to global adder port
            create_single_conn(data, op, multiplier_vector[i], adder, 1, 0, 0, bus_addr_bitsize, is_not_a_phi);
         }
      }

      recursive_indexes_values.pop_back();
      global_adder = adder;
      return adder;
   }
   THROW_ERROR("Error in dynamic recursive function: array type not supported");
   return adder;
}

void mux_connection_binding::create_single_conn(const OpGraphConstRef data, const vertex& op, generic_objRef fu_obj_src, generic_objRef fu_obj, unsigned int port_num, unsigned int port_index, unsigned int tree_var, unsigned int precision,
                                                bool is_not_a_phi)
{
   const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(op);
   const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
   for(auto rs_it = running_states.begin(); rs_it_end != rs_it; ++rs_it)
   {
      vertex state = *rs_it;

      if(GetPointer<register_obj>(fu_obj) && !is_not_a_phi)
      {
         const StateInfoConstRef state_info = is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(*rs_it);
         if(state_info && state_info->is_duplicated && !state_info->all_paths)
         {
            bool found_branch = false;
            const tree_managerRef TreeM = HLSMgr->get_tree_manager();
            const auto gp = GetPointer<const gimple_phi>(TreeM->get_tree_node_const(data->CGetOpNodeInfo(op)->GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               unsigned int bbID = def_edge.second;
               if(!state_info->isOriginalState && bbID != state_info->sourceBb)
                  continue;
               else if(state_info->isOriginalState && bbID == state_info->sourceBb)
                  continue;
               else if(state_info->moved_op_def_set.find(cur_phi_tree_var) != state_info->moved_op_def_set.end())
                  continue;
               else if(def_edge.first->index != cur_phi_tree_var)
                  continue;
               found_branch = true;
               break;
            }
            if(!found_branch)
               continue;
         }
         /// instead of tree_var we use cur_phi_tree_var when we look into the state in data structure since tree_var can be modified in the mean time
         THROW_ASSERT(HLS->Rliv->has_state_in(state, op, cur_phi_tree_var), " no state in for @" + STR(tree_var));
         const CustomOrderedSet<vertex>& states_in = HLS->Rliv->get_state_in(state, op, cur_phi_tree_var);
         const CustomOrderedSet<vertex>::const_iterator s_in_it_end = states_in.end();
         for(auto s_in_it = states_in.begin(); s_in_it != s_in_it_end; ++s_in_it)
         {
            HLS->Rconn->add_data_transfer(fu_obj_src, fu_obj, port_num, port_index, data_transfer(tree_var, precision, *s_in_it, state, op));
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                          "       - add data transfer from " << fu_obj_src->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                             << HLS->Rliv->get_name(*s_in_it) + " to state " + HLS->Rliv->get_name(state) + (tree_var ? (" for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var)) : ""));
            generic_objRef enable_obj = GetPointer<register_obj>(fu_obj)->get_wr_enable();
            GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*s_in_it, state, commandport_obj::data_operation_pair(cur_phi_tree_var, op)));
            GetPointer<commandport_obj>(enable_obj)->set_phi_write_enable();
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + fu_obj->get_string() + " from state " << HLS->Rliv->get_name(*s_in_it) + " to state " + HLS->Rliv->get_name(state));
         }
      }
      else
      {
         HLS->Rconn->add_data_transfer(fu_obj_src, fu_obj, port_num, port_index, data_transfer(tree_var, precision, state, NULL_VERTEX, op));
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                       "       - add data transfer from " << fu_obj_src->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " in state "
                                                          << HLS->Rliv->get_name(state) + (tree_var ? (" for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var)) : ""));
      }
   }
}

unsigned int mux_connection_binding::address_precision(unsigned int precision, const vertex& op, const OpGraphConstRef data, const tree_managerRef TreeM)
{
   unsigned int fu_type = HLS->Rfu->get_assign(op);
   unsigned int node_id = data->CGetOpNodeInfo(op)->GetNodeId();
   const tree_nodeRef node = TreeM->get_tree_node_const(node_id);
   auto* gm = GetPointer<gimple_assign>(node);
   bool right_addr_expr = false;
   if(gm && GetPointer<addr_expr>(GET_NODE(gm->op1)))
      right_addr_expr = true;
   bool is_load_store = GET_TYPE(data, op) & (TYPE_LOAD | TYPE_STORE);
   if(!right_addr_expr and is_load_store and HLS->allocation_information->is_direct_access_memory_unit(fu_type))
   {
      unsigned var = HLS->allocation_information->is_memory_unit(fu_type) ? HLS->allocation_information->get_memory_var(fu_type) : HLS->allocation_information->get_proxy_memory_var(fu_type);
      if(var && HLSMgr->Rmem->is_private_memory(var))
      {
         unsigned long long int max_addr = HLSMgr->Rmem->get_base_address(var, HLS->functionId) + tree_helper::size(TreeM, var) / 8;
         unsigned int address_bitsize;
         for(address_bitsize = 1; max_addr > (1ull << address_bitsize); ++address_bitsize)
            ;
         return address_bitsize;
      }
   }
   return precision;
}

bool mux_connection_binding::isZeroObj(unsigned int tree_index, const tree_managerRef TreeM)
{
   tree_nodeRef tn = TreeM->get_tree_node_const(tree_index);
   if(GetPointer<integer_cst>(tn))
   {
      auto* curr_int = GetPointer<integer_cst>(tn);
      auto val = tree_helper::get_integer_cst_value(curr_int);
      return val == 0;
   }
   else
      return false;
}

bool mux_connection_binding::isConstantObj(unsigned int tree_index, const tree_managerRef TreeM)
{
   if(tree_index == 0)
      return true;
   tree_nodeRef tn = TreeM->get_tree_node_const(tree_index);
   if(GetPointer<integer_cst>(tn))
      return true;
   else
      return false;
}

void mux_connection_binding::determine_connection(const vertex& op, const HLS_manager::io_binding_type& _var, generic_objRef fu_obj, unsigned int port_num, unsigned int port_index, const OpGraphConstRef data, unsigned int precision, unsigned int alignment)
{
   bool is_not_a_phi = (GET_TYPE(data, op) & TYPE_PHI) == 0;
   unsigned int tree_var = std::get<0>(_var);
   unsigned long long int constant_value = std::get<1>(_var);
   unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
   bus_addr_bitsize = std::min(precision, bus_addr_bitsize);
   memory_symbolRef m_sym;
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);

   if(tree_var)
   {
      const tree_managerRef TreeM = HLSMgr->get_tree_manager();
      tree_nodeRef tn = TreeM->get_tree_node_const(tree_var);
      switch(tn->get_kind())
      {
         case addr_expr_K:
         {
            auto* ae = GetPointer<addr_expr>(tn);
            unsigned int node_id = data->CGetOpNodeInfo(op)->GetNodeId();
            const tree_nodeRef node = TreeM->get_tree_node_const(node_id);
            auto* gm = GetPointer<gimple_assign>(node);
            unsigned int return_index;
            const tree_nodeRef type = tree_helper::get_type_node(GET_NODE(ae->op), return_index);
            if(type && GetPointer<type_node>(type))
            {
#if USE_ALIGNMENT_INFO
               if(alignment)
                  alignment = std::min(alignment, GetPointer<type_node>(type)->algn);
               else
                  alignment = GetPointer<type_node>(type)->algn;
#endif
            }
            if(gm && gm->temporary_address)
            {
               unsigned int ref_var = tree_helper::get_base_index(TreeM, GET_INDEX_NODE(gm->op0));
               unsigned local_precision = bus_addr_bitsize;
               if(FB->is_variable_mem(ref_var))
               {
                  unsigned long long int max_addr = HLSMgr->Rmem->get_base_address(ref_var, HLS->functionId) + tree_helper::size(TreeM, ref_var) / 8;
                  for(local_precision = 1; max_addr > (1ull << local_precision); ++local_precision)
                     ;
               }
               determine_connection(op, HLS_manager::io_binding_type(GET_INDEX_NODE(ae->op), 0), fu_obj, port_num, port_index, data, local_precision, alignment);
            }
            else
            {
               determine_connection(op, HLS_manager::io_binding_type(GET_INDEX_NODE(ae->op), 0), fu_obj, port_num, port_index, data, precision, alignment);
            }
            return;
         }
         case array_ref_K:
         {
            auto* ar = GetPointer<array_ref>(tn);
            if(GET_NODE(ar->op0)->get_kind() == array_ref_K) /// dynamic multidimensional array case
            {
               unsigned int base_address_index = 0;
               std::vector<unsigned int> recursive_indexes_values;
               std::vector<unsigned int> dims;
               generic_objRef global_adder;
               mux_connection_binding::dynamic_multidimensional_array_handler(ar, op, data, base_address_index, recursive_indexes_values, dims, global_adder, is_not_a_phi);
               generic_objRef offset_calculator_port = generic_objRef(new multiplier_conn_obj("multiplier_conn_obj_" + STR(id++)));
               HLS->Rconn->add_sparse_logic(offset_calculator_port);
               unsigned int local_precision = address_precision(precision, op, data, TreeM);
               GetPointer<multiplier_conn_obj>(offset_calculator_port)->add_bitsize(local_precision);
               // global adder into port 0
               create_single_conn(data, op, global_adder, offset_calculator_port, 0, 0, 0, local_precision, is_not_a_phi);

               // step into port 1
               unsigned int step = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, tree_var)) / 8;
               GetPointer<multiplier_conn_obj>(offset_calculator_port)->set_multiplication_to_constant(step);
               determine_connection(op, HLS_manager::io_binding_type(0, step), offset_calculator_port, 1, 0, data, local_precision, alignment);
               generic_objRef dynamic_port = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
               if(alignment)
               {
                  GetPointer<adder_conn_obj>(dynamic_port)->set_trimmed_bits(align_to_trimmed_bits(alignment));
               }
               HLS->Rconn->add_sparse_logic(dynamic_port);
               GetPointer<adder_conn_obj>(dynamic_port)->add_bitsize(local_precision);
               // offset calculator into port 0
               create_single_conn(data, op, offset_calculator_port, dynamic_port, 0, 0, 0, local_precision, is_not_a_phi);

               // base address into port 1
               determine_connection(op, HLS_manager::io_binding_type(base_address_index, 0), dynamic_port, 1, 0, data, local_precision, alignment);
               create_single_conn(data, op, dynamic_port, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            }
            else
            {
               // determines address offset
               generic_objRef offset_calculator_port = generic_objRef(new multiplier_conn_obj("multiplier_conn_obj_" + STR(id++)));
               HLS->Rconn->add_sparse_logic(offset_calculator_port);
               unsigned int tree_index = GET_INDEX_NODE(ar->op1);
               unsigned int ar_index = GET_INDEX_NODE(ar->op0);
               unsigned int local_precision = address_precision(precision, op, data, TreeM);
               GetPointer<multiplier_conn_obj>(offset_calculator_port)->add_bitsize(local_precision);
               connect_array_index(tree_index, offset_calculator_port, 0, 0, local_precision, data, op);

               unsigned int step = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, tree_var)) / 8;
               GetPointer<multiplier_conn_obj>(offset_calculator_port)->set_multiplication_to_constant(step);
               determine_connection(op, HLS_manager::io_binding_type(0, step), offset_calculator_port, 1, 0, data, local_precision, alignment);

               // determines address from base and offset
               generic_objRef dynamic_port = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
               GetPointer<adder_conn_obj>(dynamic_port)->add_bitsize(local_precision);
               if(alignment)
               {
                  GetPointer<adder_conn_obj>(dynamic_port)->set_trimmed_bits(align_to_trimmed_bits(alignment));
               }
               HLS->Rconn->add_sparse_logic(dynamic_port);
               create_single_conn(data, op, offset_calculator_port, dynamic_port, 0, 0, 0, local_precision, is_not_a_phi);
               determine_connection(op, HLS_manager::io_binding_type(ar_index, 0), dynamic_port, 1, 0, data, local_precision, alignment);

               create_single_conn(data, op, dynamic_port, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            }
            return;
         }
         case component_ref_K:
         {
            auto* cr = GetPointer<component_ref>(tn);

            unsigned int base_index = GET_INDEX_NODE(cr->op0);
            auto* fd = GetPointer<field_decl>(GET_NODE(cr->op1));
            THROW_ASSERT(fd, "expected an field_decl but got something of different");
            THROW_ASSERT(!fd->is_bitfield(), "bitfield not yet supported: " + fd->ToString());
            auto* curr_int = GetPointer<integer_cst>(GET_NODE(fd->bpos));
            THROW_ASSERT(curr_int, "expected an integer_cst but got something of different");
            auto offset = static_cast<unsigned int>(tree_helper::get_integer_cst_value(curr_int));
            if(offset % 8 != 0)
               THROW_ERROR("bitfields are not yet supported");
#if USE_ALIGNMENT_INFO
            alignment = offset & (alignment - 1);
#endif
            generic_objRef address_port = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
            unsigned int local_precision = address_precision(precision, op, data, TreeM);
            GetPointer<adder_conn_obj>(address_port)->add_bitsize(local_precision);
            if(alignment)
            {
               GetPointer<adder_conn_obj>(address_port)->set_trimmed_bits(align_to_trimmed_bits(alignment));
            }
            HLS->Rconn->add_sparse_logic(address_port);

            determine_connection(op, HLS_manager::io_binding_type(base_index, 0), address_port, 0, 0, data, local_precision, alignment);
            determine_connection(op, HLS_manager::io_binding_type(0, offset / 8), address_port, 1, 0, data, local_precision, alignment);
            create_single_conn(data, op, address_port, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            return;
         }
         case misaligned_indirect_ref_K:
         {
            auto* mir = GetPointer<misaligned_indirect_ref>(tn);
            if(GetPointer<ssa_name>(GET_NODE(mir->op)))
            {
               tree_var = GET_INDEX_NODE(mir->op);
            }
            else
               THROW_ERROR("determine_connection-misaligned_indirect_ref_K pattern not supported: " + std::string(tn->get_kind_text()) + " @" + STR(tree_var));
            break;
         }
         case indirect_ref_K:
         {
            auto* ir = GetPointer<indirect_ref>(tn);
            if(GetPointer<ssa_name>(GET_NODE(ir->op)))
            {
               tree_var = GET_INDEX_NODE(ir->op);
            }
            else if(GetPointer<integer_cst>(GET_NODE(ir->op)))
            {
               constant_value = static_cast<unsigned int>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(ir->op))));
               tree_var = 0;
               precision = address_precision(bus_addr_bitsize, op, data, TreeM);
            }
            else if(GetPointer<addr_expr>(GET_NODE(ir->op)))
            {
               unsigned int local_precision = address_precision(precision, op, data, TreeM);
               determine_connection(op, HLS_manager::io_binding_type(GET_INDEX_NODE(ir->op), 0), fu_obj, port_num, port_index, data, local_precision, alignment);
               return;
            }
            else
               THROW_ERROR("determine_connection-indirect_ref_K pattern not supported: " + std::string(tn->get_kind_text()) + " @" + STR(tree_var));
            break;
         }
         case mem_ref_K:
         {
            auto* mr = GetPointer<mem_ref>(tn);
            unsigned int base_index = GET_INDEX_NODE(mr->op0);
            long long int offset = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(mr->op1)));
            unsigned int offset_index = offset ? GET_INDEX_NODE(mr->op1) : 0;
            generic_objRef current_operand;
            unsigned int local_precision = address_precision(precision, op, data, TreeM);
            if(offset_index)
            {
#if USE_ALIGNMENT_INFO
               long long int cost_val = offset;
               alignment = std::min(static_cast<unsigned int>(8 * (cost_val & -cost_val)), alignment);
#endif
               current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
               GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
               if(alignment)
               {
                  GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
               }
               HLS->Rconn->add_sparse_logic(current_operand);
               determine_connection(op, HLS_manager::io_binding_type(base_index, 0), current_operand, 0, 0, data, local_precision, alignment);
               determine_connection(op, HLS_manager::io_binding_type(offset_index, 0), current_operand, 1, 0, data, local_precision, alignment);

               create_single_conn(data, op, current_operand, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            }
            else
               determine_connection(op, HLS_manager::io_binding_type(base_index, 0), fu_obj, port_num, port_index, data, local_precision, alignment);
            return;
         }
         case target_mem_ref_K:
         {
            auto* tmr = GetPointer<target_mem_ref>(tn);
            unsigned int symbol_index = tmr->symbol ? GET_INDEX_NODE(tmr->symbol) : 0;
            unsigned int base_index = tmr->base ? GET_INDEX_NODE(tmr->base) : 0;
            unsigned int idx_index = tmr->idx ? GET_INDEX_NODE(tmr->idx) : 0;
            unsigned int step_index = tmr->step ? GET_INDEX_NODE(tmr->step) : 0;
            unsigned int offset_index = tmr->offset ? GET_INDEX_NODE(tmr->offset) : 0;
            /// symbol_index + base_index + idx_index * step_index + offset_index

            generic_objRef current_operand, previous_operand;
            generic_objRef idx_step_port;
            unsigned int previous_index = symbol_index;
            unsigned int current_index = base_index;
            unsigned int local_precision = address_precision(precision, op, data, TreeM);
#if USE_ALIGNMENT_INFO
            if(offset_index)
            {
               auto* curr_int = GetPointer<integer_cst>(GET_NODE(tmr->offset));
               long long int cost_val = tree_helper::get_integer_cst_value(curr_int);
               unsigned int offset = 8 * static_cast<unsigned int>(cost_val & -cost_val);
               if(offset < alignment)
                  alignment = offset & (alignment - 1);
            }
            if(step_index)
            {
               auto* curr_int = GetPointer<integer_cst>(GET_NODE(tmr->step));
               alignment = std::min(static_cast<unsigned int>((8 * (tree_helper::get_integer_cst_value(curr_int) & -tree_helper::get_integer_cst_value(curr_int)))), alignment);
            }
            else if(idx_index)
               alignment = 8;
#endif
            if(current_index)
            {
               if(previous_index)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  determine_connection(op, HLS_manager::io_binding_type(current_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), current_operand, 1, 0, data, local_precision, alignment);
               }
               previous_operand = current_operand;
               previous_index = current_index;
            }

            if(idx_index)
            {
               if(step_index)
               {
                  idx_step_port = generic_objRef(new multiplier_conn_obj("multiplier_conn_obj_" + STR(id++)));
                  GetPointer<multiplier_conn_obj>(idx_step_port)->add_bitsize(local_precision);
                  HLS->Rconn->add_sparse_logic(idx_step_port);
                  determine_connection(op, HLS_manager::io_binding_type(idx_index, 0), idx_step_port, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(step_index, 0), idx_step_port, 1, 0, data, local_precision, alignment);
               }

               if(previous_operand)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  if(step_index)
                  {
                     create_single_conn(data, op, idx_step_port, current_operand, 0, 0, 0, local_precision, is_not_a_phi);
                  }
                  else
                     determine_connection(op, HLS_manager::io_binding_type(idx_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  create_single_conn(data, op, previous_operand, current_operand, 1, 0, 0, local_precision, is_not_a_phi);
                  previous_operand = current_operand;
               }
               else if(previous_index)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  if(step_index)
                  {
                     create_single_conn(data, op, idx_step_port, current_operand, 0, 0, 0, local_precision, is_not_a_phi);
                  }
                  else
                     determine_connection(op, HLS_manager::io_binding_type(idx_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), current_operand, 1, 0, data, local_precision, alignment);
                  previous_operand = current_operand;
               }
               else if(step_index)
                  previous_operand = idx_step_port;
               previous_index = idx_index;
            }

            current_index = offset_index;
            if(current_index)
            {
               if(previous_operand)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  determine_connection(op, HLS_manager::io_binding_type(current_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  create_single_conn(data, op, previous_operand, current_operand, 1, 0, 0, local_precision, is_not_a_phi);
                  previous_operand = current_operand;
               }
               else if(previous_index)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  determine_connection(op, HLS_manager::io_binding_type(current_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), current_operand, 1, 0, data, local_precision, alignment);
                  previous_operand = current_operand;
               }
               else
                  previous_index = current_index;
            }

            if(previous_operand)
            {
               create_single_conn(data, op, previous_operand, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            }
            else
               determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), fu_obj, port_num, port_index, data, local_precision, alignment);
            return;
         }
         case target_mem_ref461_K:
         {
            auto* tmr = GetPointer<target_mem_ref461>(tn);
            unsigned int idx2_index = tmr->idx2 ? GET_INDEX_NODE(tmr->idx2) : 0;
            unsigned int base_index = tmr->base ? GET_INDEX_NODE(tmr->base) : 0;
            unsigned int idx_index = tmr->idx ? GET_INDEX_NODE(tmr->idx) : 0;
            unsigned int step_index = tmr->step ? GET_INDEX_NODE(tmr->step) : 0;
            unsigned int offset_index = tmr->offset ? GET_INDEX_NODE(tmr->offset) : 0;
            /// idx2_index + base_index + idx_index * step_index + offset_index

            generic_objRef current_operand, previous_operand;
            generic_objRef idx_step_port;
            unsigned int previous_index = idx2_index;
            unsigned int current_index = base_index;
            unsigned int local_precision = address_precision(precision, op, data, TreeM);
#if USE_ALIGNMENT_INFO
            if(base_index)
            {
               tree_nodeRef base_type_n = tree_helper::get_type_node(GET_NODE(tmr->base));
               auto* pt = GetPointer<pointer_type>(base_type_n);
               if(pt)
               {
                  auto* ptd_type = GetPointer<type_node>(GET_NODE(pt->ptd));
                  alignment = std::min(ptd_type->algn, alignment);
               }
               else if(base_type_n->get_kind() == reference_type_K)
               {
                  auto* rt = GetPointer<reference_type>(base_type_n);
                  auto* rtd_type = GetPointer<type_node>(GET_NODE(rt->refd));
                  alignment = std::min(rtd_type->algn, alignment);
               }
            }
            if(offset_index)
            {
               auto* curr_int = GetPointer<integer_cst>(GET_NODE(tmr->offset));
               long long int cost_val = tree_helper::get_integer_cst_value(curr_int);
               unsigned int offset = 8 * static_cast<unsigned int>(cost_val & -cost_val);
               if(offset < alignment)
                  alignment = offset & (alignment - 1);
            }
            if(step_index)
            {
               auto* curr_int = GetPointer<integer_cst>(GET_NODE(tmr->step));
               alignment = std::min(static_cast<unsigned int>((8 * (tree_helper::get_integer_cst_value(curr_int) & -tree_helper::get_integer_cst_value(curr_int)))), alignment);
            }
            else if(idx_index)
            {
               alignment = 8;
            }
            if(idx2_index)
            {
               alignment = 8;
            }
#endif
            if(current_index)
            {
               if(previous_index)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  determine_connection(op, HLS_manager::io_binding_type(current_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), current_operand, 1, 0, data, local_precision, alignment);
               }
               previous_operand = current_operand;
               previous_index = current_index;
            }

            if(idx_index)
            {
               if(step_index)
               {
                  idx_step_port = generic_objRef(new multiplier_conn_obj("multiplier_conn_obj_" + STR(id++)));
                  GetPointer<multiplier_conn_obj>(idx_step_port)->add_bitsize(local_precision);
                  HLS->Rconn->add_sparse_logic(idx_step_port);
                  determine_connection(op, HLS_manager::io_binding_type(idx_index, 0), idx_step_port, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(step_index, 0), idx_step_port, 1, 0, data, local_precision, alignment);
               }

               if(previous_operand)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  if(step_index)
                  {
                     create_single_conn(data, op, idx_step_port, current_operand, 0, 0, 0, local_precision, is_not_a_phi);
                  }
                  else
                     determine_connection(op, HLS_manager::io_binding_type(idx_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  create_single_conn(data, op, previous_operand, current_operand, 1, 0, 0, local_precision, is_not_a_phi);
                  previous_operand = current_operand;
               }
               else if(previous_index)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  if(step_index)
                  {
                     create_single_conn(data, op, idx_step_port, current_operand, 0, 0, 0, local_precision, is_not_a_phi);
                  }
                  else
                     determine_connection(op, HLS_manager::io_binding_type(idx_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), current_operand, 1, 0, data, local_precision, alignment);
                  previous_operand = current_operand;
               }
               else if(step_index)
                  previous_operand = idx_step_port;
               previous_index = idx_index;
            }

            current_index = offset_index;
            if(current_index)
            {
               if(previous_operand)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  determine_connection(op, HLS_manager::io_binding_type(current_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  create_single_conn(data, op, previous_operand, current_operand, 1, 0, 0, local_precision, is_not_a_phi);
                  previous_operand = current_operand;
               }
               else if(previous_index)
               {
                  current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
                  GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
                  if(alignment)
                  {
                     GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
                  }
                  HLS->Rconn->add_sparse_logic(current_operand);
                  determine_connection(op, HLS_manager::io_binding_type(current_index, 0), current_operand, 0, 0, data, local_precision, alignment);
                  determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), current_operand, 1, 0, data, local_precision, alignment);
                  previous_operand = current_operand;
               }
               else
                  previous_index = current_index;
            }

            if(previous_operand)
            {
               create_single_conn(data, op, previous_operand, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            }
            else
               determine_connection(op, HLS_manager::io_binding_type(previous_index, 0), fu_obj, port_num, port_index, data, local_precision, alignment);
            return;
         }

         case realpart_expr_K: /// the first element of a complex object is the realpart so we just need the base_address
         {
            auto* rpe = GetPointer<realpart_expr>(tn);
            tree_var = GET_INDEX_NODE(rpe->op);
            if(HLSMgr->Rmem->has_base_address(tree_var))
            {
               m_sym = HLSMgr->Rmem->get_symbol(tree_var, HLS->functionId);
               constant_value = HLSMgr->Rmem->get_base_address(tree_var, HLS->functionId);
               tree_var = 0;
               precision = address_precision(bus_addr_bitsize, op, data, TreeM);
            }
            else
            {
               determine_connection(op, HLS_manager::io_binding_type(GET_INDEX_NODE(rpe->op), 0), fu_obj, port_num, port_index, data, precision, alignment);
               return;
            }
            break;
         }
         case imagpart_expr_K:
         {
            auto* ipe = GetPointer<imagpart_expr>(tn);
            unsigned int base_index = GET_INDEX_NODE(ipe->op);
            unsigned int offset = tree_helper::size(TreeM, tree_helper::get_type_index(TreeM, base_index)) / 16;
#if USE_ALIGNMENT_INFO
            alignment = (8 * offset) & (alignment - 1);
#endif
            generic_objRef address_port = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
            unsigned int local_precision = address_precision(precision, op, data, TreeM);
            GetPointer<adder_conn_obj>(address_port)->add_bitsize(local_precision);
            if(alignment)
            {
               GetPointer<adder_conn_obj>(address_port)->set_trimmed_bits(align_to_trimmed_bits(alignment));
            }
            HLS->Rconn->add_sparse_logic(address_port);

            determine_connection(op, HLS_manager::io_binding_type(base_index, 0), address_port, 0, 0, data, local_precision, alignment);
            determine_connection(op, HLS_manager::io_binding_type(0, offset), address_port, 1, 0, data, local_precision, alignment);
            create_single_conn(data, op, address_port, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            return;
         }
         case string_cst_K:
         case integer_cst_K:
         case real_cst_K:
         case vector_cst_K:
         case void_cst_K:
         case complex_cst_K:
         case ssa_name_K:
         case var_decl_K:
         {
            if(HLSMgr->Rmem->has_base_address(tree_var))
            {
               m_sym = HLSMgr->Rmem->get_symbol(tree_var, HLS->functionId);
               constant_value = HLSMgr->Rmem->get_base_address(tree_var, HLS->functionId);
               tree_var = 0;
               precision = bus_addr_bitsize;
            }
            // else a direct connection is considered
            break;
         }
         case parm_decl_K:
         {
            if(HLSMgr->Rmem->has_base_address(tree_var))
            {
               if(HLSMgr->Rmem->is_parm_decl_copied(tree_var) || HLSMgr->Rmem->is_parm_decl_stored(tree_var))
               {
                  m_sym = HLSMgr->Rmem->get_symbol(tree_var, HLS->functionId);
                  constant_value = HLSMgr->Rmem->get_base_address(tree_var, HLS->functionId);
                  tree_var = 0;
                  precision = bus_addr_bitsize;
               }
               else
               {
                  /// else a direct connection is considered since adresses are passed
                  precision = bus_addr_bitsize;
               }
            }
            /// else a direct connection is considered
            break;
         }
         case view_convert_expr_K:
         {
            auto* vc = GetPointer<view_convert_expr>(tn);
            if(GetPointer<ssa_name>(GET_NODE(vc->op)))
            {
               auto* sn = GetPointer<ssa_name>(GET_NODE(vc->op));
               auto* pd = GetPointer<parm_decl>(GET_NODE(sn->var));
               if(pd)
               {
                  THROW_ASSERT(HLSMgr->Rmem->has_base_address(GET_INDEX_NODE(sn->var)), "expected a parm_decl allocated in memory");
                  m_sym = HLSMgr->Rmem->get_symbol(GET_INDEX_NODE(sn->var), HLS->functionId);
                  constant_value = HLSMgr->Rmem->get_base_address(GET_INDEX_NODE(sn->var), HLS->functionId);
                  tree_var = 0;
                  precision = bus_addr_bitsize;
               }
               else
                  THROW_ERROR("determine_connection view_convert_expr currently not supported: " + GET_NODE(vc->op)->get_kind_text() + " @" + STR(tree_var));
            }
            else if(GetPointer<var_decl>(GET_NODE(vc->op)))
            {
               THROW_ASSERT(HLSMgr->Rmem->has_base_address(GET_INDEX_NODE(vc->op)), "expected a var_decl allocated in memory");
               m_sym = HLSMgr->Rmem->get_symbol(GET_INDEX_NODE(vc->op), HLS->functionId);
               constant_value = HLSMgr->Rmem->get_base_address(GET_INDEX_NODE(vc->op), HLS->functionId);
               tree_var = 0;
               precision = bus_addr_bitsize;
            }
            else
               THROW_ERROR("determine_connection view_convert_expr currently not supported: " + GET_NODE(vc->op)->get_kind_text() + " @" + STR(tree_var));
            break;
         }
         case function_decl_K:
         {
            m_sym = HLSMgr->Rmem->get_symbol(tree_var, tree_var);
            constant_value = HLSMgr->Rmem->get_base_address(tree_var, tree_var);
            tree_var = 0;
            precision = bus_addr_bitsize;
            break;
         }
         case bit_field_ref_K:
         {
            auto* bf = GetPointer<bit_field_ref>(tn);
            long long int bpos = tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(bf->op2)));
            if(bpos % 8)
               THROW_ERROR_CODE(BITFIELD_EC, "Bitfield LOAD/STORE not yet supported @" + std::to_string(tree_var));
            /// check bitsize
            auto bsize = static_cast<unsigned int>(tree_helper::get_integer_cst_value(GetPointer<integer_cst>(GET_NODE(bf->op1))));
            if(bsize == 1 || resize_to_1_8_16_32_64_128_256_512(bsize) != bsize)
               THROW_ERROR_CODE(BITFIELD_EC, "Bitfield LOAD/STORE not yet supported @" + std::to_string(tree_var));
            auto offset = static_cast<unsigned int>(bpos / 8);
#if USE_ALIGNMENT_INFO
            alignment = offset & (alignment - 1);
#endif
            generic_objRef current_operand = generic_objRef(new adder_conn_obj("adder_conn_obj_" + STR(id++)));
            unsigned int local_precision = address_precision(precision, op, data, TreeM);
            GetPointer<adder_conn_obj>(current_operand)->add_bitsize(local_precision);
            if(alignment)
            {
               GetPointer<adder_conn_obj>(current_operand)->set_trimmed_bits(align_to_trimmed_bits(alignment));
            }
            HLS->Rconn->add_sparse_logic(current_operand);
            determine_connection(op, HLS_manager::io_binding_type(GET_INDEX_NODE(bf->op0), 0), current_operand, 0, 0, data, local_precision, alignment);
            determine_connection(op, HLS_manager::io_binding_type(0, offset), current_operand, 1, 0, data, local_precision, alignment);
            create_single_conn(data, op, current_operand, fu_obj, port_num, port_index, 0, local_precision, is_not_a_phi);
            return;
         }
         case constructor_K:
         {
            THROW_ERROR("determine_connection pattern not supported: constructor as right part of an assignment");
            break;
         }
         case abs_expr_K:
         case arrow_expr_K:
         case bit_not_expr_K:
         case buffer_ref_K:
         case card_expr_K:
         case cleanup_point_expr_K:
         case conj_expr_K:
         case convert_expr_K:
         case exit_expr_K:
         case fix_ceil_expr_K:
         case fix_floor_expr_K:
         case fix_round_expr_K:
         case fix_trunc_expr_K:
         case float_expr_K:
         case loop_expr_K:
         case lut_expr_K:
         case negate_expr_K:
         case non_lvalue_expr_K:
         case nop_expr_K:
         case reference_expr_K:
         case reinterpret_cast_expr_K:
         case sizeof_expr_K:
         case static_cast_expr_K:
         case throw_expr_K:
         case truth_not_expr_K:
         case unsave_expr_K:
         case va_arg_expr_K:
         case reduc_max_expr_K:
         case reduc_min_expr_K:
         case reduc_plus_expr_K:
         case vec_unpack_hi_expr_K:
         case vec_unpack_lo_expr_K:
         case vec_unpack_float_hi_expr_K:
         case vec_unpack_float_lo_expr_K:
         case assert_expr_K:
         case bit_and_expr_K:
         case bit_ior_expr_K:
         case bit_xor_expr_K:
         case catch_expr_K:
         case ceil_div_expr_K:
         case ceil_mod_expr_K:
         case complex_expr_K:
         case compound_expr_K:
         case eh_filter_expr_K:
         case eq_expr_K:
         case exact_div_expr_K:
         case fdesc_expr_K:
         case floor_div_expr_K:
         case floor_mod_expr_K:
         case ge_expr_K:
         case gt_expr_K:
         case goto_subroutine_K:
         case in_expr_K:
         case init_expr_K:
         case le_expr_K:
         case lrotate_expr_K:
         case lshift_expr_K:
         case lt_expr_K:
         case max_expr_K:
         case min_expr_K:
         case minus_expr_K:
         case modify_expr_K:
         case mult_expr_K:
         case mult_highpart_expr_K:
         case ne_expr_K:
         case ordered_expr_K:
         case paren_expr_K:
         case plus_expr_K:
         case pointer_plus_expr_K:
         case postdecrement_expr_K:
         case postincrement_expr_K:
         case predecrement_expr_K:
         case preincrement_expr_K:
         case range_expr_K:
         case rdiv_expr_K:
         case round_div_expr_K:
         case round_mod_expr_K:
         case rrotate_expr_K:
         case rshift_expr_K:
         case set_le_expr_K:
         case trunc_div_expr_K:
         case trunc_mod_expr_K:
         case truth_and_expr_K:
         case truth_andif_expr_K:
         case truth_or_expr_K:
         case truth_orif_expr_K:
         case truth_xor_expr_K:
         case try_catch_expr_K:
         case try_finally_K:
         case uneq_expr_K:
         case ltgt_expr_K:
         case unge_expr_K:
         case ungt_expr_K:
         case unle_expr_K:
         case unlt_expr_K:
         case unordered_expr_K:
         case widen_sum_expr_K:
         case widen_mult_expr_K:
         case with_size_expr_K:
         case vec_lshift_expr_K:
         case vec_rshift_expr_K:
         case widen_mult_hi_expr_K:
         case widen_mult_lo_expr_K:
         case vec_cond_expr_K:
         case vec_pack_trunc_expr_K:
         case vec_pack_sat_expr_K:
         case vec_pack_fix_trunc_expr_K:
         case vec_perm_expr_K:
         case vec_extracteven_expr_K:
         case vec_extractodd_expr_K:
         case vec_interleavehigh_expr_K:
         case vec_interleavelow_expr_K:
         case CASE_CPP_NODES:
         case const_decl_K:
         case field_decl_K:
         case label_decl_K:
         case namespace_decl_K:
         case result_decl_K:
         case translation_unit_decl_K:
         case error_mark_K:
         case using_decl_K:
         case type_decl_K:
         case template_decl_K:
         case CASE_GIMPLE_NODES:
         case call_expr_K:
         case aggr_init_expr_K:
         case case_label_expr_K:
         case CASE_FAKE_NODES:
         case CASE_PRAGMA_NODES:
         case binfo_K:
         case block_K:
         case identifier_node_K:
         case statement_list_K:
         case tree_list_K:
         case tree_vec_K:
         case array_range_ref_K:
         case target_expr_K:
         case cond_expr_K:
         case dot_prod_expr_K:
         case ternary_plus_expr_K:
         case ternary_pm_expr_K:
         case ternary_mp_expr_K:
         case ternary_mm_expr_K:
         case bit_ior_concat_expr_K:
         case obj_type_ref_K:
         case save_expr_K:
         case vtable_ref_K:
         case with_cleanup_expr_K:
         case extract_bit_expr_K:
         case CASE_TYPE_NODES:
         default:
            THROW_ERROR("determine_connection pattern not supported: " + std::string(tn->get_kind_text()) + " @" + STR(tree_var));
      }
   }

   if(tree_var == 0)
   {
      /// create connection with the constant
      THROW_ASSERT(precision, "a precision greater than 0 is expected");

      std::string string_value = convert_to_binary(static_cast<unsigned long long int>(constant_value), precision);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - Constant value: " + STR(constant_value));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - " + string_value);
      std::string param_name;
      if(m_sym)
      {
         param_name = m_sym->get_symbol_name();
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - param: " + param_name);
         string_value = STR(m_sym->get_address());
      }
      generic_objRef C_obj = HLS->Rconn->get_constant_obj(string_value, param_name, precision);
      create_single_conn(data, op, C_obj, fu_obj, port_num, port_index, 0, precision, is_not_a_phi);
      return;
   }

   const BehavioralHelperConstRef behavioral_helper = FB->CGetBehavioralHelper();
   if(behavioral_helper->is_a_constant(tree_var))
   {
      THROW_ASSERT(precision, "a precision greater than 0 is expected");
      std::string C_value = HLSMgr->get_constant_string(tree_var, precision);
      /*if(behavioral_helper->is_unsigned(tree_var))
         C_value = "0"+C_value;*/
      generic_objRef C_obj = HLS->Rconn->get_constant_obj(C_value, "", precision);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - Tree constant value: " + behavioral_helper->PrintVariable(tree_var));
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "         - " + C_value);
      create_single_conn(data, op, C_obj, fu_obj, port_num, port_index, tree_var, precision, is_not_a_phi);
      return;
   }

   connect_to_registers(op, data, fu_obj, port_num, port_index, tree_var, precision, is_not_a_phi);
}

unsigned int mux_connection_binding::extract_parm_decl(unsigned int tree_var, const tree_managerRef TreeM)
{
   unsigned int base_index;
   tree_nodeRef node = TreeM->get_tree_node_const(tree_var);
   if(GetPointer<parm_decl>(node))
      base_index = tree_var;
   else
   {
      auto* sn = GetPointer<ssa_name>(node);
      base_index = GET_INDEX_NODE(sn->var);
   }
   return base_index;
}

void mux_connection_binding::connect_to_registers(vertex op, const OpGraphConstRef data, generic_objRef fu_obj, unsigned int port_num, unsigned int port_index, unsigned int tree_var, unsigned int precision, const bool is_not_a_phi)
{
   THROW_ASSERT(tree_var, "a non-null tree var is expected");
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(op);
   const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
   last_intermediate_state fetch_previous(HLS->STG->GetStg(), HLSMgr->CGetFunctionBehavior(funId)->build_simple_pipeline());
   next_unique_state get_next(HLS->STG->GetStg());
   for(auto rs_it = running_states.begin(); rs_it_end != rs_it; ++rs_it)
   {
      unsigned int tree_var_state_in;
      if(!is_not_a_phi)
      {
         THROW_ASSERT(not HLSMgr->GetFunctionBehavior(HLS->functionId)->build_simple_pipeline(), "A pipelined function should not contain any phi operations");
         const StateInfoConstRef state_info = is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(*rs_it);
         if(state_info && state_info->is_duplicated && !state_info->all_paths)
         {
            bool found_branch = false;
            const auto gp = GetPointer<const gimple_phi>(TreeM->get_tree_node_const(data->CGetOpNodeInfo(op)->GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               unsigned int bbID = def_edge.second;
               if(!state_info->isOriginalState && bbID != state_info->sourceBb)
                  continue;
               else if(state_info->isOriginalState && bbID == state_info->sourceBb)
                  continue;
               else if(state_info->moved_op_def_set.find(cur_phi_tree_var) != state_info->moved_op_def_set.end())
                  continue;
               else if(state_info->moved_op_use_set.find(gp->res->index) != state_info->moved_op_use_set.end())
                  continue;
               else if(def_edge.first->index != cur_phi_tree_var)
                  continue;
               found_branch = true;
               break;
            }
            if(!found_branch)
               continue;
         }
         tree_var_state_in = cur_phi_tree_var;
      }
      else
         tree_var_state_in = tree_var;
      THROW_ASSERT(HLS->Rliv->has_state_in(*rs_it, op, tree_var_state_in), " no state in for @" + STR(tree_var_state_in) + " - " + (is_not_a_phi ? " not a phi" : "phi") + " - " + HLS->Rliv->get_name(*rs_it) + " for op " + GET_NAME(data, op));
      const CustomOrderedSet<vertex>& states_in = HLS->Rliv->get_state_in(*rs_it, op, tree_var_state_in);
      const CustomOrderedSet<vertex>::const_iterator s_in_it_end = states_in.end();
      for(auto s_in_it = states_in.begin(); s_in_it != s_in_it_end; ++s_in_it)
      {
         vertex state = *rs_it;
         generic_objRef reg_obj;

         if(!is_not_a_phi)
         {
            THROW_ASSERT(not HLSMgr->GetFunctionBehavior(HLS->functionId)->build_simple_pipeline(), "A pipelined function should not contain any phi operations");
            vertex src_state = *s_in_it;
            if(src_state == NULL_VERTEX)
               std::swap(src_state, state);
            if(tree_helper::is_parameter(TreeM, tree_var))
            {
               unsigned int base_index = extract_parm_decl(tree_var, TreeM);
               const generic_objRef fu_src_obj = input_ports[base_index];
               THROW_ASSERT(fu_src_obj, "unexpected condition");
               HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, src_state, state, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from primary input " << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                              << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
            }
            else
            {
               vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
               const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
               if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
               {
                  if(def_op_ending_states.find(src_state) != def_op_ending_states.end())
                  {
                     const generic_objRef fu_src_obj = HLS->Rfu->get(def_op);
                     HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, src_state, state, op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                      << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  }
                  else if(HLS->storage_value_information->is_a_storage_value(fetch_previous(src_state, state), tree_var))
                  {
                     unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(fetch_previous(src_state, state), tree_var);
                     unsigned int r_index = HLS->Rreg->get_register(storage_value);
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - register: " << r_index << " from " << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     reg_obj = HLS->Rreg->get(r_index);
                     if(reg_obj != fu_obj)
                     {
                        HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, src_state, state, op));
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                      "       - add data transfer from " << reg_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                         << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     }
                  }
                  else
                  {
                     THROW_ERROR("not expected from " + HLS->Rliv->get_name(src_state) + " to " + HLS->Rliv->get_name(state) + " " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  }
               }
               else
               {
                  THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(fetch_previous(src_state, state), tree_var), "it has to be a register");
                  unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(fetch_previous(src_state, state), tree_var);
                  unsigned int r_index = HLS->Rreg->get_register(storage_value);
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "       - register: " << r_index << " from " << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  reg_obj = HLS->Rreg->get(r_index);
                  if(reg_obj != fu_obj)
                  {
                     HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, src_state, state, op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << reg_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                      << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  }
               }
            }
            if(GetPointer<register_obj>(fu_obj) && reg_obj != fu_obj)
            {
               generic_objRef enable_obj = GetPointer<register_obj>(fu_obj)->get_wr_enable();
               GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(fetch_previous(src_state, state), state, commandport_obj::data_operation_pair(tree_var_state_in, op)));
               GetPointer<commandport_obj>(enable_obj)->set_phi_write_enable();
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + fu_obj->get_string() + " from state " << HLS->Rliv->get_name(src_state) + " to state " + HLS->Rliv->get_name(state));
            }
         }
         else
         {
            vertex tgt_state = *s_in_it;
            if(tree_helper::is_parameter(TreeM, tree_var) && not HLSMgr->GetFunctionBehavior(HLS->functionId)->build_simple_pipeline())
            {
               unsigned int base_index = extract_parm_decl(tree_var, TreeM);
               const generic_objRef fu_src_obj = input_ports[base_index];
               THROW_ASSERT(fu_src_obj, "unexpected condition");
               HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, state, tgt_state, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from primary input " << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                              << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
            }
            else if(tree_helper::is_parameter(TreeM, tree_var))
            {
               // Primary inputs need a dedicated register per each state if we
               // want to preserve their values along pipeline steps
               unsigned int storage_value;
               unsigned int r_index;
               unsigned int tgt_port = port_num;
               unsigned int tgt_index = port_index;
               vertex previous = fetch_previous(HLS->STG->get_entry_state(), tgt_state);
               generic_objRef tgt_obj = fu_obj;
               if(tgt_state != get_next(HLS->STG->get_entry_state()))
               {
                  while(previous != get_next(HLS->STG->get_entry_state()))
                  {
                     THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(previous, tree_var), "The chain of registers propagating a primary input is broken");
                     storage_value = HLS->storage_value_information->get_storage_value_index(previous, tree_var);
                     r_index = HLS->Rreg->get_register(storage_value);
                     reg_obj = HLS->Rreg->get(r_index);
                     HLS->Rconn->add_data_transfer(reg_obj, tgt_obj, tgt_port, tgt_index, data_transfer(tree_var, precision, previous, tgt_state, op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << reg_obj->get_string() << " to " << tgt_obj->get_string() << " port " << std::to_string(tgt_port) << ":" << std::to_string(tgt_index) << " from state "
                                                                      << HLS->Rliv->get_name(previous) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     THROW_ASSERT(reg_obj != tgt_obj, "There is a loop in the propagation chain");
                     tgt_state = previous;
                     previous = fetch_previous(HLS->STG->get_entry_state(), tgt_state);
                     tgt_obj = reg_obj;
                     tgt_port = 0;
                     tgt_index = 0;
                  }
               }
               unsigned int base_index = extract_parm_decl(tree_var, TreeM);
               const generic_objRef fu_src_obj = input_ports[base_index];
               THROW_ASSERT(fu_src_obj, "unexpected condition");
               HLS->Rconn->add_data_transfer(fu_src_obj, tgt_obj, tgt_port, port_index, data_transfer(tree_var, precision, previous, tgt_state, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from primary input " << fu_src_obj->get_string() << " to " << tgt_obj->get_string() << " port " << std::to_string(tgt_port) << ":" << std::to_string(port_index) << " from state "
                                                                              << HLS->Rliv->get_name(previous) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " +
                                                                                     HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
            }
            else
            {
               vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
               const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
               const StateInfoConstRef state_info = is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(state);
               if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
               {
                  if(def_op_ending_states.find(state) != def_op_ending_states.end())
                  {
                     const generic_objRef fu_src_obj = HLS->Rfu->get(def_op);
                     HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, state, tgt_state, op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                      << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  }
                  else if(HLS->storage_value_information->is_a_storage_value(fetch_previous(state, tgt_state), tree_var))
                  {
                     unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(fetch_previous(state, tgt_state), tree_var);
                     unsigned int r_index = HLS->Rreg->get_register(storage_value);
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - register: " << r_index << " from " << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     reg_obj = HLS->Rreg->get(r_index);
                     THROW_ASSERT(not(reg_obj == fu_obj && HLSMgr->GetFunctionBehavior(HLS->functionId)->build_simple_pipeline()), "There can be no direct forwarding in pipelining");
                     if(reg_obj != fu_obj)
                     {
                        HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, state, tgt_state, op));
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                      "       - add data transfer from " << reg_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                         << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     }
                  }
                  else
                  {
                     THROW_UNREACHABLE("not expected from " + HLS->Rliv->get_name(state) + " to " + HLS->Rliv->get_name(tgt_state) + " " + HLSMgr->get_tree_manager()->get_tree_node_const(tree_var)->ToString());
                  }
               }
               else if(state_info && state_info->is_duplicated && state_info->clonedState != NULL_VERTEX && !state_info->all_paths && def_op_ending_states.find(state) != def_op_ending_states.end() &&
                       std::find(state_info->moved_exec_op.begin(), state_info->moved_exec_op.end(), op) == state_info->moved_exec_op.end())
               {
                  const auto gp = GetPointer<const gimple_phi>(TreeM->get_tree_node_const(data->CGetOpNodeInfo(def_op)->GetNodeId()));
                  bool phi_postponed = false;
                  unsigned int tree_temp = 0;
                  for(const auto& def_edge : gp->CGetDefEdgesList())
                  {
                     unsigned int bbID = def_edge.second;
                     tree_temp = def_edge.first->index;
                     if(bbID != state_info->sourceBb)
                        continue;
                     else if(state_info->moved_op_def_set.find(tree_temp) != state_info->moved_op_def_set.end())
                     {
                        phi_postponed = true;
                        break;
                     }
                     else if(state_info->moved_op_use_set.find(tree_var) != state_info->moved_op_use_set.end())
                     {
                        phi_postponed = true;
                        break;
                     }
                     else
                     {
                        break;
                     }
                  }
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Is phi postponed? " + (phi_postponed ? std::string("YES") : std::string("NO")));
                  if(phi_postponed)
                  {
                     // std::cerr << "phi postponed 0" << std::endl;
                     generic_objRef fu_src_obj;
                     if(state_info->moved_op_use_set.find(tree_var) != state_info->moved_op_use_set.end() && state_info->moved_op_def_set.find(tree_temp) == state_info->moved_op_def_set.end())
                     {
                        unsigned int src_storage_value = HLS->storage_value_information->get_storage_value_index(fetch_previous(state, tgt_state), tree_temp);
                        unsigned int src_r_index = HLS->Rreg->get_register(src_storage_value);
                        fu_src_obj = HLS->Rreg->get(src_r_index);
                     }
                     else
                     {
                        vertex src_def_op = HLS->Rliv->get_op_where_defined(tree_temp);
                        fu_src_obj = HLS->Rfu->get(src_def_op);
                     }
                     HLS->Rconn->add_data_transfer(fu_src_obj, fu_obj, port_num, port_index, data_transfer(tree_temp, precision, state, tgt_state, op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << fu_src_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                      << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_temp));
                  }
                  else
                  {
                     THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(fetch_previous(state, tgt_state), tree_var), "it has to be a register");
                     unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(fetch_previous(state, tgt_state), tree_var);
                     unsigned int r_index = HLS->Rreg->get_register(storage_value);
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - register: " << r_index << " from " << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     reg_obj = HLS->Rreg->get(r_index);
                     if(reg_obj != fu_obj)
                     {
                        HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, state, tgt_state, op));
                        PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                      "       - add data transfer from " << reg_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                         << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                     }
                  }
               }
               else
               {
                  THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(fetch_previous(state, tgt_state), tree_var), "it has to be a register");
                  unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(fetch_previous(state, tgt_state), tree_var);
                  unsigned int r_index = HLS->Rreg->get_register(storage_value);
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                "       - register: " << r_index << " from " << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  reg_obj = HLS->Rreg->get(r_index);
                  if(reg_obj != fu_obj)
                  {
                     HLS->Rconn->add_data_transfer(reg_obj, fu_obj, port_num, port_index, data_transfer(tree_var, precision, state, tgt_state, op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << reg_obj->get_string() << " to " << fu_obj->get_string() << " port " << std::to_string(port_num) << ":" << std::to_string(port_index) << " from state "
                                                                      << HLS->Rliv->get_name(state) + " to state " + HLS->Rliv->get_name(tgt_state) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
                  }
               }
            }
         }
      }
      if(HLSMgr->CGetFunctionBehavior(funId)->build_simple_pipeline())
      {
         vertex target;
         vertex previous;
         vertex def_op;
         vertex top;
         unsigned int origin_idx;
         unsigned int target_idx;
         unsigned int origin_reg_idx;
         unsigned int target_reg_idx;
         generic_objRef origin_reg;
         generic_objRef target_reg;
         CustomOrderedSet<unsigned int> in_vars = HLS->Rliv->get_live_in(*rs_it);
         for(auto& var : in_vars)
         {
            target = *rs_it;
            previous = fetch_previous(HLS->STG->get_entry_state(), target);
            def_op = HLS->Rliv->get_op_where_defined(var);
            THROW_ASSERT(HLS->Rliv->get_state_where_end(def_op).size() == 1, "A pipelined operation has more than one ending state");
            top = *HLS->Rliv->get_state_where_end(def_op).begin();
            THROW_ASSERT(target != top, "State defining a variable cannot have it as a live in variable");
            THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(target, var), "There is a live in variable without any register");
            while(previous != top)
            {
               THROW_ASSERT(HLS->storage_value_information->is_a_storage_value(previous, var), "There is a live in variable without any register");
               THROW_ASSERT(HLS->Rliv->get_live_in(previous).find(var) != HLS->Rliv->get_live_in(previous).end(), "The variable is not in live-in");
               THROW_ASSERT(HLS->Rliv->get_live_out(previous).find(var) != HLS->Rliv->get_live_out(previous).end(), "The variable is not in live-out");
               origin_idx = HLS->storage_value_information->get_storage_value_index(previous, var);
               target_idx = HLS->storage_value_information->get_storage_value_index(target, var);
               origin_reg_idx = HLS->Rreg->get_register(origin_idx);
               target_reg_idx = HLS->Rreg->get_register(target_idx);
               origin_reg = HLS->Rreg->get(origin_reg_idx);
               target_reg = HLS->Rreg->get(target_reg_idx);
               // Always add a data transfer on port 0 since it's always an input to reg_STD
               HLS->Rconn->add_data_transfer(origin_reg, target_reg, 0, 0, data_transfer(var, precision, previous, target, op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from " << origin_reg->get_string() << " to " << target_reg->get_string() << " port " << std::to_string(0) << ":" << std::to_string(0) << " from state "
                                                                << HLS->Rliv->get_name(previous) + " to state " + HLS->Rliv->get_name(target) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(tree_var));
               target = previous;
               previous = fetch_previous(HLS->STG->get_entry_state(), target);
            }
         }
      }
   }
}

void mux_connection_binding::add_conversion(unsigned int num, unsigned int size_tree_var, VertexIterator op, unsigned int form_par_type, unsigned int port_index, const generic_objRef fu_obj, const OpGraphConstRef data, const tree_managerRef TreeM,
                                            unsigned int tree_var, std::vector<HLS_manager::io_binding_type>& var_read, unsigned int size_form_par)
{
   if(tree_helper::is_int(TreeM, tree_var) && (tree_helper::is_unsigned(TreeM, form_par_type) or tree_helper::is_bool(TreeM, form_par_type)))
   {
      auto varObj = var_read[num];
      if(isZeroObj(std::get<0>(varObj), TreeM))
         varObj = HLS_manager::io_binding_type(0, 0);
      generic_objRef conv_port;
      unsigned int in_bitsize = size_form_par;
      auto key = std::make_tuple(in_bitsize, iu_conv, varObj);
      if(connCache.find(key) == connCache.end())
      {
         conv_port = generic_objRef(new iu_conv_conn_obj("iu_conv_conn_obj_" + STR(id++)));
         if(isConstantObj(std::get<0>(varObj), TreeM))
            connCache[key] = conv_port;
         HLS->Rconn->add_sparse_logic(conv_port);
         GetPointer<iu_conv_conn_obj>(conv_port)->add_bitsize(in_bitsize);
         determine_connection(*op, varObj, conv_port, 0, 0, data, size_tree_var);
      }
      else
         conv_port = connCache.find(key)->second;
      create_single_conn(data, *op, conv_port, fu_obj, num, port_index, tree_var, in_bitsize, true);
   }
   else if((tree_helper::is_unsigned(TreeM, tree_var) or tree_helper::is_bool(TreeM, tree_var)) && tree_helper::is_int(TreeM, form_par_type))
   {
      auto varObj = var_read[num];
      if(isZeroObj(std::get<0>(varObj), TreeM))
         varObj = HLS_manager::io_binding_type(0, 0);
      generic_objRef conv_port;
      unsigned int in_bitsize = size_form_par;
      auto key = std::make_tuple(in_bitsize, ui_conv, varObj);
      if(connCache.find(key) == connCache.end())
      {
         conv_port = generic_objRef(new ui_conv_conn_obj("ui_conv_conn_obj_" + STR(id++)));
         if(isConstantObj(std::get<0>(varObj), TreeM))
            connCache[key] = conv_port;
         HLS->Rconn->add_sparse_logic(conv_port);
         GetPointer<ui_conv_conn_obj>(conv_port)->add_bitsize(in_bitsize);
         determine_connection(*op, varObj, conv_port, 0, 0, data, size_tree_var);
      }
      else
         conv_port = connCache.find(key)->second;
      create_single_conn(data, *op, conv_port, fu_obj, num, port_index, tree_var, in_bitsize, true);
   }
   else if(size_form_par != size_tree_var)
   {
      generic_objRef conv_port = generic_objRef(new ff_conv_conn_obj("ff_conv_conn_obj_" + STR(id++)));
      if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
      {
         technology_nodeRef current_fu;
         AllocationInformation::extract_bambu_provided_name(size_tree_var, size_form_par, HLSMgr, current_fu);
      }
      HLS->Rconn->add_sparse_logic(conv_port);
      unsigned int in_bitsize = size_form_par;
      GetPointer<ff_conv_conn_obj>(conv_port)->add_bitsize_in(size_tree_var);
      GetPointer<ff_conv_conn_obj>(conv_port)->add_bitsize_out(in_bitsize);
      determine_connection(*op, var_read[num], conv_port, 0, 0, data, size_tree_var);
      create_single_conn(data, *op, conv_port, fu_obj, num, port_index, tree_var, in_bitsize, true);
   }
   else
   {
      determine_connection(*op, var_read[num], fu_obj, num, port_index, data, object_bitsize(TreeM, var_read[num]));
   }
}

void mux_connection_binding::create_connections()
{
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const BehavioralHelperConstRef behavioral_helper = FB->CGetBehavioralHelper();
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::FDFG);
   unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();
   if(parameters->getOption<int>(OPT_memory_banks_number) > 1 && !parameters->isOption(OPT_context_switch))
   {
      HLS->Rconn = conn_bindingRef(new ParallelMemoryConnBinding(behavioral_helper, parameters));
   }
   else
   {
      HLS->Rconn = conn_bindingRef(conn_binding::create_conn_binding(HLSMgr, HLS, behavioral_helper, parameters));
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Starting execution of interconnection binding");

   unsigned int num_regs = HLS->Rreg->get_used_regs();
   for(unsigned int r = 0; r < num_regs; r++)
   {
      generic_objRef reg_obj = HLS->Rreg->get(r);
      generic_objRef sel_port = HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::WRENABLE, reg_obj, r);
      GetPointer<register_obj>(reg_obj)->set_wr_enable(sel_port);
   }
   for(unsigned int i : HLS->Rfu->get_allocation_list())
   {
      // number of instance functional unit i
      unsigned int num = HLS->Rfu->get_number(i);
      for(unsigned int fu_num = 0; fu_num < num; fu_num++)
      {
         // get the functional unit object associated to i and fu_num (id and index)
         generic_objRef tmp_Fu = HLS->Rfu->get(i, fu_num);
         std::vector<technology_nodeRef> tmp_ops_node = GetPointer<functional_unit>(HLS->allocation_information->get_fu(i))->get_operations();

         if(tmp_ops_node.size() > 1)
         {
            // check all operations associated to functional unit tmp_Fu
            for(unsigned int oper = 0; oper < tmp_ops_node.size(); oper++)
            {
               generic_objRef sel_port = HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::SELECTOR, tmp_Fu, oper);
               GetPointer<funit_obj>(tmp_Fu)->add_selector_op(sel_port, tmp_ops_node[oper]->get_name());
            }
         }
      }
   }

   /// add the ports representing the parameters
   add_parameter_ports();

   VertexIterator op, opend;
   for(boost::tie(op, opend) = boost::vertices(*data); op != opend; op++)
   {
      /// check for required and produced values
      if(GET_TYPE(data, *op) & TYPE_VPHI)
         continue; /// virtual phis are skipped
      unsigned int fu = HLS->Rfu->get_assign(*op);
      unsigned int idx = HLS->Rfu->get_index(*op);
      unsigned int n_channels = HLS->allocation_information->get_number_channels(fu);
      if((GET_TYPE(data, *op) & TYPE_PHI) == 0) /// phis are skipped
      {
         unsigned int port_index = n_channels < 2 ? 0 : idx % n_channels;
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  * Operation: " + GET_NAME(data, *op) << " " + data->CGetOpNodeInfo(*op)->GetOperation());
         HLS->Rconn->bind_command_port(*op, conn_binding::IN, commandport_obj::OPERATION, data);

         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - FU: " + HLS->allocation_information->get_fu_name(fu).first);
         /// adding activation's state of selector related to operation op
         std::vector<technology_nodeRef> tmp_ops_node = GetPointer<functional_unit>(HLS->allocation_information->get_fu(fu))->get_operations();
         if(tmp_ops_node.size() > 1)
         {
            if(!GetPointer<funit_obj>(HLS->Rfu->get(fu, idx)))
               THROW_ERROR("Functional unit " + HLS->allocation_information->get_string_name(fu) + " does not have an instance " + STR(idx));
            generic_objRef selector_obj = GetPointer<funit_obj>(HLS->Rfu->get(fu, idx))->GetSelector_op(data->CGetOpNodeInfo(*op)->GetOperation());
            if(!selector_obj)
               THROW_ERROR("Functional unit " + HLS->allocation_information->get_string_name(fu) + " does not exist or it does not have selector " + data->CGetOpNodeInfo(*op)->GetOperation() + "(" + STR(idx) +
                           ") Operation: " + STR(data->CGetOpNodeInfo(*op)->GetNodeId()));
            const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(*op);
            const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
            for(auto rs_it = running_states.begin(); rs_it_end != rs_it; ++rs_it)
            {
               GetPointer<commandport_obj>(selector_obj)->add_activation(commandport_obj::transition(*rs_it, NULL_VERTEX, commandport_obj::data_operation_pair(0, *op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - add activation for " + selector_obj->get_string() + " in state " << HLS->Rliv->get_name(*rs_it));
            }
         }

         const generic_objRef fu_obj = HLS->Rfu->get(*op);
         std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, *op);
         for(unsigned int num = 0; num < var_read.size(); num++)
         {
            if(std::get<0>(var_read[num]) == 0)
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << num << ". Read: " + STR(std::get<1>(var_read[num])));
            else
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << num << ". Read: " + behavioral_helper->PrintVariable(std::get<0>(var_read[num])));
         }
         if(GET_TYPE(data, *op) & (TYPE_LOAD | TYPE_STORE))
         {
            unsigned int node_id = data->CGetOpNodeInfo(*op)->GetNodeId();
            const tree_nodeRef node = TreeM->get_tree_node_const(node_id);
            auto* gm = GetPointer<gimple_assign>(node);
            THROW_ASSERT(gm, "only gimple_assign's are allowed as memory operations");

            if(HLS->allocation_information->is_direct_access_memory_unit(fu) || HLS->allocation_information->is_indirect_access_memory_unit(fu)) /// MEMORY REFERENCES
            {
               unsigned int alignment = 0;
               tree_nodeRef var_node;
               unsigned int size_var;
               tree_nodeRef tn;
               unsigned tn_index;
               unsigned int var_node_idx;
               unsigned int return_index;
               unsigned int prec = 0;
               const tree_nodeRef type = tree_helper::get_type_node(GET_NODE(gm->op0), return_index);
               if(type && (type->get_kind() == integer_type_K))
                  prec = GetPointer<integer_type>(type)->prec;
               unsigned int algn = 0;
               if(type && (type->get_kind() == integer_type_K))
                  algn = GetPointer<integer_type>(type)->algn;
#if USE_ALIGNMENT_INFO
               if(type && GetPointer<type_node>(type))
                  alignment = GetPointer<type_node>(type)->algn;
#endif
               if(GET_TYPE(data, *op) & TYPE_STORE)
               {
                  size_var = std::get<0>(var_read[0]);
                  tn = TreeM->get_tree_node_const(size_var);
                  tn = tree_helper::get_type_node(tn, tn_index);
                  var_node = GET_NODE(gm->op0);
                  var_node_idx = GET_INDEX_NODE(gm->op0);

                  if(size_var)
                  {
                     const integer_cst* obj_size = GetPointer<integer_cst>(GET_NODE(GetPointer<type_node>(tn)->size));
                     THROW_ASSERT(obj_size, "size is not an integer_cst");
                     long long int IR_var_bitsize = tree_helper::get_integer_cst_value(obj_size);
                     unsigned int var_bitsize;
                     if(prec != algn && prec % algn)
                        var_bitsize = prec;
                     else
                        var_bitsize = static_cast<unsigned int>(IR_var_bitsize);
                     generic_objRef conv_port;
                     auto varObj = var_read[0];
                     if(isZeroObj(std::get<0>(varObj), TreeM))
                        varObj = HLS_manager::io_binding_type(0, 0);
                     if(tree_helper::is_int(TreeM, size_var))
                     {
                        auto key = std::make_tuple(var_bitsize, iu_conv, varObj);
                        if(connCache.find(key) == connCache.end())
                        {
                           conv_port = generic_objRef(new iu_conv_conn_obj("iu_conv_conn_obj_" + STR(id++)));
                           if(isConstantObj(std::get<0>(varObj), TreeM))
                              connCache[key] = conv_port;
                           HLS->Rconn->add_sparse_logic(conv_port);
                           GetPointer<iu_conv_conn_obj>(conv_port)->add_bitsize(var_bitsize);
                           determine_connection(*op, varObj, conv_port, 0, 0, data, var_bitsize);
                        }
                        else
                           conv_port = connCache.find(key)->second;
                     }
                     else
                     {
                        auto key = std::make_tuple(var_bitsize, vb_assign, varObj);
                        if(connCache.find(key) == connCache.end())
                        {
                           conv_port = generic_objRef(new vb_assign_conn_obj("vb_assign_conn_obj_" + STR(id++)));
                           if(isConstantObj(std::get<0>(varObj), TreeM))
                              connCache[key] = conv_port;
                           HLS->Rconn->add_sparse_logic(conv_port);
                           GetPointer<vb_assign_conn_obj>(conv_port)->add_bitsize(var_bitsize);
                           determine_connection(*op, varObj, conv_port, 0, 0, data, var_bitsize);
                        }
                        else
                           conv_port = connCache.find(key)->second;
                     }
                     create_single_conn(data, *op, conv_port, fu_obj, 0, port_index, size_var, var_bitsize, true);
                  }
                  else
                     determine_connection(*op, var_read[0], fu_obj, 0, port_index, data, object_bitsize(TreeM, var_read[0]));
               }
               else
               {
                  size_var = HLSMgr->get_produced_value(HLS->functionId, *op);
                  tn = TreeM->get_tree_node_const(size_var);
                  tn = tree_helper::get_type_node(tn, tn_index);
                  var_node = GET_NODE(gm->op1);
                  var_node_idx = GET_INDEX_NODE(gm->op1);
               }
#ifndef NDEBUG
               if(var_node->get_kind() == ssa_name_K)
               {
                  unsigned int temp_type_index;
                  THROW_ASSERT(tree_helper::get_type_node(var_node, temp_type_index)->get_kind() == complex_type_K, "only complex objects are considered");
               }
#endif
               if(gm->predicate)
               {
                  THROW_ASSERT(tree_helper::size(TreeM, gm->predicate->index) == 1, STR(gm->predicate));
                  determine_connection(*op, HLS_manager::io_binding_type(gm->predicate->index, 0), fu_obj, 3, port_index, data, 1);
               }
               else
               {
                  determine_connection(*op, HLS_manager::io_binding_type(0, 1), fu_obj, 3, port_index, data, 1);
               }
               switch(var_node->get_kind())
               {
                  case parm_decl_K:
                  case var_decl_K:
                  case ssa_name_K:
                  case misaligned_indirect_ref_K:
                  case indirect_ref_K:
                  case mem_ref_K:
                  case array_ref_K:
                  case component_ref_K:
                  case target_mem_ref_K:
                  case target_mem_ref461_K:
                  case imagpart_expr_K:
                  case realpart_expr_K:
                  case view_convert_expr_K:
                  case bit_field_ref_K:
                  {
                     determine_connection(*op, HLS_manager::io_binding_type(var_node_idx, 0), fu_obj, 1, port_index, data, bus_addr_bitsize, alignment);
                     if(prec != algn && prec % algn) /// bitfield management
                        determine_connection(*op, HLS_manager::io_binding_type(0, prec), fu_obj, 2, port_index, data, object_bitsize(TreeM, HLS_manager::io_binding_type(0, prec)));
                     else
                     {
                        const integer_cst* obj_size = GetPointer<integer_cst>(GET_NODE(GetPointer<type_node>(tn)->size));
                        THROW_ASSERT(obj_size, "size is not an integer_cst");
                        long long int IR_var_bitsize = tree_helper::get_integer_cst_value(obj_size);
                        unsigned int var_bitsize;
                        if(prec != algn && prec % algn)
                           var_bitsize = prec;
                        else
                           var_bitsize = static_cast<unsigned int>(IR_var_bitsize);
                        determine_connection(*op, HLS_manager::io_binding_type(GET_INDEX_NODE(GetPointer<type_node>(tn)->size), 0), fu_obj, 2, port_index, data, object_bitsize(TreeM, HLS_manager::io_binding_type(0, var_bitsize)));
                     }

                     break;
                  }
                  case assert_expr_K:
                  case bit_and_expr_K:
                  case bit_ior_expr_K:
                  case bit_xor_expr_K:
                  case catch_expr_K:
                  case ceil_div_expr_K:
                  case ceil_mod_expr_K:
                  case complex_expr_K:
                  case compound_expr_K:
                  case eh_filter_expr_K:
                  case eq_expr_K:
                  case exact_div_expr_K:
                  case fdesc_expr_K:
                  case floor_div_expr_K:
                  case floor_mod_expr_K:
                  case ge_expr_K:
                  case gt_expr_K:
                  case goto_subroutine_K:
                  case in_expr_K:
                  case init_expr_K:
                  case le_expr_K:
                  case lrotate_expr_K:
                  case lshift_expr_K:
                  case lt_expr_K:
                  case max_expr_K:
                  case min_expr_K:
                  case minus_expr_K:
                  case modify_expr_K:
                  case mult_expr_K:
                  case mult_highpart_expr_K:
                  case ne_expr_K:
                  case ordered_expr_K:
                  case paren_expr_K:
                  case plus_expr_K:
                  case pointer_plus_expr_K:
                  case postdecrement_expr_K:
                  case postincrement_expr_K:
                  case predecrement_expr_K:
                  case preincrement_expr_K:
                  case range_expr_K:
                  case rdiv_expr_K:
                  case round_div_expr_K:
                  case round_mod_expr_K:
                  case rrotate_expr_K:
                  case rshift_expr_K:
                  case set_le_expr_K:
                  case trunc_div_expr_K:
                  case trunc_mod_expr_K:
                  case truth_and_expr_K:
                  case truth_andif_expr_K:
                  case truth_or_expr_K:
                  case truth_orif_expr_K:
                  case truth_xor_expr_K:
                  case try_catch_expr_K:
                  case try_finally_K:
                  case uneq_expr_K:
                  case ltgt_expr_K:
                  case unge_expr_K:
                  case ungt_expr_K:
                  case unle_expr_K:
                  case unlt_expr_K:
                  case unordered_expr_K:
                  case widen_sum_expr_K:
                  case widen_mult_expr_K:
                  case with_size_expr_K:
                  case vec_lshift_expr_K:
                  case vec_rshift_expr_K:
                  case widen_mult_hi_expr_K:
                  case widen_mult_lo_expr_K:
                  case vec_cond_expr_K:
                  case vec_pack_trunc_expr_K:
                  case vec_pack_sat_expr_K:
                  case vec_pack_fix_trunc_expr_K:
                  case vec_perm_expr_K:
                  case vec_extracteven_expr_K:
                  case vec_extractodd_expr_K:
                  case vec_interleavehigh_expr_K:
                  case vec_interleavelow_expr_K:
                  case binfo_K:
                  case block_K:
                  case call_expr_K:
                  case aggr_init_expr_K:
                  case case_label_expr_K:
                  case constructor_K:
                  case identifier_node_K:
                  case statement_list_K:
                  case tree_list_K:
                  case tree_vec_K:
                  case CASE_CPP_NODES:
                  case CASE_CST_NODES:
                  case CASE_FAKE_NODES:
                  case CASE_GIMPLE_NODES:
                  case CASE_PRAGMA_NODES:
                  case CASE_TYPE_NODES:
                  case const_decl_K:
                  case field_decl_K:
                  case function_decl_K:
                  case label_decl_K:
                  case namespace_decl_K:
                  case result_decl_K:
                  case translation_unit_decl_K:
                  case error_mark_K:
                  case using_decl_K:
                  case type_decl_K:
                  case template_decl_K:
                  case array_range_ref_K:
                  case target_expr_K:
                  case vtable_ref_K:
                  case with_cleanup_expr_K:
                  case obj_type_ref_K:
                  case save_expr_K:
                  case cond_expr_K:
                  case dot_prod_expr_K:
                  case ternary_plus_expr_K:
                  case ternary_pm_expr_K:
                  case ternary_mp_expr_K:
                  case ternary_mm_expr_K:
                  case bit_ior_concat_expr_K:
                  case abs_expr_K:
                  case addr_expr_K:
                  case arrow_expr_K:
                  case bit_not_expr_K:
                  case buffer_ref_K:
                  case card_expr_K:
                  case cleanup_point_expr_K:
                  case conj_expr_K:
                  case convert_expr_K:
                  case exit_expr_K:
                  case fix_ceil_expr_K:
                  case fix_floor_expr_K:
                  case fix_round_expr_K:
                  case fix_trunc_expr_K:
                  case float_expr_K:
                  case loop_expr_K:
                  case lut_expr_K:
                  case negate_expr_K:
                  case non_lvalue_expr_K:
                  case nop_expr_K:
                  case reference_expr_K:
                  case reinterpret_cast_expr_K:
                  case sizeof_expr_K:
                  case static_cast_expr_K:
                  case throw_expr_K:
                  case truth_not_expr_K:
                  case unsave_expr_K:
                  case va_arg_expr_K:
                  case reduc_max_expr_K:
                  case reduc_min_expr_K:
                  case reduc_plus_expr_K:
                  case vec_unpack_hi_expr_K:
                  case vec_unpack_lo_expr_K:
                  case vec_unpack_float_hi_expr_K:
                  case vec_unpack_float_lo_expr_K:
                  case extract_bit_expr_K:
                  default:
                     THROW_ERROR("MEMORY REFERENCE/LOAD-STORE type not supported: " + var_node->get_kind_text() + " " + STR(node_id));
               }
            }
            else
               THROW_ERROR("Unit " + HLS->allocation_information->get_fu_name(fu).first + " not supported");
         }
         else if(data->CGetOpNodeInfo(*op)->GetOperation() == MEMCPY)
         {
            unsigned int node_id = data->CGetOpNodeInfo(*op)->GetNodeId();
            const tree_nodeRef node = TreeM->get_tree_node_const(node_id);
            switch(node->get_kind())
            {
               case gimple_call_K:
               {
                  for(unsigned int num = 0; num < var_read.size(); num++)
                  {
                     determine_connection(*op, var_read[num], fu_obj, num, 0, data, object_bitsize(TreeM, var_read[num]));
                  }
                  break;
               }
               case gimple_assign_K:
               {
                  auto* gm = GetPointer<gimple_assign>(node);
                  if(GET_NODE(gm->op1)->get_kind() == call_expr_K || GET_NODE(gm->op1)->get_kind() == aggr_init_expr_K)
                  {
                     auto* ce = GetPointer<call_expr>(GET_NODE(gm->op1));
                     tree_nodeRef cefn = GET_NODE(ce->fn);
                     THROW_ASSERT(cefn && cefn->get_kind() == addr_expr_K, "expected a function");
#ifndef NDEBUG
                     auto* fd = GetPointer<function_decl>(GET_NODE(GetPointer<addr_expr>(cefn)->op));
                     THROW_ASSERT(fd && (tree_helper::print_function_name(TreeM, fd) == "memcpy" || tree_helper::print_function_name(TreeM, fd) == "__internal_bambu_memcpy"), "expected a memcpy call");
#endif
                     const std::vector<tree_nodeRef>& args = ce->args;
                     unsigned int num = 0;
                     auto arg_end = args.end();
                     for(auto arg = args.begin(); arg != arg_end; ++arg, ++num)
                     {
                        determine_connection(*op, HLS_manager::io_binding_type(GET_INDEX_NODE(*arg), 0), fu_obj, num, 0, data, bus_addr_bitsize);
                     }
                  }
                  else
                  {
                     determine_connection(*op, HLS_manager::io_binding_type(GET_INDEX_NODE(gm->op0), 0), fu_obj, 0, 0, data, bus_addr_bitsize);
                     determine_connection(*op, HLS_manager::io_binding_type(GET_INDEX_NODE(gm->op1), 0), fu_obj, 1, 0, data, bus_addr_bitsize);
                     unsigned int type_index = tree_helper::get_type_index(TreeM, GET_INDEX_NODE(gm->op0));
                     determine_connection(*op, HLS_manager::io_binding_type(0, tree_helper::size(TreeM, type_index) / 8), fu_obj, 2, 0, data, bus_addr_bitsize);
                  }
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case gimple_asm_K:
               case gimple_bind_K:
               case gimple_cond_K:
               case gimple_for_K:
               case gimple_goto_K:
               case gimple_label_K:
               case gimple_multi_way_if_K:
               case gimple_nop_K:
               case gimple_phi_K:
               case gimple_pragma_K:
               case gimple_predict_K:
               case gimple_resx_K:
               case gimple_return_K:
               case gimple_switch_K:
               case gimple_while_K:
               case error_mark_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case CASE_UNARY_EXPRESSION:
               {
                  THROW_UNREACHABLE("MEMCPY type not supported: " + node->get_kind_text() + " - @" + STR(node_id));
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         else if(data->CGetOpNodeInfo(*op)->GetOperation() == MEMSET)
         {
            unsigned int node_id = data->CGetOpNodeInfo(*op)->GetNodeId();
            const tree_nodeRef node = TreeM->get_tree_node_const(node_id);
            switch(node->get_kind())
            {
               case gimple_call_K:
               {
                  for(unsigned int num = 0; num < var_read.size(); num++)
                  {
                     determine_connection(*op, var_read[num], fu_obj, num, 0, data, object_bitsize(TreeM, var_read[num]));
                  }
                  break;
               }
               case gimple_assign_K:
               {
                  auto* gm = GetPointer<gimple_assign>(node);
                  if(GET_NODE(gm->op1)->get_kind() == call_expr_K || GET_NODE(gm->op1)->get_kind() == aggr_init_expr_K)
                  {
                     auto* ce = GetPointer<call_expr>(GET_NODE(gm->op1));
                     tree_nodeRef cefn = GET_NODE(ce->fn);
                     THROW_ASSERT(cefn && cefn->get_kind() == addr_expr_K, "expected a function");
#ifndef NDEBUG
                     auto* fd = GetPointer<function_decl>(GET_NODE(GetPointer<addr_expr>(cefn)->op));
                     THROW_ASSERT(fd && (tree_helper::print_function_name(TreeM, fd) == "memset" || tree_helper::print_function_name(TreeM, fd) == "__internal_bambu_memset"), "expected a memcpy call");
#endif
                     const std::vector<tree_nodeRef>& args = ce->args;
                     unsigned int num = 0;
                     auto arg_end = args.end();
                     for(auto arg = args.begin(); arg != arg_end; ++arg, ++num)
                     {
                        determine_connection(*op, HLS_manager::io_binding_type(GET_INDEX_NODE(*arg), 0), fu_obj, num, 0, data, object_bitsize(TreeM, HLS_manager::io_binding_type(GET_INDEX_NODE(*arg), 0)));
                     }
                  }
                  else
                  {
                     THROW_ASSERT(GET_NODE(gm->op1)->get_kind() == constructor_K && GetPointer<constructor>(GET_NODE(gm->op1)) && GetPointer<constructor>(GET_NODE(gm->op1))->list_of_idx_valu.size() == 0, "pattern not supported");
                     determine_connection(*op, HLS_manager::io_binding_type(GET_INDEX_NODE(gm->op0), 0), fu_obj, 0, 0, data, bus_addr_bitsize);
                     determine_connection(*op, HLS_manager::io_binding_type(0, 0), fu_obj, 1, 0, data, 1);
                     unsigned int type_index = tree_helper::get_type_index(TreeM, GET_INDEX_NODE(gm->op0));
                     determine_connection(*op, HLS_manager::io_binding_type(0, tree_helper::size(TreeM, type_index) / 8), fu_obj, 2, 0, data, bus_addr_bitsize);
                  }
                  break;
               }
               case binfo_K:
               case block_K:
               case call_expr_K:
               case aggr_init_expr_K:
               case case_label_expr_K:
               case constructor_K:
               case identifier_node_K:
               case ssa_name_K:
               case statement_list_K:
               case target_expr_K:
               case target_mem_ref_K:
               case target_mem_ref461_K:
               case tree_list_K:
               case tree_vec_K:
               case gimple_asm_K:
               case gimple_bind_K:
               case gimple_cond_K:
               case gimple_for_K:
               case gimple_goto_K:
               case gimple_label_K:
               case gimple_multi_way_if_K:
               case gimple_nop_K:
               case gimple_phi_K:
               case gimple_pragma_K:
               case gimple_predict_K:
               case gimple_resx_K:
               case gimple_return_K:
               case gimple_switch_K:
               case gimple_while_K:
               case error_mark_K:
               case lut_expr_K:
               case CASE_BINARY_EXPRESSION:
               case CASE_CPP_NODES:
               case CASE_CST_NODES:
               case CASE_DECL_NODES:
               case CASE_FAKE_NODES:
               case CASE_PRAGMA_NODES:
               case CASE_QUATERNARY_EXPRESSION:
               case CASE_TERNARY_EXPRESSION:
               case CASE_TYPE_NODES:
               case CASE_UNARY_EXPRESSION:
               {
                  THROW_UNREACHABLE("MEMCPY type not supported: " + node->get_kind_text() + " - @" + STR(node_id));
                  break;
               }
               default:
               {
                  THROW_UNREACHABLE("");
               }
            }
         }
         else if(data->CGetOpNodeInfo(*op)->GetOperation() == MULTI_READ_COND)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << var_read.size() << " reads");
            for(unsigned int num = 0; num < var_read.size(); num++)
            {
               determine_connection(*op, var_read[num], fu_obj, 0, num, data, object_bitsize(TreeM, var_read[num]));
            }
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << var_read.size() << " reads");
            unsigned first_valid_id = 0;
            if(HLS->Rfu->get_ports_are_swapped(*op))
            {
               THROW_ASSERT(var_read.size() == 2, "unexpected condition");
               std::swap(var_read[0], var_read[1]);
            }
            for(unsigned int num = 0; num < var_read.size(); num++)
            {
               unsigned int port_num = num;
               unsigned int tree_var = std::get<0>(var_read[port_num]);
               unsigned int node_id = data->CGetOpNodeInfo(*op)->GetNodeId();
               unsigned int form_par_type = tree_helper::get_formal_ith(TreeM, node_id, port_num);
               unsigned int size_tree_var = tree_var == 0 ? 0 : tree_helper::size(TreeM, tree_var);
               unsigned int size_form_par = form_par_type == 0 ? 0 : tree_helper::size(TreeM, form_par_type);
               auto OperationType = data->CGetOpNodeInfo(*op)->GetOperation();
               if(tree_var && !first_valid_id)
                  first_valid_id = tree_var;
               if((OperationType == "cond_expr" || OperationType == "vec_cond_expr") && port_num != 0 && tree_var)
                  first_valid_id = tree_var;

               if(tree_var == 0)
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << port_num << ". Read: " + STR(std::get<1>(var_read[port_num])));
               }
               else
               {
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - " << port_num << ". Read: " + behavioral_helper->PrintVariable(tree_var));
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "          * " + TreeM->get_tree_node_const(std::get<0>(var_read[port_num]))->get_kind_text());
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "          * bitsize " + STR(object_bitsize(TreeM, var_read[port_num])));
               }
               if(tree_var && HLSMgr->Rmem->is_actual_parm_loaded(tree_var))
                  THROW_ERROR("LOADING of actual parameter not yet implemented");
               else if(form_par_type && tree_var &&
                       ((tree_helper::is_int(TreeM, tree_var) && (tree_helper::is_unsigned(TreeM, form_par_type) or tree_helper::is_bool(TreeM, form_par_type))) ||
                        ((tree_helper::is_unsigned(TreeM, tree_var) or (tree_helper::is_bool(TreeM, form_par_type))) && tree_helper::is_int(TreeM, form_par_type)) || (tree_helper::is_real(TreeM, tree_var) && tree_helper::is_real(TreeM, form_par_type))))
               {
                  add_conversion(port_num, size_tree_var, op, form_par_type, port_index, fu_obj, data, TreeM, tree_var, var_read, size_form_par);
               }
               else if(first_valid_id && tree_var && first_valid_id != tree_var && form_par_type == 0 && OperationType != "rshift_expr" && OperationType != "lshift_expr" && OperationType != "extract_bit_expr" && OperationType != "rrotate_expr" &&
                       OperationType != "lrotate_expr" && ((tree_helper::is_int(TreeM, tree_var) && tree_helper::is_unsigned(TreeM, first_valid_id)) || (tree_helper::is_unsigned(TreeM, tree_var) && tree_helper::is_int(TreeM, first_valid_id))))
               {
                  size_form_par = tree_helper::size(TreeM, tree_var); // we only need type conversion and not size conversion
                  add_conversion(port_num, size_tree_var, op, first_valid_id, port_index, fu_obj, data, TreeM, tree_var, var_read, size_form_par);
               }
               else
               {
                  determine_connection(*op, var_read[port_num], fu_obj, port_num, port_index, data, object_bitsize(TreeM, var_read[port_num]));
               }
            }
         }
      }

      if(GET_TYPE(data, *op) & TYPE_PHI)
      {
         /// phi must be differently managed
         unsigned int var_written = HLSMgr->get_produced_value(HLS->functionId, *op);
         CustomOrderedSet<unsigned int> source_already_analyzed;
         const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(*op);
         THROW_ASSERT(ending_states.size() == 1 || is_PC || HLS->STG->GetStg()->CGetStateInfo(*ending_states.begin())->is_duplicated, "phis cannot run in more than one state");
         const CustomOrderedSet<vertex>::const_iterator e_it_end = ending_states.end();
         for(auto e_it = ending_states.begin(); e_it_end != e_it; ++e_it)
         {
            const StateInfoConstRef state_info = is_PC ? StateInfoConstRef() : HLS->STG->GetStg()->CGetStateInfo(*e_it);
            const auto gp = GetPointer<const gimple_phi>(TreeM->get_tree_node_const(data->CGetOpNodeInfo(*op)->GetNodeId()));
            for(const auto& def_edge : gp->CGetDefEdgesList())
            {
               unsigned int tree_temp = def_edge.first->index;
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Pre-Managing phi operation " + GET_NAME(data, *op) + " ending in state " + HLS->Rliv->get_name(*e_it) + (tree_temp ? " for variable " + def_edge.first->ToString() : ""));
               bool phi_postponed = false;
               if(state_info && state_info->is_duplicated && !state_info->all_paths)
               {
                  unsigned int bbID = def_edge.second;
                  if(!state_info->isOriginalState && bbID != state_info->sourceBb)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Not original state and not source BB");
                     continue;
                  }
                  else if(state_info->isOriginalState && bbID == state_info->sourceBb)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Original state and source BB");
                     continue;
                  }
                  else if(state_info->moved_op_def_set.find(tree_temp) != state_info->moved_op_def_set.end())
                     phi_postponed = true;
                  else if(state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end())
                     phi_postponed = true;
               }
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Is phi postponed? " + (phi_postponed ? std::string("YES") : std::string("NO")));
               if(!phi_postponed)
               {
                  if(source_already_analyzed.find(tree_temp) == source_already_analyzed.end())
                     source_already_analyzed.insert(tree_temp);
                  else
                     continue;
               }
               cur_phi_tree_var = tree_temp;
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Pre-Managing phi operation2 " + GET_NAME(data, *op) + " ending in state " + HLS->Rliv->get_name(*e_it) + (cur_phi_tree_var ? " for variable " + behavioral_helper->PrintVariable(cur_phi_tree_var) : ""));
               THROW_ASSERT(cur_phi_tree_var, "something of wrong happen");
               THROW_ASSERT(!HLSMgr->Rmem->has_base_address(tree_temp), "phi cannot manage memory objects: @" + STR(tree_temp));
               THROW_ASSERT(!HLSMgr->Rmem->has_base_address(var_written), "phi cannot manage memory objects: @" + STR(var_written));
               THROW_ASSERT(TreeM->get_tree_node_const(tree_temp)->get_kind() != array_ref_K, "unexpected phi use");
               THROW_ASSERT(TreeM->get_tree_node_const(tree_temp)->get_kind() != indirect_ref_K, "unexpected phi use");
               THROW_ASSERT(TreeM->get_tree_node_const(tree_temp)->get_kind() != misaligned_indirect_ref_K, "unexpected phi use");

               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "Managing phi operation " + GET_NAME(data, *op) + " ending in state " + HLS->Rliv->get_name(*e_it) + (cur_phi_tree_var ? " for variable " + behavioral_helper->PrintVariable(cur_phi_tree_var) : ""));
               if(HLS->storage_value_information->is_a_storage_value(*e_it, var_written))
               {
                  unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(*e_it, var_written);
                  unsigned int r_index = HLS->Rreg->get_register(storage_value);
                  unsigned int in_bitsize = object_bitsize(TreeM, HLS_manager::io_binding_type(tree_temp, 0));
                  unsigned int out_bitsize = object_bitsize(TreeM, HLS_manager::io_binding_type(var_written, 0));
                  generic_objRef tgt_reg_obj = HLS->Rreg->get(r_index);
                  if(in_bitsize != out_bitsize)
                  {
                     if(tree_helper::is_unsigned(TreeM, var_written) || tree_helper::is_a_pointer(TreeM, var_written) || tree_helper::is_bool(TreeM, var_written))
                     {
                        generic_objRef conv_port = generic_objRef(new u_assign_conn_obj("u_assign_conn_obj_" + STR(id++)));
                        HLS->Rconn->add_sparse_logic(conv_port);
                        GetPointer<u_assign_conn_obj>(conv_port)->add_bitsize(in_bitsize);
                        if(phi_postponed)
                        {
                           if(HLS->Rliv->has_state_out(*e_it, *op, var_written))
                           {
                              generic_objRef fu_src_obj;
                              if(state_info && state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() && state_info->moved_op_def_set.find(cur_phi_tree_var) == state_info->moved_op_def_set.end())
                              {
                                 unsigned int src_storage_value = HLS->storage_value_information->get_storage_value_index(*e_it, cur_phi_tree_var);
                                 unsigned int src_r_index = HLS->Rreg->get_register(src_storage_value);
                                 fu_src_obj = HLS->Rreg->get(src_r_index);
                              }
                              else
                              {
                                 vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                                 fu_src_obj = HLS->Rfu->get(src_def_op);
                              }
                              const CustomOrderedSet<vertex>& states_out = HLS->Rliv->get_state_out(*e_it, *op, var_written);
                              const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                              for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                              {
                                 HLS->Rconn->add_data_transfer(fu_src_obj, conv_port, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                                 HLS->Rconn->add_data_transfer(conv_port, tgt_reg_obj, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - add data transfer from " << fu_src_obj->get_string() << " to " << conv_port->get_string() << " port 0:0 from state "
                                                                                  << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it) + " for " +
                                                                                         HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(cur_phi_tree_var));
                                 generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                                 GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*e_it, *s_out_it, commandport_obj::data_operation_pair(cur_phi_tree_var, *op)));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + tgt_reg_obj->get_string() + " from " << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it));
                              }
                           }
                        }
                        else
                        {
                           determine_connection(*op, HLS_manager::io_binding_type(tree_temp, 0), conv_port, 0, 0, data, in_bitsize);
                           create_single_conn(data, *op, conv_port, tgt_reg_obj, 0, 0, cur_phi_tree_var, in_bitsize, false);
                        }
                     }
                     else if(tree_helper::is_int(TreeM, var_written))
                     {
                        if(phi_postponed)
                        {
                           if(HLS->Rliv->has_state_out(*e_it, *op, var_written))
                           {
                              generic_objRef conv_port = generic_objRef(new i_assign_conn_obj("i_assign_conn_obj_phi" + STR(id++)));
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<i_assign_conn_obj>(conv_port)->add_bitsize(in_bitsize);
                              generic_objRef fu_src_obj;
                              if(state_info && state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() && state_info->moved_op_def_set.find(cur_phi_tree_var) == state_info->moved_op_def_set.end())
                              {
                                 unsigned int src_storage_value = HLS->storage_value_information->get_storage_value_index(*e_it, cur_phi_tree_var);
                                 unsigned int src_r_index = HLS->Rreg->get_register(src_storage_value);
                                 fu_src_obj = HLS->Rreg->get(src_r_index);
                              }
                              else
                              {
                                 vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                                 fu_src_obj = HLS->Rfu->get(src_def_op);
                              }

                              const CustomOrderedSet<vertex>& states_out = HLS->Rliv->get_state_out(*e_it, *op, var_written);
                              const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                              for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                              {
                                 HLS->Rconn->add_data_transfer(fu_src_obj, conv_port, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                                 HLS->Rconn->add_data_transfer(conv_port, tgt_reg_obj, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - add data transfer from " << fu_src_obj->get_string() << " to " << conv_port->get_string() << " port 0:0 from state "
                                                                                  << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it) + " for " +
                                                                                         HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(cur_phi_tree_var));
                                 generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                                 GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*e_it, *s_out_it, commandport_obj::data_operation_pair(cur_phi_tree_var, *op)));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + tgt_reg_obj->get_string() + " from " << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it));
                              }
                           }
                        }
                        else
                        {
                           auto varObj = HLS_manager::io_binding_type(tree_temp, 0);
                           if(isZeroObj(tree_temp, TreeM))
                              varObj = HLS_manager::io_binding_type(0, 0);
                           generic_objRef conv_port;
                           auto key = std::make_tuple(in_bitsize, i_assign, varObj);
                           if(connCache.find(key) == connCache.end())
                           {
                              conv_port = generic_objRef(new i_assign_conn_obj("i_assign_conn_obj_" + STR(id++)));
                              if(isConstantObj(std::get<0>(varObj), TreeM))
                                 connCache[key] = conv_port;
                              HLS->Rconn->add_sparse_logic(conv_port);
                              GetPointer<i_assign_conn_obj>(conv_port)->add_bitsize(in_bitsize);
                              determine_connection(*op, varObj, conv_port, 0, 0, data, in_bitsize);
                           }
                           else
                              conv_port = connCache.find(key)->second;
                           create_single_conn(data, *op, conv_port, tgt_reg_obj, 0, 0, cur_phi_tree_var, in_bitsize, false);
                        }
                     }
                     else if(tree_helper::is_real(TreeM, var_written))
                     {
                        generic_objRef conv_port = generic_objRef(new f_assign_conn_obj("f_assign_conn_obj_" + STR(id++)));
                        HLS->Rconn->add_sparse_logic(conv_port);
                        GetPointer<f_assign_conn_obj>(conv_port)->add_bitsize(in_bitsize);
                        if(phi_postponed)
                        {
                           if(HLS->Rliv->has_state_out(*e_it, *op, var_written))
                           {
                              generic_objRef fu_src_obj;
                              if(state_info && state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() && state_info->moved_op_def_set.find(cur_phi_tree_var) == state_info->moved_op_def_set.end())
                              {
                                 unsigned int src_storage_value = HLS->storage_value_information->get_storage_value_index(*e_it, cur_phi_tree_var);
                                 unsigned int src_r_index = HLS->Rreg->get_register(src_storage_value);
                                 fu_src_obj = HLS->Rreg->get(src_r_index);
                              }
                              else
                              {
                                 vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                                 fu_src_obj = HLS->Rfu->get(src_def_op);
                              }

                              const CustomOrderedSet<vertex>& states_out = HLS->Rliv->get_state_out(*e_it, *op, var_written);
                              const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                              for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                              {
                                 HLS->Rconn->add_data_transfer(fu_src_obj, conv_port, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                                 HLS->Rconn->add_data_transfer(conv_port, tgt_reg_obj, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                               "       - add data transfer from " << fu_src_obj->get_string() << " to " << conv_port->get_string() << " port 0:0 from state "
                                                                                  << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it) + " for " +
                                                                                         HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(cur_phi_tree_var));
                                 generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                                 GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*e_it, *s_out_it, commandport_obj::data_operation_pair(cur_phi_tree_var, *op)));
                                 PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + tgt_reg_obj->get_string() + " from " << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it));
                              }
                           }
                        }
                        else
                        {
                           determine_connection(*op, HLS_manager::io_binding_type(tree_temp, 0), conv_port, 0, 0, data, in_bitsize);
                           create_single_conn(data, *op, conv_port, tgt_reg_obj, 0, 0, cur_phi_tree_var, in_bitsize, false);
                        }
                     }
                     else
                        THROW_ERROR("not expected conversion " + STR(cur_phi_tree_var) + " " + STR(in_bitsize) + " " + STR(out_bitsize) + " " + TreeM->get_tree_node_const(data->CGetOpNodeInfo(*op)->GetNodeId())->ToString());
                  }
                  else
                  {
                     if(phi_postponed)
                     {
                        // std::cerr << "phi postponed 1" << std::endl;
                        if(HLS->Rliv->has_state_out(*e_it, *op, var_written))
                        {
                           generic_objRef fu_src_obj;
                           if(state_info && state_info->moved_op_use_set.find(var_written) != state_info->moved_op_use_set.end() && state_info->moved_op_def_set.find(cur_phi_tree_var) == state_info->moved_op_def_set.end())
                           {
                              unsigned int src_storage_value = HLS->storage_value_information->get_storage_value_index(*e_it, cur_phi_tree_var);
                              unsigned int src_r_index = HLS->Rreg->get_register(src_storage_value);
                              fu_src_obj = HLS->Rreg->get(src_r_index);
                           }
                           else
                           {
                              vertex src_def_op = HLS->Rliv->get_op_where_defined(cur_phi_tree_var);
                              fu_src_obj = HLS->Rfu->get(src_def_op);
                           }
                           const CustomOrderedSet<vertex>& states_out = HLS->Rliv->get_state_out(*e_it, *op, var_written);
                           const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                           for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                           {
                              HLS->Rconn->add_data_transfer(fu_src_obj, tgt_reg_obj, 0, 0, data_transfer(cur_phi_tree_var, in_bitsize, *e_it, *s_out_it, *op));
                              PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                            "       - add data transfer from " << fu_src_obj->get_string() << " to " << tgt_reg_obj->get_string() << " port 0:0 from state "
                                                                               << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it) + " for " +
                                                                                      HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(cur_phi_tree_var));
                              generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                              GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*e_it, *s_out_it, commandport_obj::data_operation_pair(cur_phi_tree_var, *op)));
                              PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + tgt_reg_obj->get_string() + " from " << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it));
                           }
                        }
                     }
                     else
                        determine_connection(*op, HLS_manager::io_binding_type(tree_temp, 0), tgt_reg_obj, 0, 0, data, in_bitsize);
                  }
               }
               cur_phi_tree_var = 0;
            }
         }
      }
      else
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "  * Ending Operation: " + GET_NAME(data, *op));
         HLS->Rconn->bind_command_port(*op, conn_binding::IN, commandport_obj::OPERATION, data);

         PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - FU: " + HLS->allocation_information->get_fu_name(HLS->Rfu->get_assign(*op)).first);
         const generic_objRef fu_obj = HLS->Rfu->get(*op);

         unsigned int var_written = HLSMgr->get_produced_value(HLS->functionId, *op);
         if((GET_TYPE(data, *op) & (TYPE_MULTIIF)) != 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (multi-way if value)");
            unsigned int node_id = data->CGetOpNodeInfo(*op)->GetNodeId();
            std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, *op);
            generic_objRef TargetPort = HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::MULTIIF, *op, data);
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(*op);
            const CustomOrderedSet<vertex>::const_iterator e_it_end = ending_states.end();
            for(auto e_it = ending_states.begin(); e_it_end != e_it; ++e_it)
            {
               HLS->Rconn->add_data_transfer(fu_obj, TargetPort, 0, 0, data_transfer(node_id, var_read.size(), *e_it, NULL_VERTEX, *op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - add data transfer from " << fu_obj->get_string() << " to " << TargetPort->get_string() << " in state " << HLS->Rliv->get_name(*e_it) + " for " + STR(node_id));
               GetPointer<commandport_obj>(TargetPort)->add_activation(commandport_obj::transition(*e_it, NULL_VERTEX, commandport_obj::data_operation_pair(node_id, *op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - add activation for " + TargetPort->get_string() + " in state " << HLS->Rliv->get_name(*e_it));
            }
         }
         else if(var_written == 0)
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (no value produced)");
         else if((GET_TYPE(data, *op) & TYPE_IF) != 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (boolean value)");
            generic_objRef TargetPort = HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::CONDITION, *op, data);
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(*op);
            const CustomOrderedSet<vertex>::const_iterator e_it_end = ending_states.end();
            for(auto e_it = ending_states.begin(); e_it_end != e_it; ++e_it)
            {
               HLS->Rconn->add_data_transfer(fu_obj, TargetPort, 0, 0, data_transfer(var_written, tree_helper::size(TreeM, var_written), *e_it, NULL_VERTEX, *op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - add data transfer from " << fu_obj->get_string() << " to " << TargetPort->get_string() << " in state " << HLS->Rliv->get_name(*e_it) + "for condition");
               GetPointer<commandport_obj>(TargetPort)->add_activation(commandport_obj::transition(*e_it, NULL_VERTEX, commandport_obj::data_operation_pair(var_written, *op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - add activation for " + TargetPort->get_string() + " in state " << HLS->Rliv->get_name(*e_it));
            }
         }
         else if((GET_TYPE(data, *op) & TYPE_SWITCH) != 0)
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: (switch value)");
            generic_objRef TargetPort = HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::SWITCH, *op, data);
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(*op);
            const CustomOrderedSet<vertex>::const_iterator e_it_end = ending_states.end();
            for(auto e_it = ending_states.begin(); e_it_end != e_it; ++e_it)
            {
               HLS->Rconn->add_data_transfer(fu_obj, TargetPort, 0, 0, data_transfer(var_written, tree_helper::size(TreeM, var_written), *e_it, NULL_VERTEX, *op));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                             "       - add data transfer from " << fu_obj->get_string() << " to " << TargetPort->get_string() << " in state "
                                                                << HLS->Rliv->get_name(*e_it) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var_written));
               GetPointer<commandport_obj>(TargetPort)->add_activation(commandport_obj::transition(*e_it, NULL_VERTEX, commandport_obj::data_operation_pair(var_written, *op)));
               PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - add activation for " + TargetPort->get_string() + " in state " << HLS->Rliv->get_name(*e_it));
            }
         }
         else
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - Write: " + behavioral_helper->PrintVariable(var_written));
            const CustomOrderedSet<vertex>& ending_states = HLS->Rliv->get_state_where_end(*op);
            const CustomOrderedSet<vertex>::const_iterator e_it_end = ending_states.end();
            for(auto e_it = ending_states.begin(); e_it_end != e_it; ++e_it)
            {
               if(HLS->Rliv->has_state_out(*e_it, *op, var_written))
               {
                  const CustomOrderedSet<vertex>& states_out = HLS->Rliv->get_state_out(*e_it, *op, var_written);
                  const CustomOrderedSet<vertex>::const_iterator s_out_it_end = states_out.end();
                  for(auto s_out_it = states_out.begin(); s_out_it != s_out_it_end; ++s_out_it)
                  {
                     unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(*s_out_it, var_written);
                     unsigned int r_index = HLS->Rreg->get_register(storage_value);
                     generic_objRef tgt_reg_obj = HLS->Rreg->get(r_index);
                     HLS->Rconn->add_data_transfer(fu_obj, tgt_reg_obj, 0, 0, data_transfer(var_written, tree_helper::size(TreeM, var_written), *e_it, *s_out_it, *op));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                   "       - add data transfer from " << fu_obj->get_string() << " to " << tgt_reg_obj->get_string() << " from state "
                                                                      << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it) + " for " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(var_written));
                     generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
                     GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*e_it, *s_out_it, commandport_obj::data_operation_pair(var_written, *op)));
                     PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "       - write enable for " + tgt_reg_obj->get_string() + " from " << HLS->Rliv->get_name(*e_it) + " to state " + HLS->Rliv->get_name(*s_out_it));
                  }
               }
               else
               {
                  /// if the variable does not belong to the live-out set, it means that it is used inside the state (chaining) and
                  /// this situation is managed above
                  PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - write in a data_transfer");
               }
            }
         }
      }
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Ended execution of interconnection binding");

#if 0
   const std::list<vertex> & support = HLS->Rliv->get_support();
   const std::list<vertex>::const_iterator vEnd = support.end();
   for(std::list<vertex>::const_iterator vIt = support.begin(); vIt != vEnd; vIt++)
   {
      const CustomOrderedSet<unsigned int>& live_in = HLS->Rliv->get_live_in(*vIt);
      for(CustomOrderedSet<unsigned int>::const_iterator k = live_in.begin(); k != live_in.end(); ++k)
      {
         if(!HLS->Rliv->has_state_in(*vIt, *k)) continue;
         const CustomOrderedSet<vertex>& states_in = HLS->Rliv->get_state_in(*vIt,*k);
         const CustomOrderedSet<vertex>::const_iterator si_it_end = states_in.end();
         for(CustomOrderedSet<vertex>::const_iterator si_it=states_in.begin(); si_it != si_it_end; si_it++)
         {
            const CustomOrderedSet<unsigned int>& live_in_src = HLS->Rliv->get_live_in(*si_it);
            if (live_in_src.find(*k) == live_in_src.end()) continue;
            unsigned int storage_value_src = HLS->storage_value_information->get_storage_value_index(*si_it, *k);
            unsigned int r_index_src = HLS->Rreg->get_register(storage_value_src);
            unsigned int storage_value_dest = HLS->storage_value_information->get_storage_value_index(*vIt, *k);
            unsigned int r_index_dest = HLS->Rreg->get_register(storage_value_dest);
            generic_objRef src_reg_obj = HLS->Rreg->get(r_index_src);
            generic_objRef tgt_reg_obj = HLS->Rreg->get(r_index_dest);
            if (src_reg_obj == tgt_reg_obj) continue;
            PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "     - copying from register " + STR(r_index_src) + " to register " + STR(r_index_dest) + " in transition " + HLS->Rliv->get_name(*si_it) + " -> " + HLS->Rliv->get_name(*vIt));
            HLS->Rconn->add_data_transfer(src_reg_obj, tgt_reg_obj, 0, data_transfer(*k, tree_helper::size(TreeM, TreeM, *k), *si_it, *vIt, NULL_VERTEX));
            generic_objRef enable_obj = GetPointer<register_obj>(tgt_reg_obj)->get_wr_enable();
            GetPointer<commandport_obj>(enable_obj)->add_activation(commandport_obj::transition(*si_it, *vIt, commandport_obj::data_operation_pair(0, NULL_VERTEX));
         }
      }
   }
#endif
}

unsigned int mux_connection_binding::mux_interconnection()
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Starting datapath interconnection based on mux architecture");

   unsigned int allocated_mux = 0;
   unsigned int iteration = 0;

   for(const auto& connection : HLS->Rconn->get_data_transfers())
   {
      const generic_objRef unit = std::get<0>(connection.first);
      unsigned int operand = std::get<1>(connection.first);
      unsigned int port_index = std::get<2>(connection.first);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Unit: " + unit->get_string() + "(" + std::to_string(operand) + ":" + std::to_string(port_index) + "): " + std::to_string(connection.second.size()) + " connections");
      allocated_mux += input_logic(connection.second, unit, operand, port_index, iteration);
      ++iteration;
   }

   return allocated_mux;
}

unsigned int mux_connection_binding::input_logic(const conn_binding::ConnectionSources& srcs, const generic_objRef tgt, unsigned int op, unsigned int port_index, unsigned int iteration)
{
   static unsigned int used_mux = 0;
   unsigned int starting_value = used_mux;

   /// if it's a one-to-one connection, a directed link can be used
   if(srcs.size() == 1)
   {
      generic_objRef op1 = srcs.begin()->first;
      THROW_ASSERT(op1, "Target \"" + tgt->get_string() + "\" connected with an undefined source");
      connection_objRef conn_obj = connection_objRef(new direct_conn(srcs.begin()->second));
      HLS->Rconn->AddConnectionCB(op1, tgt, op, port_index, conn_obj);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  - Direct connection between " + op1->get_string() + " and " + tgt->get_string() + "(" + STR(op) + ":" + STR(port_index) + ")");
      return 0;
   }

   /// map between the source object and the resulting tree of multiplexers to the current target object
   std::map<generic_objRef, std::vector<std::pair<generic_objRef, unsigned int>>> src_mux_tree;

   /// map between a data_transfer and object associated at the moment
   std::map<data_transfer, generic_objRef> var2obj;
   /// map between a generic_obj and list of data_transfer associated with this object
   std::map<generic_objRef, std::list<data_transfer>> obj2var;
   /// map between a data_transfer and original source object
   std::map<data_transfer, generic_objRef> var2src;

#ifndef NDEBUG
   std::map<std::pair<vertex, vertex>, generic_objRef> check_sources;
#endif

   std::list<generic_objRef> to_allocate;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "  - Connection from: ");
   for(const auto& src : srcs)
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     * Source: " + src.first->get_string() + " ");
      const CustomOrderedSet<data_transfer>& vars = src.second;
      THROW_ASSERT(vars.size(), "A connection should contain at least one data-transfer");
      for(const auto& var : vars)
      {
         if(std::get<0>(var) == INFINITE_UINT)
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - var: (bool) from. " + HLS->Rliv->get_name(std::get<2>(var)) + " to " + HLS->Rliv->get_name(std::get<3>(var)));
         else if(std::get<0>(var) != 0)
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                          "       - var: " + HLSMgr->CGetFunctionBehavior(funId)->CGetBehavioralHelper()->PrintVariable(std::get<0>(var)) + " of size " + STR(std::get<1>(var)) + " from. " + HLS->Rliv->get_name(std::get<2>(var)) + " to " +
                              HLS->Rliv->get_name(std::get<3>(var)));
         else
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - size: " + STR(std::get<1>(var)) + " from. " + HLS->Rliv->get_name(std::get<2>(var)) + " to " + HLS->Rliv->get_name(std::get<3>(var)));

         var2obj[var] = src.first;
         var2src[var] = src.first;
         obj2var[src.first].push_back(var);
#ifndef NDEBUG
         if(check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var))) != check_sources.end() && check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var)))->second != src.first)
            THROW_ERROR("two different sources for the same transition: from. " + HLS->Rliv->get_name(std::get<2>(var)) + " to " + HLS->Rliv->get_name(std::get<3>(var)) + " source 1 " + src.first->get_string() + " source 2 " +
                        check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var)))->second->get_string());
         else if(check_sources.find(std::make_pair(std::get<2>(var), std::get<3>(var))) == check_sources.end())
            check_sources[std::make_pair(std::get<2>(var), std::get<3>(var))] = src.first;
#endif
      }
      if(src.first->get_type() != generic_obj::REGISTER)
         to_allocate.push_back(src.first);
      else
         to_allocate.push_front(src.first);
   }

   std::string tgt_string = tgt->get_string() + "_" + STR(op);
   if(to_allocate.size() > 1)
   {
      unsigned int level = 0;
      std::map<unsigned int, unsigned int> level_map;
      do
      {
         /// all the inputs are connected with 2:1 multiplexers. Two inputs are taken at each time.
         generic_objRef first = to_allocate.front();
         to_allocate.pop_front();
         generic_objRef second = to_allocate.front();
         to_allocate.pop_front();

         if(GetPointer<mux_obj>(first))
         {
            level = GetPointer<mux_obj>(first)->get_level() + 1;
         }

         std::string mux_name = "MUX_" + STR(iteration) + "_" + tgt_string + "_" + STR(level) + "_" + STR(level_map[level]++);
         generic_objRef mux = generic_objRef(new mux_obj(first, second, level, mux_name, tgt));
         ++used_mux;
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, GetPointer<mux_obj>(mux)->get_string());

         generic_objRef sel_port = HLS->Rconn->bind_selector_port(conn_binding::IN, commandport_obj::SELECTOR, mux, 0);
         GetPointer<mux_obj>(mux)->set_selector(sel_port);

         to_allocate.push_back(mux);

         std::list<data_transfer>::iterator v;

         /// stuff for the first input
         for(v = obj2var[first].begin(); v != obj2var[first].end(); ++v)
         {
            if(GetPointer<mux_obj>(var2obj[*v]))
               GetPointer<mux_obj>(var2obj[*v])->set_target(mux);

            var2obj[*v] = mux;
            obj2var[mux].push_back(*v);

            if(std::find(src_mux_tree[var2src[*v]].begin(), src_mux_tree[var2src[*v]].end(), std::make_pair(mux, T_COND)) == src_mux_tree[var2src[*v]].end())
               src_mux_tree[var2src[*v]].push_back(std::make_pair(mux, T_COND));

            GetPointer<mux_obj>(mux)->add_bitsize(std::get<1>(*v));

            GetPointer<commandport_obj>(sel_port)->add_activation(commandport_obj::transition(std::get<2>(*v), std::get<3>(*v), commandport_obj::data_operation_pair(std::get<0>(*v), std::get<4>(*v))));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - add mux activation for " + sel_port->get_string() + " from state " << HLS->Rliv->get_name(std::get<2>(*v)) + " to state " + HLS->Rliv->get_name(std::get<3>(*v)));
         }
         /// stuff for the second input
         for(v = obj2var[second].begin(); v != obj2var[second].end(); ++v)
         {
            if(GetPointer<mux_obj>(var2obj[*v]))
               GetPointer<mux_obj>(var2obj[*v])->set_target(mux);

            var2obj[*v] = mux;
            obj2var[mux].push_back(*v);

            if(std::find(src_mux_tree[var2src[*v]].begin(), src_mux_tree[var2src[*v]].end(), std::make_pair(mux, F_COND)) == src_mux_tree[var2src[*v]].end())
               src_mux_tree[var2src[*v]].push_back(std::make_pair(mux, F_COND));

            GetPointer<mux_obj>(mux)->add_bitsize(std::get<1>(*v));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - FALSE input for " + sel_port->get_string() + " from state " << HLS->Rliv->get_name(std::get<2>(*v)) + " to state " + HLS->Rliv->get_name(std::get<3>(*v)));
         }
      } while(to_allocate.size() > 1);

      /// specialize connections between sources and target
      for(const auto& src : srcs)
      {
         connection_objRef conn_obj = connection_objRef(new mux_conn(src.second, src_mux_tree[src.first]));
         HLS->Rconn->AddConnectionCB(src.first, tgt, op, port_index, conn_obj);
      }
   }
   else
      THROW_ERROR("no mux to_allocate" + STR(to_allocate.size()));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "");
   return used_mux - starting_value;
}

unsigned int mux_connection_binding::object_bitsize(const tree_managerRef TreeM, const HLS_manager::io_binding_type& obj) const
{
   unsigned int first = std::get<0>(obj);
   unsigned int second = std::get<1>(obj);
   if(first)
   {
      unsigned int type_index = tree_helper::get_type_index(TreeM, first);
      unsigned int bus_addr_bitsize = HLSMgr->get_address_bitsize();

      if(tree_helper::is_an_array(TreeM, type_index) || tree_helper::is_a_struct(TreeM, type_index) || tree_helper::is_an_union(TreeM, type_index) /*|| tree_helper::is_a_complex(TreeM, type_index)*/)
         return bus_addr_bitsize;
      else
         return tree_helper::size(TreeM, first);
   }
   else
   {
      if(second)
      {
         unsigned int count;
         for(count = 1; second >= (1u << count); ++count)
            ;
         return count + 1;
      }
      else
         return 1;
   }
}

unsigned int mux_connection_binding::swap_p(const OpGraphConstRef data, vertex op, unsigned int num, std::vector<HLS_manager::io_binding_type>& vars_read, const BehavioralHelperConstRef behavioral_helper, const tree_managerRef TreeM)
{
   std::string operation = data->CGetOpNodeInfo(op)->GetOperation();
   if(operation == STOK(TOK_PLUS_EXPR) || operation == STOK(TOK_POINTER_PLUS_EXPR) || operation == STOK(TOK_MULT_EXPR) || operation == STOK(TOK_BIT_IOR_EXPR) || operation == STOK(TOK_BIT_XOR_EXPR) || operation == STOK(TOK_BIT_AND_EXPR) ||
      operation == STOK(TOK_EQ_EXPR) || operation == STOK(TOK_NE_EXPR) || operation == STOK(TOK_WIDEN_SUM_EXPR) || operation == STOK(TOK_WIDEN_MULT_EXPR))
   {
      if(swap_computed_table.find(op) != swap_computed_table.end())
      {
         if(num == 0)
            return 1;
         else
            return 0;
      }
      if(noswap_computed_table.find(op) != noswap_computed_table.end())
      {
         return num;
      }

      fu_binding& fu = *(HLS->Rfu);
      resource_id_type resource_id = std::make_pair(fu.get_assign(op), fu.get_index(op));
      std::map<unsigned int, CustomOrderedSet<unsigned int>> regs_in_op;
      std::map<unsigned int, CustomOrderedSet<unsigned int>> chained_in_op;
      std::map<unsigned int, CustomOrderedSet<resource_id_type>> module_in_op;
      bool has_constant = false;
      for(unsigned int port_index = 0; port_index < vars_read.size() && !has_constant; ++port_index)
      {
         unsigned int tree_var = std::get<0>(vars_read[port_index]);
         if(tree_var != 0)
         {
            if(behavioral_helper->is_a_constant(tree_var))
               has_constant = true;
            else
            {
               const CustomOrderedSet<vertex>& running_states = HLS->Rliv->get_state_where_run(op);
               const CustomOrderedSet<vertex>::const_iterator rs_it_end = running_states.end();
               for(auto rs_it = running_states.begin(); rs_it != rs_it_end; ++rs_it)
               {
                  vertex state = *rs_it;
                  if(tree_helper::is_parameter(TreeM, tree_var) || !HLS->Rliv->has_op_where_defined(tree_var))
                  {
                     chained_in_op[port_index].insert(tree_var); /// it is not chained but from the mux binding it counts as input to the mux tree
                     // std::cerr << "port " << port_index << " chained " << tree_var << std::endl;
                  }
                  else
                  {
                     vertex def_op = HLS->Rliv->get_op_where_defined(tree_var);
                     {
                        const CustomOrderedSet<vertex>& def_op_ending_states = HLS->Rliv->get_state_where_end(def_op);
                        if((GET_TYPE(data, def_op) & TYPE_PHI) == 0)
                        {
                           if(def_op_ending_states.find(state) != def_op_ending_states.end())
                           {
                              if(fu.get_index(def_op) != INFINITE_UINT)
                                 module_in_op[port_index].insert(std::make_pair(fu.get_assign(def_op), fu.get_index(def_op)));
                              else
                                 chained_in_op[port_index].insert(tree_var);
                              // std::cerr << "port " << port_index << " chained " << tree_var << std::endl;
                           }
                           else if(HLS->storage_value_information->is_a_storage_value(state, tree_var))
                           {
                              unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(state, tree_var);
                              regs_in_op[port_index].insert(HLS->Rreg->get_register(storage_value));
                              // std::cerr << "port " << port_index << " reg " << HLS->Rreg->get_register(storage_value) << std::endl;
                           }
                        }
                        else
                        {
                           unsigned int storage_value = HLS->storage_value_information->get_storage_value_index(state, tree_var);
                           regs_in_op[port_index].insert(HLS->Rreg->get_register(storage_value));
                           // std::cerr << "port " << port_index << " reg " << HLS->Rreg->get_register(storage_value) << std::endl;
                        }
                     }
                  }
               }
            }
         }
         else
            has_constant = true;
      }
      if(has_constant)
      {
         noswap_computed_table.insert(op);
         return num;
      }
      else
      {
         size_t n_mux_in_a_0 = 0;
         size_t n_mux_in_a_1 = 0;
         size_t n_mux_in_b_0;
         size_t n_mux_in_b_1;

         n_mux_in_a_0 += regs_in.find(resource_id) == regs_in.end() ? 0 : (regs_in.find(resource_id)->second.find(0) == regs_in.find(resource_id)->second.end() ? 0 : static_cast<size_t>(regs_in.find(resource_id)->second.find(0)->second.size()));
         n_mux_in_a_0 +=
             chained_in.find(resource_id) == chained_in.end() ? 0 : (chained_in.find(resource_id)->second.find(0) == chained_in.find(resource_id)->second.end() ? 0 : static_cast<size_t>(chained_in.find(resource_id)->second.find(0)->second.size()));
         n_mux_in_a_0 += module_in.find(resource_id) == module_in.end() ? 0 : (module_in.find(resource_id)->second.find(0) == module_in.find(resource_id)->second.end() ? 0 : static_cast<size_t>(module_in.find(resource_id)->second.find(0)->second.size()));

         n_mux_in_a_1 += regs_in.find(resource_id) == regs_in.end() ? 0 : (regs_in.find(resource_id)->second.find(1) == regs_in.find(resource_id)->second.end() ? 0 : static_cast<size_t>(regs_in.find(resource_id)->second.find(1)->second.size()));
         n_mux_in_a_1 +=
             chained_in.find(resource_id) == chained_in.end() ? 0 : (chained_in.find(resource_id)->second.find(1) == chained_in.find(resource_id)->second.end() ? 0 : static_cast<size_t>(chained_in.find(resource_id)->second.find(1)->second.size()));
         n_mux_in_a_1 += module_in.find(resource_id) == module_in.end() ? 0 : (module_in.find(resource_id)->second.find(1) == module_in.find(resource_id)->second.end() ? 0 : static_cast<size_t>(module_in.find(resource_id)->second.find(1)->second.size()));

         n_mux_in_b_0 = n_mux_in_a_0;
         n_mux_in_b_1 = n_mux_in_a_1;

         const std::map<unsigned int, CustomOrderedSet<unsigned int>>::const_iterator rio_it_end = regs_in_op.end();
         for(std::map<unsigned int, CustomOrderedSet<unsigned int>>::const_iterator rio_it = regs_in_op.begin(); rio_it != rio_it_end; ++rio_it)
         {
            if(rio_it->first == 0)
            {
               if(regs_in.find(resource_id)->second.find(0) == regs_in.find(resource_id)->second.end())
               {
                  n_mux_in_a_0 += static_cast<size_t>(rio_it->second.size());
               }
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator rio2_it_end = rio_it->second.end();
                  for(auto rio2_it = rio_it->second.begin(); rio2_it != rio2_it_end; ++rio2_it)
                  {
                     if(regs_in.find(resource_id)->second.find(0)->second.find(*rio2_it) == regs_in.find(resource_id)->second.find(0)->second.end())
                     {
                        ++n_mux_in_a_0;
                     }
                  }
               }
               if(regs_in.find(resource_id)->second.find(1) == regs_in.find(resource_id)->second.end())
               {
                  n_mux_in_b_1 += static_cast<size_t>(rio_it->second.size());
               }
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator rio2_it_end = rio_it->second.end();
                  for(auto rio2_it = rio_it->second.begin(); rio2_it != rio2_it_end; ++rio2_it)
                  {
                     if(regs_in.find(resource_id)->second.find(1)->second.find(*rio2_it) == regs_in.find(resource_id)->second.find(1)->second.end())
                     {
                        ++n_mux_in_b_1;
                     }
                  }
               }
            }
            else
            {
               if(regs_in.find(resource_id)->second.find(1) == regs_in.find(resource_id)->second.end())
                  n_mux_in_a_1 += static_cast<size_t>(rio_it->second.size());
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator rio2_it_end = rio_it->second.end();
                  for(auto rio2_it = rio_it->second.begin(); rio2_it != rio2_it_end; ++rio2_it)
                  {
                     if(regs_in.find(resource_id)->second.find(1)->second.find(*rio2_it) == regs_in.find(resource_id)->second.find(1)->second.end())
                     {
                        ++n_mux_in_a_1;
                     }
                  }
               }
               if(regs_in.find(resource_id)->second.find(0) == regs_in.find(resource_id)->second.end())
                  n_mux_in_b_0 += static_cast<size_t>(rio_it->second.size());
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator rio2_it_end = rio_it->second.end();
                  for(auto rio2_it = rio_it->second.begin(); rio2_it != rio2_it_end; ++rio2_it)
                  {
                     if(regs_in.find(resource_id)->second.find(0)->second.find(*rio2_it) == regs_in.find(resource_id)->second.find(0)->second.end())
                     {
                        ++n_mux_in_b_0;
                     }
                  }
               }
            }
         }

         const std::map<unsigned int, CustomOrderedSet<unsigned int>>::const_iterator cio_it_end = chained_in_op.end();
         for(std::map<unsigned int, CustomOrderedSet<unsigned int>>::const_iterator cio_it = chained_in_op.begin(); cio_it != cio_it_end; ++cio_it)
         {
            if(cio_it->first == 0)
            {
               if(chained_in.find(resource_id)->second.find(0) == chained_in.find(resource_id)->second.end())
                  n_mux_in_a_0 += static_cast<size_t>(cio_it->second.size());
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator cio2_it_end = cio_it->second.end();
                  for(auto cio2_it = cio_it->second.begin(); cio2_it != cio2_it_end; ++cio2_it)
                  {
                     if(chained_in.find(resource_id)->second.find(0)->second.find(*cio2_it) == chained_in.find(resource_id)->second.find(0)->second.end())
                     {
                        ++n_mux_in_a_0;
                     }
                  }
               }
               if(chained_in.find(resource_id)->second.find(1) == chained_in.find(resource_id)->second.end())
                  n_mux_in_b_1 += static_cast<size_t>(cio_it->second.size());
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator cio2_it_end = cio_it->second.end();
                  for(auto cio2_it = cio_it->second.begin(); cio2_it != cio2_it_end; ++cio2_it)
                  {
                     if(chained_in.find(resource_id)->second.find(1)->second.find(*cio2_it) == chained_in.find(resource_id)->second.find(1)->second.end())
                     {
                        ++n_mux_in_b_1;
                     }
                  }
               }
            }
            else
            {
               if(chained_in.find(resource_id)->second.find(1) == chained_in.find(resource_id)->second.end())
                  n_mux_in_a_1 += static_cast<size_t>(cio_it->second.size());
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator cio2_it_end = cio_it->second.end();
                  for(auto cio2_it = cio_it->second.begin(); cio2_it != cio2_it_end; ++cio2_it)
                  {
                     if(chained_in.find(resource_id)->second.find(1)->second.find(*cio2_it) == chained_in.find(resource_id)->second.find(1)->second.end())
                     {
                        ++n_mux_in_a_1;
                     }
                  }
               }
               if(chained_in.find(resource_id)->second.find(0) == chained_in.find(resource_id)->second.end())
                  n_mux_in_b_0 += static_cast<size_t>(cio_it->second.size());
               else
               {
                  const CustomOrderedSet<unsigned int>::const_iterator cio2_it_end = cio_it->second.end();
                  for(auto cio2_it = cio_it->second.begin(); cio2_it != cio2_it_end; ++cio2_it)
                  {
                     if(chained_in.find(resource_id)->second.find(0)->second.find(*cio2_it) == chained_in.find(resource_id)->second.find(0)->second.end())
                     {
                        ++n_mux_in_b_0;
                     }
                  }
               }
            }
         }

         const std::map<unsigned int, CustomOrderedSet<resource_id_type>>::const_iterator mio_it_end = module_in_op.end();
         for(std::map<unsigned int, CustomOrderedSet<resource_id_type>>::const_iterator mio_it = module_in_op.begin(); mio_it != mio_it_end; ++mio_it)
         {
            if(mio_it->first == 0)
            {
               if(module_in.find(resource_id)->second.find(0) == module_in.find(resource_id)->second.end())
                  n_mux_in_a_0 += static_cast<size_t>(mio_it->second.size());
               else
               {
                  const CustomOrderedSet<resource_id_type>::const_iterator mio2_it_end = mio_it->second.end();
                  for(auto mio2_it = mio_it->second.begin(); mio2_it != mio2_it_end; ++mio2_it)
                  {
                     if(module_in.find(resource_id)->second.find(0)->second.find(*mio2_it) == module_in.find(resource_id)->second.find(0)->second.end())
                     {
                        ++n_mux_in_a_0;
                     }
                  }
               }
               if(module_in.find(resource_id)->second.find(1) == module_in.find(resource_id)->second.end())
                  n_mux_in_b_1 += static_cast<size_t>(mio_it->second.size());
               else
               {
                  const CustomOrderedSet<resource_id_type>::const_iterator mio2_it_end = mio_it->second.end();
                  for(auto mio2_it = mio_it->second.begin(); mio2_it != mio2_it_end; ++mio2_it)
                  {
                     if(module_in.find(resource_id)->second.find(1)->second.find(*mio2_it) == module_in.find(resource_id)->second.find(1)->second.end())
                     {
                        ++n_mux_in_b_1;
                     }
                  }
               }
            }
            else
            {
               if(module_in.find(resource_id)->second.find(1) == module_in.find(resource_id)->second.end())
                  n_mux_in_a_1 += static_cast<size_t>(mio_it->second.size());
               else
               {
                  const CustomOrderedSet<resource_id_type>::const_iterator mio2_it_end = mio_it->second.end();
                  for(auto mio2_it = mio_it->second.begin(); mio2_it != mio2_it_end; ++mio2_it)
                  {
                     if(module_in.find(resource_id)->second.find(1)->second.find(*mio2_it) == module_in.find(resource_id)->second.find(1)->second.end())
                     {
                        ++n_mux_in_a_1;
                     }
                  }
               }
               if(module_in.find(resource_id)->second.find(0) == module_in.find(resource_id)->second.end())
                  n_mux_in_b_0 += static_cast<size_t>(mio_it->second.size());
               else
               {
                  const CustomOrderedSet<resource_id_type>::const_iterator mio2_it_end = mio_it->second.end();
                  for(auto mio2_it = mio_it->second.begin(); mio2_it != mio2_it_end; ++mio2_it)
                  {
                     if(module_in.find(resource_id)->second.find(0)->second.find(*mio2_it) == module_in.find(resource_id)->second.find(0)->second.end())
                     {
                        ++n_mux_in_b_0;
                     }
                  }
               }
            }
         }

         // std::cerr << "resource_id " << resource_id.first << " - " << resource_id.second << std::endl;
         // std::cerr << GET_NAME(data, op) << ":" << data->CGetOpNodeInfo(op)->GetOperation() << " n_mux_in_a_0+n_mux_in_a_1=" << n_mux_in_a_0+n_mux_in_a_1 << " n_mux_in_b_0+n_mux_in_b_1=" << n_mux_in_b_0+n_mux_in_b_1 << std::endl;
         // std::cerr << " n_mux_in_a_0, n_mux_in_a_1=" << n_mux_in_a_0 << "-" << n_mux_in_a_1 << " n_mux_in_b_0, n_mux_in_b_1=" << n_mux_in_b_0 << "-" << n_mux_in_b_1 << std::endl;

         if(n_mux_in_a_0 + n_mux_in_a_1 <= n_mux_in_b_0 + n_mux_in_b_1)
         {
            if(regs_in_op.find(0) != regs_in_op.end())
               regs_in[resource_id][0].insert(regs_in_op.find(0)->second.begin(), regs_in_op.find(0)->second.end());
            if(regs_in_op.find(1) != regs_in_op.end())
               regs_in[resource_id][1].insert(regs_in_op.find(1)->second.begin(), regs_in_op.find(1)->second.end());
            if(chained_in_op.find(0) != chained_in_op.end())
               chained_in[resource_id][0].insert(chained_in_op.find(0)->second.begin(), chained_in_op.find(0)->second.end());
            if(chained_in_op.find(1) != chained_in_op.end())
               chained_in[resource_id][1].insert(chained_in_op.find(1)->second.begin(), chained_in_op.find(1)->second.end());
            if(module_in_op.find(0) != module_in_op.end())
               module_in[resource_id][0].insert(module_in_op.find(0)->second.begin(), module_in_op.find(0)->second.end());
            if(module_in_op.find(1) != module_in_op.end())
               module_in[resource_id][1].insert(module_in_op.find(1)->second.begin(), module_in_op.find(1)->second.end());
            noswap_computed_table.insert(op);
            return num;
         }
         else
         {
            if(regs_in_op.find(0) != regs_in_op.end())
               regs_in[resource_id][1].insert(regs_in_op.find(0)->second.begin(), regs_in_op.find(0)->second.end());
            if(regs_in_op.find(1) != regs_in_op.end())
               regs_in[resource_id][0].insert(regs_in_op.find(1)->second.begin(), regs_in_op.find(1)->second.end());
            if(chained_in_op.find(0) != chained_in_op.end())
               chained_in[resource_id][1].insert(chained_in_op.find(0)->second.begin(), chained_in_op.find(0)->second.end());
            if(chained_in_op.find(1) != chained_in_op.end())
               chained_in[resource_id][0].insert(chained_in_op.find(1)->second.begin(), chained_in_op.find(1)->second.end());
            if(module_in_op.find(0) != module_in_op.end())
               module_in[resource_id][1].insert(module_in_op.find(0)->second.begin(), module_in_op.find(0)->second.end());
            if(module_in_op.find(1) != module_in_op.end())
               module_in[resource_id][0].insert(module_in_op.find(1)->second.begin(), module_in_op.find(1)->second.end());
            // std::cerr << GET_NAME(data, op) << ":" << data->CGetOpNodeInfo(op)->GetOperation() << " swap" << std::endl;
            swap_computed_table.insert(op);
            if(num == 0)
               return 1;
            else
               return 0;
         }
      }
   }
   return num;
}

void mux_connection_binding::connect_array_index(unsigned int tree_index, generic_objRef fu_obj, unsigned int port_num, unsigned int port_index, unsigned int bus_addr_bitsize, const OpGraphConstRef data, const vertex& op)
{
   const tree_managerRef TreeM = HLSMgr->get_tree_manager();
   if(tree_helper::is_int(TreeM, tree_index))
   {
      auto varObj = HLS_manager::io_binding_type(tree_index, 0);
      if(isZeroObj(tree_index, TreeM))
         varObj = HLS_manager::io_binding_type(0, 0);
      auto key = std::make_tuple(bus_addr_bitsize, iu_conv, varObj);
      generic_objRef conv_port;
      if(connCache.find(key) == connCache.end())
      {
         conv_port = generic_objRef(new iu_conv_conn_obj("iu_conv_conn_obj_" + STR(id++)));
         if(isConstantObj(std::get<0>(varObj), TreeM))
            connCache[key] = conv_port;
         HLS->Rconn->add_sparse_logic(conv_port);
         GetPointer<iu_conv_conn_obj>(conv_port)->add_bitsize(bus_addr_bitsize);
         determine_connection(op, varObj, conv_port, 0, 0, data, bus_addr_bitsize);
      }
      else
         conv_port = connCache.find(key)->second;
      create_single_conn(data, op, conv_port, fu_obj, port_num, port_index, tree_index, bus_addr_bitsize, true);
   }
   else
      determine_connection(op, HLS_manager::io_binding_type(tree_index, 0), fu_obj, port_num, port_index, data, bus_addr_bitsize);
}
