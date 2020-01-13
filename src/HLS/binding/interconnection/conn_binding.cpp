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
 * @file conn_binding.cpp
 * @brief Class implementation of the interconnection binding data structure.
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "conn_binding.hpp"
#include "conn_binding_cs.hpp"

#include "hls_manager.hpp"
#include "hls_target.hpp"

#include "connection_obj.hpp"
#include "mux_conn.hpp"

#include "adder_conn_obj.hpp"
#include "bitfield_obj.hpp"
#include "commandport_obj.hpp"
#include "conn_binding_creator.hpp"
#include "conv_conn_obj.hpp"
#include "dataport_obj.hpp"
#include "dbgPrintHelper.hpp"
#include "fu_binding.hpp"
#include "funit_obj.hpp"
#include "multi_unbounded_obj.hpp"
#include "multiplier_conn_obj.hpp"
#include "mux_obj.hpp"
#include "omp_functions.hpp"
#include "register_obj.hpp"

#include "hls.hpp"

#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "time_model.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "behavioral_helper.hpp"

#include <boost/lexical_cast.hpp>

///. include
#include "Parameter.hpp"

/// HLS/module_allocation include
#include "allocation.hpp"
#include "allocation_information.hpp"

/// HLS/stg include
#include "state_transition_graph_manager.hpp"

/// HLS/virtual_components include
#include "generic_obj.hpp"

/// STL include
#include "custom_set.hpp"
#include <algorithm>
#include <list>
#include <tuple>
#include <utility>
#include <vector>

/// technology/physical_library include
#include "string_manipulation.hpp" // for GET_CLASS
#include "technology_node.hpp"

#define CONN_COLUMN_SIZE 40

unsigned conn_binding::unique_id = 0;

conn_binding::conn_binding(const BehavioralHelperConstRef _BH, const ParameterConstRef _parameters)
    : parameters(_parameters), debug_level(_parameters->get_class_debug_level(GET_CLASS(*this))), output_level(_parameters->getOption<int>(OPT_output_level)), BH(_BH)
{
}

conn_bindingRef conn_binding::create_conn_binding(const HLS_managerRef _HLSMgr, const hlsRef _HLS, const BehavioralHelperConstRef _BH, const ParameterConstRef _parameters)
{
   if(_parameters->isOption(OPT_context_switch))
   {
      auto omp_functions = GetPointer<OmpFunctions>(_HLSMgr->Rfuns);
      bool found = false;
      if(omp_functions->kernel_functions.find(_HLS->functionId) != omp_functions->kernel_functions.end())
         found = true;
      if(omp_functions->parallelized_functions.find(_HLS->functionId) != omp_functions->parallelized_functions.end())
         found = true;
      if(omp_functions->atomic_functions.find(_HLS->functionId) != omp_functions->atomic_functions.end())
         found = true;
      if(found)
         return conn_bindingRef(new conn_binding_cs(_BH, _parameters));
      else
         return conn_bindingRef(new conn_binding(_BH, _parameters));
   }
   else
      return conn_bindingRef(new conn_binding(_BH, _parameters));
}

conn_binding::~conn_binding() = default;

generic_objRef conn_binding::get_port(unsigned int var, conn_binding::direction_type dir)
{
   switch(dir)
   {
      case IN:
         THROW_ASSERT(input_ports.count(var), "Data port not stored into conn_binding class");
         return input_ports[var];
      case OUT:
         THROW_ASSERT(output_ports.count(var), "Data port not stored into conn_binding class");
         return output_ports[var];
      default:
         THROW_ERROR("Port kind not allowed");
   }
   return generic_objRef();
}

void conn_binding::add_data_transfer(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index, data_transfer data)
{
   conn_variables[ConnectionTarget(op2, operand, port_index)][op1].insert(data);
}

const std::map<conn_binding::ConnectionTarget, conn_binding::ConnectionSources>& conn_binding::get_data_transfers() const
{
   return conn_variables;
}

const conn_binding::conn_implementation_map& conn_binding::get_connection_implementations() const
{
   return conn_implementation;
}

void conn_binding::add_connection(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index, connection_objRef conn)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding connection " + op1->get_string() + " " + op2->get_string() + " " + STR(operand) + " " + STR(port_index));
   conn_implementation[connection(op1, op2, operand, port_index)] = conn;
}

connection_objRef conn_binding::get_connection(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index) const
{
   THROW_ASSERT(is_connection(op1, op2, operand, port_index), "missing connection between source and target");
   return conn_implementation.find(connection(op1, op2, operand, port_index))->second;
}

bool conn_binding::is_connection(const generic_objRef op1, const generic_objRef op2, unsigned int operand, unsigned int port_index) const
{
   return conn_implementation.find(connection(op1, op2, operand, port_index)) != conn_implementation.end();
}

generic_objRef conn_binding::get_command_port(const vertex& ver, conn_binding::direction_type dir, unsigned int mode)
{
   switch(dir)
   {
      case IN:
         THROW_ASSERT(command_input_ports.count(std::make_pair(ver, mode)), "Command port not stored into conn_binding class");
         return command_input_ports[std::make_pair(ver, mode)];
      case OUT:
         THROW_ASSERT(command_output_ports.count(ver), "Command port not stored into conn_binding class");
         return command_output_ports[ver];
      default:
         THROW_ERROR("Port kind not allowed");
   }
   return generic_objRef();
}

generic_objRef conn_binding::bind_port(unsigned int var, conn_binding::direction_type dir)
{
   switch(dir)
   {
      case IN:
      {
         if(input_ports.count(var) == 0)
            input_ports[var] = generic_objRef(new dataport_obj("IN_PORT_" + BH->PrintVariable(var), 0));
         return input_ports[var];
      }
      case OUT:
      {
         if(output_ports.count(var) == 0)
            output_ports[var] = generic_objRef(new dataport_obj("OUT_PORT_" + BH->PrintVariable(var), 0));
         return output_ports[var];
      }
      default:
         THROW_ERROR("Port kind not allowed");
   }
   return generic_objRef();
}

generic_objRef conn_binding::bind_command_port(const vertex& ver, conn_binding::direction_type dir, unsigned int mode, const OpGraphConstRef g)
{
   switch(dir)
   {
      case IN:
      {
         if(command_input_ports.count(std::make_pair(ver, mode)) == 0)
            command_input_ports[std::make_pair(ver, mode)] = generic_objRef(new commandport_obj(ver, mode, "IN_" + commandport_obj::get_mode_string(mode) + "_" + GET_NAME(g, ver)));
         return command_input_ports[std::make_pair(ver, mode)];
      }
      case OUT:
      {
         if(command_output_ports.count(ver) == 0)
            command_output_ports[ver] = generic_objRef(new commandport_obj(ver, mode, "OUT_" + commandport_obj::get_mode_string(mode) + "_" + GET_NAME(g, ver)));
         return command_output_ports[ver];
      }
      default:
         THROW_ERROR("Port kind not allowed");
   }
   return generic_objRef();
}

generic_objRef conn_binding::bind_selector_port(conn_binding::direction_type dir, unsigned int mode, const generic_objRef elem, unsigned int op)
{
   if(selectors.find(dir) == selectors.end() or selectors[dir].find(std::make_pair(elem, op)) == selectors[dir].end())
      selectors[dir][std::make_pair(elem, op)] = generic_objRef(new commandport_obj(elem, mode, (dir == IN ? "IN_" : "OUT_") + elem->get_string() + "_" + commandport_obj::get_mode_string(mode) + "_" + STR(op)));
   return selectors[dir][std::make_pair(elem, op)];
}

generic_objRef conn_binding::bind_selector_port(conn_binding::direction_type dir, unsigned int mode, const vertex& cond, const OpGraphConstRef data)
{
   if(activation_ports.find(cond) != activation_ports.end() and activation_ports[cond].find(dir) != activation_ports[cond].end())
      return activation_ports[cond][dir];
   generic_objRef port = generic_objRef(new commandport_obj(cond, mode, (dir == IN ? "IN_" : "OUT_") + commandport_obj::get_mode_string(mode) + "_" + GET_NAME(data, cond)));
   activation_ports[cond][dir] = port;
   return selectors[dir][std::make_pair(port, 0)] = port;
}

unsigned int conn_binding::get_component_num(const std::string& type) const
{
   if(component_num.find(type) == component_num.end())
      return 0;
   return component_num.find(type)->second;
}

void conn_binding::add_component(const std::string& type, unsigned int num)
{
   if(component_num.find(type) == component_num.end())
   {
      component_num[type] = num;
      return;
   }
   component_num[type] += num;
}

const std::map<std::string, unsigned int>& conn_binding::get_components() const
{
   return component_num;
}

unsigned int conn_binding::get_to_controller_ports() const
{
   return static_cast<unsigned int>(command_output_ports.size());
}

std::map<std::pair<vertex, unsigned int>, generic_objRef> conn_binding::get_command_input_ports() const
{
   return command_input_ports;
}

unsigned int conn_binding::get_from_controller_ports() const
{
   return static_cast<unsigned int>(command_input_ports.size());
}

bool conn_binding::check_pv_allconnected(structural_objectRef port_i)
{
   bool allconnected = true;
   for(unsigned int p = 0; p < GetPointer<port_o>(port_i)->get_ports_size() && allconnected; ++p)
   {
      structural_objectRef port_d = GetPointer<port_o>(port_i)->get_port(p);
      if(!GetPointer<port_o>(port_d)->find_bounded_object())
         allconnected = false;
   }

   return allconnected;
}

void conn_binding::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM)
{
   /// add command ports
   add_command_ports(HLSMgr, HLS, SM);

   /// add sparse logic
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding sparse logic to the datapath");
   add_sparse_logic_dp(HLS, SM, HLSMgr);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

#ifndef NDEBUG
   const HLSFlowStep_Type connection_type = HLS->Param->getOption<HLSFlowStep_Type>(OPT_datapath_interconnection_algorithm);
   /// up to now, circuit is general about interconnections. Now, proper interconnection architecture will be executed
   THROW_ASSERT(connection_type == HLSFlowStep_Type::MUX_INTERCONNECTION_BINDING, "Unexpected interconnection binding");
#endif
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Datapath interconnection using mux architecture");
   mux_connection(HLS, SM);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   const structural_objectRef circuit = SM->get_circ();
   std::map<unsigned int, structural_objectRef> null_values;
   for(unsigned int i = 0; i < GetPointer<module>(circuit)->get_internal_objects_size(); i++)
   {
      structural_objectRef curr_gate = GetPointer<module>(circuit)->get_internal_object(i);
      if(!GetPointer<module>(curr_gate) || GetPointer<module>(curr_gate)->get_id() == "scheduler_kernel")
         continue;
      for(unsigned int j = 0; j < GetPointer<module>(curr_gate)->get_in_port_size(); j++)
      {
         structural_objectRef port_i = GetPointer<module>(curr_gate)->get_in_port(j);
         // std::cerr << "port_i " << port_i->get_path() << " of size " << GET_TYPE_SIZE(port_i) << std::endl;
         if((port_i->get_kind() == port_o_K || port_i->get_kind() == port_vector_o_K) && GetPointer<port_o>(port_i)->find_bounded_object())
            continue;
         if(port_i->get_kind() == port_vector_o_K && check_pv_allconnected(port_i))
            continue;
         // std::cerr << "  empty\n";
         if(port_i->get_kind() == port_vector_o_K)
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(port_i)->get_ports_size(); ++p)
            {
               structural_objectRef port_d = GetPointer<port_o>(port_i)->get_port(p);
               auto bw = GET_TYPE_SIZE(port_d);
               if(null_values.find(bw) == null_values.end())
               {
                  structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", bw));
                  structural_objectRef const_obj = SM->add_constant("null_value_" + STR(bw), circuit, bool_type, STR(0));
                  null_values[bw] = const_obj;
               }
               if(!GetPointer<port_o>(port_d)->find_bounded_object())
                  SM->add_connection(port_d, null_values[bw]);
            }
         }
         else
         {
            auto bw = GET_TYPE_SIZE(port_i);
            if(null_values.find(bw) == null_values.end())
            {
               structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", bw));
               structural_objectRef const_obj = SM->add_constant("null_value_" + STR(bw), circuit, bool_type, STR(0));
               null_values[bw] = const_obj;
            }
            SM->add_connection(port_i, null_values[bw]);
         }
      }
      for(unsigned int j = 0; j < GetPointer<module>(curr_gate)->get_out_port_size(); j++)
      {
         structural_objectRef port_out = GetPointer<module>(curr_gate)->get_out_port(j);
         if((port_out->get_kind() == port_o_K || port_out->get_kind() == port_vector_o_K) && GetPointer<port_o>(port_out)->find_bounded_object())
            continue;
         if(port_out->get_kind() == port_vector_o_K && check_pv_allconnected(port_out))
            continue;

         if(port_out->get_kind() == port_vector_o_K)
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(port_out)->get_ports_size(); ++p)
            {
               structural_objectRef port_d = GetPointer<port_o>(port_out)->get_port(p);
               if(!GetPointer<port_o>(port_d)->find_bounded_object())
               {
                  std::string name = "null_out_signal_" + port_out->get_owner()->get_id() + "_" + port_out->get_id() + "_" + port_d->get_id();
                  structural_objectRef sign = SM->add_sign(name, circuit, port_d->get_typeRef());
                  SM->add_connection(port_d, sign);
               }
            }
         }
      }
   }
}

void conn_binding::mux_connection(const hlsRef HLS, const structural_managerRef SM)
{
   structural_objectRef circuit = SM->get_circ();

   // CustomOrderedSet<std::pair<std::string, std::string> > already_considered;
   for(std::map<std::tuple<generic_objRef, generic_objRef, unsigned int, unsigned int>, connection_objRef>::const_iterator i = conn_implementation.begin(); i != conn_implementation.end(); ++i)
   {
      generic_objRef src = std::get<0>(i->first);
      generic_objRef tgt = std::get<1>(i->first);

      THROW_ASSERT(src, "a NULL src may come from uninitialized variables. Target: " + tgt->get_string());
      unsigned int operand = std::get<2>(i->first);
      unsigned int port_index = std::get<3>(i->first);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Creating CONNECTION between " + src->get_string() + " and " + tgt->get_string() + "(" + STR(operand) + ":" + STR(port_index) + ")");

      structural_objectRef src_module = src->get_structural_obj();
      structural_objectRef tgt_module = tgt->get_structural_obj();
      THROW_ASSERT(src, "No source signal to " + tgt->get_string());
      structural_objectRef port_src, port_tgt;
      THROW_ASSERT(src_module, "No object associated to " + src->get_string());
      if(src_module->get_kind() == component_o_K)
      {
         auto* src_obj = GetPointer<module>(src_module);
         for(unsigned int ind = 0; ind < src_obj->get_out_port_size(); ind++)
         {
            auto curr_port = src_obj->get_out_port(ind);
            if(curr_port->get_id() == DONE_PORT_NAME)
               continue;
            if(GetPointer<port_o>(curr_port)->get_is_memory() || GetPointer<port_o>(curr_port)->get_is_global() || GetPointer<port_o>(curr_port)->get_is_extern())
               continue;
            port_src = curr_port;
            if(port_src->get_kind() == port_vector_o_K)
            {
               port_src = GetPointer<port_o>(port_src)->get_port(GetPointer<funit_obj>(src)->get_index() % GetPointer<port_o>(port_src)->get_ports_size());
            }
            break;
         }
      }
      else if(src_module->get_kind() == port_o_K || src_module->get_kind() == port_vector_o_K)
         port_src = src_module;
      if(!port_src)
      {
         src_module->print(std::cerr);
         THROW_ERROR("source module does not have the expected return_port: " + src_module->get_id() + " - " + src_module->get_path() + "\nCheck the module specification");
      }

      THROW_ASSERT(tgt_module, "No object associated to " + tgt->get_string());
      if(tgt_module->get_kind() == component_o_K)
      {
         auto* tgt_obj = GetPointer<module>(tgt_module);
         unsigned int num = 0;

         for(unsigned int ind = 0; ind < tgt_obj->get_in_port_size(); ind++)
         {
            auto curr_port = tgt_obj->get_in_port(ind);
            if(curr_port->get_id() == CLOCK_PORT_NAME || curr_port->get_id() == RESET_PORT_NAME || curr_port->get_id() == START_PORT_NAME)
               continue;
            if(GetPointer<port_o>(curr_port)->get_is_memory() || GetPointer<port_o>(curr_port)->get_is_global() || GetPointer<port_o>(curr_port)->get_is_extern())
               continue;
            if(num == operand)
            {
               port_tgt = curr_port;
               if(port_tgt->get_kind() == port_vector_o_K)
               {
                  port_tgt = GetPointer<port_o>(port_tgt)->get_port(port_index);
               }
               else
               {
                  THROW_ASSERT(port_index == 0, "expected 0 as port index");
               }
               break;
            }
            num++;
         }
      }
      else if(tgt_module->get_kind() == port_o_K || tgt_module->get_kind() == port_vector_o_K)
         port_tgt = tgt_module;
      if(!port_tgt)
      {
         tgt_module->print(std::cerr);
         THROW_ERROR("target module does not have the expected input port: " + tgt_module->get_id() + " - " + tgt_module->get_path() + "\nCheck the module specification");
      }

      structural_type_descriptorRef port_src_type = port_src->get_typeRef();
      structural_objectRef sign = src->get_out_sign();
      if(!sign)
      {
         std::string name = "out_" + src->get_string() + "_" + src->get_structural_obj()->get_id();
         sign = SM->add_sign(name, circuit, port_src_type);
         THROW_ASSERT(port_src_type->size, "size greater than one expected");
         src->set_out_sign(sign);
         SM->add_connection(port_src, sign);
      }

      switch(i->second->get_type())
      {
         case connection_obj::DIRECT_CONN:
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Creating DIRECTED CONNECTION");
            THROW_ASSERT(sign, "");
            THROW_ASSERT(port_tgt, tgt_module->get_path());
            unsigned int bits_src = GET_TYPE_SIZE(sign);
            unsigned int bits_tgt = GET_TYPE_SIZE(port_tgt);
            unsigned int conn_type = structural_type_descriptor::VECTOR_BOOL;

            if(bits_src != bits_tgt)
            {
               if(port_tgt->get_typeRef()->type == structural_type_descriptor::INT)
                  conn_type = structural_type_descriptor::INT;
               else if(port_tgt->get_typeRef()->type == structural_type_descriptor::UINT)
                  conn_type = structural_type_descriptor::UINT;
               else if(port_tgt->get_typeRef()->type == structural_type_descriptor::REAL)
                  conn_type = structural_type_descriptor::REAL;
               else
                  conn_type = structural_type_descriptor::VECTOR_BOOL;
            }
            add_datapath_connection(HLS->HLS_T->get_technology_manager(), SM, sign, port_tgt, conn_type);
            break;
         }
         case connection_obj::BY_MUX:
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Creating MUX TREE");
            mux_allocation(HLS, SM, sign, port_tgt, i->second);
            break;
         }
         default:
            THROW_ERROR("Connection type not allowed: " + STR(i->second->get_type()));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<-- ");
   }
}

void conn_binding::specialise_mux(const generic_objRef mux, unsigned int bits_tgt) const
{
   unsigned int data_size = GetPointer<mux_obj>(mux)->get_bitsize();
   data_size = std::max(data_size, bits_tgt);

   structural_objectRef mux_obj = mux->get_structural_obj();
   const module* mux_module = GetPointer<module>(mux_obj);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Specializing " + mux_obj->get_path() + ": " + STR(data_size));
   /// specializing multiplexer ports
   mux_module->get_in_port(1)->type_resize(data_size);
   mux_module->get_in_port(2)->type_resize(data_size);
   mux_module->get_out_port(0)->type_resize(data_size);
}

void conn_binding::mux_allocation(const hlsRef HLS, const structural_managerRef SM, structural_objectRef src, structural_objectRef tgt, connection_objRef conn)
{
   THROW_ASSERT(src, "mux_allocation - No source object");
   THROW_ASSERT(tgt, "mux_allocation - No target object");
   unsigned int bits_tgt = 0;
   structural_type_descriptor::s_type conn_type;
   if(tgt->get_typeRef()->type == structural_type_descriptor::INT)
   {
      bits_tgt = GET_TYPE_SIZE(tgt);
      conn_type = structural_type_descriptor::INT;
   }
   else if(tgt->get_typeRef()->type == structural_type_descriptor::UINT)
   {
      bits_tgt = GET_TYPE_SIZE(tgt);
      conn_type = structural_type_descriptor::UINT;
   }
   else if(tgt->get_typeRef()->type == structural_type_descriptor::REAL)
   {
      bits_tgt = GET_TYPE_SIZE(tgt);
      conn_type = structural_type_descriptor::REAL;
   }
   else
   {
      bits_tgt = GET_TYPE_SIZE(tgt);
      conn_type = structural_type_descriptor::VECTOR_BOOL;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---conn_binding::mux_allocation between " + src->get_path() + " and " + tgt->get_path());

   THROW_ASSERT(GetPointer<mux_conn>(conn), "The connection is not implemented through multiplexers");
   auto* mux_alloc = GetPointer<mux_conn>(conn);
   const std::vector<std::pair<generic_objRef, unsigned int>>& mux_tree = mux_alloc->get_mux_tree();
   THROW_ASSERT(mux_tree.size() > 0, "no mux into a mux connection");

   const structural_objectRef circuit = SM->get_circ();
   for(const auto& i : mux_tree)
   {
      structural_objectRef mux = i.first->get_structural_obj();
      unsigned int in_mux = i.second == T_COND ? 1 : 2;

      if(mux)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---MUX exists");
         auto* mux_object = GetPointer<module>(mux);

         /// adding input connection
         structural_objectRef mux_input = mux_object->get_in_port(in_mux);
         THROW_ASSERT(mux_input, "classic_datapath::mux_allocation - In port does not exist");
         add_datapath_connection(HLS->HLS_T->get_technology_manager(), SM, src, mux_input, conn_type);

         return;
      }
      else
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---MUX must be allocated");
         std::string name = i.first->get_string();
         mux = SM->add_module_from_technology_library(name, MUX_GATE_STD, HLS->HLS_T->get_technology_manager()->get_library(MUX_GATE_STD), circuit, HLS->HLS_T->get_technology_manager());
         i.first->set_structural_obj(mux);

         /// mux selector in datapath interface
         generic_objRef selector = GetPointer<mux_obj>(i.first)->GetSelector();
         structural_objectRef sel_obj = selector->get_structural_obj();
         THROW_ASSERT(sel_obj, "Selector obj not created");
         /// selector in mux object
         structural_objectRef mux_sel = mux->find_member("sel", port_o_K, mux);
         THROW_ASSERT(mux_sel, "Mux selector not yet created");
         SM->add_connection(sel_obj, mux_sel);

         /// specializing allocated mux
         specialise_mux(i.first, bits_tgt);

         auto* mux_object = GetPointer<module>(mux);

         /// adding input connection
         structural_objectRef mux_input = mux_object->get_in_port(in_mux);
         THROW_ASSERT(mux_input, "classic_datapath::mux_allocation - In port does not exist");
         add_datapath_connection(HLS->HLS_T->get_technology_manager(), SM, src, mux_input, conn_type);

         /// adding output signal to the multiplexer
         structural_objectRef port_out_mux = mux_object->get_out_port(0);
         THROW_ASSERT(port_out_mux, "classic_datapath::mux_allocation - Out port does not exist");
         structural_type_descriptorRef out_type = port_out_mux->get_typeRef();
         THROW_ASSERT(out_type->size, "size greater than zero expected");
         std::string sig_name = "out_" + name;
         structural_objectRef sign = SM->add_sign(sig_name, circuit, out_type);
         i.first->set_out_sign(sign);
         SM->add_connection(sign, port_out_mux);
         src = sign;
      }
   }
   add_datapath_connection(HLS->HLS_T->get_technology_manager(), SM, src, tgt, conn_type);
}

void conn_binding::add_datapath_connection(const technology_managerRef TM, const structural_managerRef SM, const structural_objectRef src, const structural_objectRef tgt, unsigned int conn_type)
{
   unsigned int bits_src = GET_TYPE_SIZE(src);
   unsigned int bits_tgt = GET_TYPE_SIZE(tgt);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding datapath connections " + src->get_path() + "(" + STR(bits_src) + " bits)-->" + tgt->get_path() + "(" + STR(bits_tgt) + " bits)");
   // std::cerr << "adding connection between " << src->get_path() << " and " << tgt->get_path() << " conn type " << conn_type << std::endl;
   if(bits_src == bits_tgt)
   {
      THROW_ASSERT(src->get_owner(), "expected an owner for src: " + src->get_path());
      THROW_ASSERT(tgt->get_owner(), "expected an owner for tgt: " + tgt->get_path());
      if(src->get_owner() == tgt->get_owner() && src->get_kind() == port_o_K && tgt->get_kind() == port_o_K)
      {
         std::string name = "io_signal_" + src->get_id() + "_" + tgt->get_id();
         structural_type_descriptorRef sign_type = tgt->get_typeRef();
         structural_objectRef sign = SM->add_sign(name, src->get_owner(), sign_type);
         SM->add_connection(src, sign);
         SM->add_connection(sign, tgt);
      }
      else
         SM->add_connection(src, tgt);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added datapath connections (same bitsize)");
      return;
   }
   structural_objectRef circuit = SM->get_circ();
   bool is_src_int = src->get_typeRef()->type == structural_type_descriptor::INT || (conn_type == structural_type_descriptor::INT && src->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL);
   bool is_tgt_int = tgt->get_typeRef()->type == structural_type_descriptor::INT || (conn_type == structural_type_descriptor::INT && tgt->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL);
   bool is_src_real = src->get_typeRef()->type == structural_type_descriptor::REAL || (conn_type == structural_type_descriptor::REAL && src->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL);
   bool is_tgt_real = tgt->get_typeRef()->type == structural_type_descriptor::REAL || (conn_type == structural_type_descriptor::REAL && tgt->get_typeRef()->type == structural_type_descriptor::VECTOR_BOOL);

   std::string name = "conv_" + src->get_id() + (is_src_int ? "_I" : (is_src_real ? "_R" : "")) + "_" + STR(bits_src) + (is_tgt_int ? "_I" : (is_tgt_real ? "_R" : "")) + "_" + STR(bits_tgt);
   if(converters.find(name) == converters.end())
   {
      // std::cerr << "name: " << name << std::endl;
      // std::cerr << src->get_typeRef()->get_name() << "--" << tgt->get_typeRef()->get_name() << std::endl;
      structural_objectRef c_obj;
      unsigned int offset = 0;
      if(!is_src_int && !is_tgt_int && !is_src_real && !is_tgt_real)
      {
         std::string library_name = TM->get_library(UUDATA_CONVERTER_STD);
         c_obj = SM->add_module_from_technology_library(name, UUDATA_CONVERTER_STD, library_name, circuit, TM);
      }
      else if(!is_src_int && !is_src_real && is_tgt_int)
      {
         std::string library_name = TM->get_library(UIDATA_CONVERTER_STD);
         c_obj = SM->add_module_from_technology_library(name, UIDATA_CONVERTER_STD, library_name, circuit, TM);
      }
      else if(is_src_int && !is_tgt_int && !is_tgt_real)
      {
         std::string library_name = TM->get_library(IUDATA_CONVERTER_STD);
         c_obj = SM->add_module_from_technology_library(name, IUDATA_CONVERTER_STD, library_name, circuit, TM);
      }
      else if(is_src_int && is_tgt_int)
      {
         std::string library_name = TM->get_library(IIDATA_CONVERTER_STD);
         c_obj = SM->add_module_from_technology_library(name, IIDATA_CONVERTER_STD, library_name, circuit, TM);
      }
      else if(is_src_real && is_tgt_real)
      {
         std::string library_name = TM->get_library(UUDATA_CONVERTER_STD);
         c_obj = SM->add_module_from_technology_library(name, UUDATA_CONVERTER_STD, library_name, circuit, TM);
#if 0
         std::string library_name = TM->get_library(FFDATA_CONVERTER_STD);
         c_obj = SM->add_module_from_technology_library(name, FFDATA_CONVERTER_STD, library_name, circuit, TM);
         offset = 2;
#endif
      }
      else
         THROW_UNREACHABLE("Conversion not expected from " + src->get_path() + "(" + src->get_typeRef()->get_name() + ") to " + tgt->get_path() + "(" + tgt->get_typeRef()->get_name() + ")(" + STR(is_src_int) + " " + STR(is_tgt_int) + " " +
                           STR(is_src_real) + " " + STR(is_tgt_real) + " " + STR(bits_src) + " " + STR(bits_tgt) + ")");

      /// fixing input stuff
      structural_objectRef in1 = GetPointer<module>(c_obj)->get_in_port(offset);
      in1->type_resize(bits_src);
      SM->add_connection(src, in1);

      /// fixing output stuff
      structural_objectRef out0 = GetPointer<module>(c_obj)->get_out_port(0);
      out0->type_resize(bits_tgt);
      THROW_ASSERT(out0->get_typeRef()->size, "size greater than one expected");

      structural_objectRef sign = SM->add_sign("out_" + c_obj->get_id(), circuit, out0->get_typeRef());
      SM->add_connection(out0, sign);
      converters[name] = sign;
   }
   SM->add_connection(converters[name], tgt);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added datapath connections");
}

generic_objRef conn_binding::get_constant_obj(const std::string& value, const std::string& param, unsigned int precision)
{
   THROW_ASSERT(value != "", "value expected");
   if(constant_values.find(const_param(value, param)) == constant_values.end())
      constant_values[const_param(value, param)] = generic_objRef(new dataport_obj("CONSTANT_" + value, param, precision));
   return constant_values[const_param(value, param)];
}

std::map<conn_binding::const_param, generic_objRef> conn_binding::get_constant_objs() const
{
   return constant_values;
}

void conn_binding::add_sparse_logic_dp(const hlsRef HLS, const structural_managerRef SM, const HLS_managerRef HLSMgr)
{
   const structural_objectRef circuit = SM->get_circ();
   structural_objectRef sparse_component;
   std::string resource_name, resource_instance_name;
   unsigned int resource_index = 0;
   unsigned int bitsize = 0;
   for(const auto& component : sparse_logic)
   {
      switch(component->get_type())
      {
         case generic_obj::ADDER_CONN_OBJ:
         {
            resource_name = GetPointer<adder_conn_obj>(component)->is_align_adder() ? UI_ALIGN_ADDER_STD : UI_ADDER_STD;
            resource_instance_name = resource_name + "_adder_" + STR(resource_index);
            bitsize = GetPointer<adder_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::MULTIPLIER_CONN_OBJ:
         {
            resource_name = GetPointer<multiplier_conn_obj>(component)->is_multiplication_to_constant() ? UI_CONST_MULTIPLIER_STD : UI_MULTIPLIER_STD;
            resource_instance_name = resource_name + "_multiplier_" + STR(resource_index);
            bitsize = GetPointer<multiplier_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::BITFIELD_OBJ:
         {
            resource_name = SIGNED_BITFIELD_FU_STD;
            resource_instance_name = resource_name + "_" + SIGNED_BITFIELD_FU_STD + "_" + STR(resource_index);
            bitsize = GetPointer<bitfield_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::UU_CONV_CONN_OBJ:
         {
            resource_name = UUDATA_CONVERTER_STD;
            resource_instance_name = resource_name + "_uu_conv_" + STR(resource_index);
            bitsize = GetPointer<uu_conv_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::UI_CONV_CONN_OBJ:
         {
            resource_name = UIDATA_CONVERTER_STD;
            resource_instance_name = resource_name + "_ui_conv_" + STR(resource_index);
            bitsize = GetPointer<ui_conv_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::IU_CONV_CONN_OBJ:
         {
            resource_name = IUDATA_CONVERTER_STD;
            resource_instance_name = resource_name + "_iu_conv_" + STR(resource_index);
            bitsize = GetPointer<iu_conv_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::II_CONV_CONN_OBJ:
         {
            resource_name = IIDATA_CONVERTER_STD;
            resource_instance_name = resource_name + "_ii_conv_" + STR(resource_index);
            bitsize = GetPointer<ii_conv_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::FF_CONV_CONN_OBJ:
         {
            bitsize = GetPointer<ff_conv_conn_obj>(component)->get_bitsize_out();
            if(HLS->Param->isOption(OPT_soft_float) && HLS->Param->getOption<bool>(OPT_soft_float))
            {
               technology_nodeRef current_fu;
               resource_name = AllocationInformation::extract_bambu_provided_name(GetPointer<ff_conv_conn_obj>(component)->get_bitsize_in(), GetPointer<ff_conv_conn_obj>(component)->get_bitsize_out(), HLSMgr, current_fu);
            }
            else
               resource_name = FFDATA_CONVERTER_STD;
            resource_instance_name = resource_name + "_ff_conv_" + STR(resource_index);
            break;
         }
         case generic_obj::I_ASSIGN_CONN_OBJ:
         {
            resource_name = ASSIGN_SIGNED_STD;
            resource_instance_name = resource_name + "_i_assign_" + STR(resource_index);
            bitsize = GetPointer<i_assign_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::U_ASSIGN_CONN_OBJ:
         {
            resource_name = ASSIGN_UNSIGNED_STD;
            resource_instance_name = resource_name + "_u_assign_" + STR(resource_index);
            bitsize = GetPointer<u_assign_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::VB_ASSIGN_CONN_OBJ:
         {
            resource_name = ASSIGN_VECTOR_BOOL_STD;
            resource_instance_name = resource_name + "_vb_assign_" + STR(resource_index);
            bitsize = GetPointer<vb_assign_conn_obj>(component)->get_bitsize();
            break;
         }
         case generic_obj::F_ASSIGN_CONN_OBJ:
         {
            resource_name = ASSIGN_REAL_STD;
            resource_instance_name = resource_name + "_f_assign_" + STR(resource_index);
            bitsize = GetPointer<f_assign_conn_obj>(component)->get_bitsize();
            break;
         }
         default:
         {
            THROW_ERROR("sparse component not yet considered " + component->get_string());
         }
      }
      ++resource_index;
      sparse_component = SM->add_module_from_technology_library(resource_instance_name, resource_name, HLS->HLS_T->get_technology_manager()->get_library(resource_name), circuit, HLS->HLS_T->get_technology_manager());
      component->set_structural_obj(sparse_component);
      auto* sparse_module = GetPointer<module>(sparse_component);

      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Specializing " + sparse_component->get_path() + ": " + STR(bitsize));
      /// specializing sparse module ports
      unsigned int shift_index = 0;
      if(component->get_type() == generic_obj::MULTIPLIER_CONN_OBJ && GetPointer<multiplier_conn_obj>(component)->is_multiplication_to_constant())
      {
         sparse_module->SetParameter(VALUE_PARAMETER, STR(GetPointer<multiplier_conn_obj>(component)->get_constant_value()));
      }
      if(component->get_type() == generic_obj::ADDER_CONN_OBJ && GetPointer<adder_conn_obj>(component)->is_align_adder())
      {
         sparse_module->SetParameter(VALUE_PARAMETER, STR(GetPointer<adder_conn_obj>(component)->get_trimmed_bits()));
      }
      else if(GetPointer<port_o>(sparse_module->get_in_port(shift_index)) && GetPointer<port_o>(sparse_module->get_in_port(shift_index))->get_is_clock())
      {
         /// the multiplier in the resource library is pipelined,
         /// so we use the non-pipelined version by setting PIPE_PARAMETER to 0
         if(component->get_type() == generic_obj::MULTIPLIER_CONN_OBJ)
         {
            sparse_module->SetParameter(PIPE_PARAMETER, "0");
         }
         ++shift_index;
      }
      if(!HLS->Param->isOption(OPT_soft_float) || !HLS->Param->getOption<bool>(OPT_soft_float) || component->get_type() != generic_obj::FF_CONV_CONN_OBJ)
      {
         if(component->get_type() == generic_obj::FF_CONV_CONN_OBJ)
         {
            ++shift_index;
            sparse_module->get_in_port(shift_index)->type_resize(GetPointer<ff_conv_conn_obj>(component)->get_bitsize_in());
         }
         else
            sparse_module->get_in_port(shift_index)->type_resize(bitsize);
      }
      if(component->get_type() != generic_obj::UU_CONV_CONN_OBJ && component->get_type() != generic_obj::UI_CONV_CONN_OBJ && component->get_type() != generic_obj::IU_CONV_CONN_OBJ && component->get_type() != generic_obj::II_CONV_CONN_OBJ &&
         component->get_type() != generic_obj::FF_CONV_CONN_OBJ && component->get_type() != generic_obj::I_ASSIGN_CONN_OBJ && component->get_type() != generic_obj::U_ASSIGN_CONN_OBJ && component->get_type() != generic_obj::VB_ASSIGN_CONN_OBJ &&
         component->get_type() != generic_obj::F_ASSIGN_CONN_OBJ)
         sparse_module->get_in_port(shift_index + 1)->type_resize(bitsize);
      if(!HLS->Param->isOption(OPT_soft_float) || !HLS->Param->getOption<bool>(OPT_soft_float) || component->get_type() != generic_obj::FF_CONV_CONN_OBJ)
         sparse_module->get_out_port(0)->type_resize(bitsize);
   }
}

void conn_binding::print() const
{
   for(const auto& conn : conn_implementation)
   {
      generic_objRef src = std::get<0>(conn.first);
      generic_objRef tgt = std::get<1>(conn.first);
      unsigned int op = std::get<2>(conn.first);
      std::string str = conn.second->get_string();
      if(conn.second->get_type() == connection_obj::BY_MUX)
      {
         str = "MUX_TREE [" + STR(GetPointer<mux_conn>(conn.second)->get_mux_tree_size()) + "] => " + str;
      }
      INDENT_OUT_MEX(OUTPUT_LEVEL_VERY_PEDANTIC, output_level, "---Source: " + src->get_string() + " Target: " + tgt->get_string() + "(" + STR(op) + ") " + " connected by: " + str);
   }
}

unsigned int conn_binding::determine_bit_level_mux() const
{
   CustomOrderedSet<generic_objRef> mux;
   for(const auto& it : conn_implementation)
   {
      if(!GetPointer<mux_conn>(it.second))
         continue;
      const std::vector<std::pair<generic_objRef, unsigned int>>& tree = GetPointer<mux_conn>(it.second)->get_mux_tree();
      for(const auto& v : tree)
         mux.insert(v.first);
   }
   unsigned int bit_mux = 0;
   for(const auto& m : mux)
      bit_mux += GetPointer<mux_obj>(m)->get_bitsize();
   return bit_mux;
}

void conn_binding::add_command_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding command ports");
   structural_objectRef circuit = SM->get_circ();

   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(HLS->functionId);
   const OpGraphConstRef data = FB->CGetOpGraph(FunctionBehavior::DFG);

   /// define the type for boolean command signals
   structural_type_descriptorRef boolean_port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding starting ports");
   const OpVertexSet& operations = HLS->operations;
   for(auto j : operations)
   {
      technology_nodeRef tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(j));
      technology_nodeRef op_tn = GetPointer<functional_unit>(tn)->get_operation(tree_helper::normalized_ID(data->CGetOpNodeInfo(j)->GetOperation()));
      THROW_ASSERT(GetPointer<operation>(op_tn)->time_m, "Time model not available for operation: " + GET_NAME(data, j));
      /// check for start port
      structural_managerRef CM = GetPointer<functional_unit>(tn)->CM;
      if(!CM)
         continue;
      structural_objectRef top = CM->get_circ();
      THROW_ASSERT(top, "expected");
      auto* fu_module = GetPointer<module>(top);
      THROW_ASSERT(fu_module, "expected");
      structural_objectRef start_port_i = fu_module->find_member(START_PORT_NAME, port_o_K, top);
      if((GET_TYPE(data, j) & TYPE_EXTERNAL && start_port_i) || !GetPointer<operation>(op_tn)->is_bounded() || start_port_i)
      {
         bind_selector_port(conn_binding::IN, commandport_obj::UNBOUNDED, j, data);
         bind_selector_port(conn_binding::OUT, commandport_obj::UNBOUNDED, j, data);
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added starting ports");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding inputs");
   std::map<structural_objectRef, std::list<structural_objectRef>> calls;
   std::map<structural_objectRef, std::list<vertex>> start_to_vertex;
   if(selectors.find(conn_binding::IN) != selectors.end())
   {
      auto connection_binding_sets = selectors.find(conn_binding::IN)->second;
      for(std::map<std::pair<generic_objRef, unsigned int>, generic_objRef>::const_iterator j = connection_binding_sets.begin(); j != connection_binding_sets.end(); ++j)
      {
         // unit associate with selector
         const generic_objRef elem = j->first.first;
         // operation's index
         unsigned int oper = j->first.second;
         /// if elem is a functional unit and has more than one operation, add a selector for each operation (e.g. selector_LOAD_1...)
         switch(elem->get_type())
         {
            case generic_obj::FUNCTIONAL_UNIT:
            {
               unsigned int type_fu = GetPointer<funit_obj>(elem)->get_fu();
               std::vector<technology_nodeRef> tmp_ops_node = GetPointer<functional_unit>(HLS->allocation_information->get_fu(type_fu))->get_operations();
               THROW_ASSERT(GetPointer<commandport_obj>(j->second), "Not valid command port");
               structural_objectRef sel_obj = SM->add_port("fuselector_" + elem->get_string() + "_" + tmp_ops_node[oper]->get_name(), port_o::IN, circuit, boolean_port_type);
               (j->second)->set_structural_obj(sel_obj);
               structural_objectRef fu_mod = elem->get_structural_obj();
               THROW_ASSERT(fu_mod, "not correct module");
               structural_objectRef port_selector = fu_mod->find_member("sel_" + tmp_ops_node[oper]->get_name(), port_o_K, fu_mod);
               /// no port selector means that functional units can implement different operations without using a selector (i.e., it has several alias operation)
               if(port_selector)
               {
                  if(port_selector->get_kind() == port_vector_o_K)
                  {
                     THROW_ASSERT(GetPointer<port_o>(port_selector)->get_ports_size() != 0, "port not correctly initialized" + port_selector->get_path());
                     port_selector = GetPointer<port_o>(port_selector)->get_port(GetPointer<funit_obj>(elem)->get_index() % GetPointer<port_o>(port_selector)->get_ports_size());
                  }
                  SM->add_connection(port_selector, sel_obj);
               }
               break;
            }
            case generic_obj::REGISTER:
            {
               structural_objectRef reg_mod = elem->get_structural_obj();

               THROW_ASSERT(GetPointer<commandport_obj>(j->second), "Not valid command port");
               structural_objectRef sel_obj = SM->add_port("wrenable_" + reg_mod->get_id(), port_o::IN, circuit, boolean_port_type);
               (j->second)->set_structural_obj(sel_obj);

               structural_objectRef port_wenable = reg_mod->find_member(WENABLE_PORT_NAME, port_o_K, reg_mod);
               SM->add_connection(port_wenable, sel_obj);
               break;
            }
            case generic_obj::COMMAND_PORT:
            case generic_obj::CONNECTION_ELEMENT:
            {
               THROW_ASSERT(GetPointer<commandport_obj>(j->second), "Not valid command port");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Adding selector_" + elem->get_string() + " " + STR(elem->get_type()));
               structural_objectRef sel_obj = SM->add_port("selector_" + elem->get_string(), port_o::IN, circuit, boolean_port_type);
               (j->second)->set_structural_obj(sel_obj);

               if(GetPointer<commandport_obj>(j->second)->get_command_type() == commandport_obj::UNBOUNDED)
               {
                  vertex op = GetPointer<commandport_obj>(j->second)->get_vertex();
                  generic_objRef fu_unit = HLS->Rfu->get(op);
                  structural_objectRef fu_obj = fu_unit->get_structural_obj();
                  structural_objectRef start = fu_obj->find_member(START_PORT_NAME, port_o_K, fu_obj);
                  THROW_ASSERT(start, fu_obj->get_path());
                  calls[start].push_back(sel_obj);
                  start_to_vertex[start].push_back(op);
               }
               break;
            }
            default:
            {
               THROW_ERROR("Not supported generic_obj " + STR(elem->get_type()));
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added inputs");
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding calls connections");
   for(auto c = calls.begin(); c != calls.end(); ++c)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding connections of " + c->first->get_path());
      auto isMultipleModule = GetPointer<module>(c->first->get_owner()) && GetPointer<module>(c->first->get_owner())->get_multi_unit_multiplicity();
      if(c->second.size() == 1 && !isMultipleModule)
      {
         SM->add_connection(c->first, c->second.front());
      }
      else
      {
         if(isMultipleModule)
         {
            THROW_ASSERT(start_to_vertex.find(c->first) != start_to_vertex.end(), "unexpected condition");
            THROW_ASSERT(c->first->get_kind() == port_vector_o_K, "unexpected condition");
            std::map<structural_objectRef, std::list<structural_objectRef>> toOred;
            auto ports_it = c->second.begin();
            for(auto v : start_to_vertex.find(c->first)->second)
            {
               technology_nodeRef tn = HLS->allocation_information->get_fu(HLS->Rfu->get_assign(v));
               auto index = 0u;
               auto& ops = GetPointer<functional_unit>(tn)->get_operations();
               for(auto o : ops)
               {
                  if(GetPointer<operation>(o)->get_name() == data->CGetOpNodeInfo(v)->GetOperation())
                     break;
                  ++index;
               }
               auto multiplicity = GetPointer<module>(c->first->get_owner())->get_multi_unit_multiplicity();
               index = index % multiplicity;
               THROW_ASSERT(multiplicity == GetPointer<port_o>(c->first)->get_ports_size(), "unexpected condition");
               THROW_ASSERT(index < ops.size(), "unexpected condition");
               auto sp_i = GetPointer<port_o>(c->first)->get_port(index);
               toOred[sp_i].push_back(*ports_it);

               THROW_ASSERT(ports_it != c->second.end(), "unexpected condition");
               ++ports_it;
            }
            for(auto pp_pair : toOred)
            {
               if(pp_pair.second.size() == 1)
                  SM->add_connection(pp_pair.first, pp_pair.second.front());
               else
               {
                  const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
                  std::string library = TM->get_library(OR_GATE_STD);
                  structural_objectRef or_gate = SM->add_module_from_technology_library("or_" + pp_pair.first->get_owner()->get_id() + STR(unique_id), OR_GATE_STD, library, SM->get_circ(), TM);
                  structural_objectRef sig = SM->add_sign("s_" + pp_pair.first->get_owner()->get_id() + STR(unique_id), SM->get_circ(), boolean_port_type);
                  ++unique_id;
                  SM->add_connection(sig, or_gate->find_member("out1", port_o_K, or_gate));
                  SM->add_connection(sig, pp_pair.first);
                  structural_objectRef in = or_gate->find_member("in", port_vector_o_K, or_gate);
                  auto* port = GetPointer<port_o>(in);
                  port->add_n_ports(static_cast<unsigned int>(pp_pair.second.size()), in);
                  unsigned int num = 0;
                  for(auto a = pp_pair.second.begin(); a != pp_pair.second.end(); ++a, ++num)
                  {
                     SM->add_connection(*a, port->get_port(num));
                  }
               }
            }
         }
         else
         {
            const technology_managerRef TM = HLS->HLS_T->get_technology_manager();
            std::string library = TM->get_library(OR_GATE_STD);
            structural_objectRef or_gate = SM->add_module_from_technology_library("or_" + c->first->get_owner()->get_id() + STR(unique_id), OR_GATE_STD, library, SM->get_circ(), TM);
            structural_objectRef sig = SM->add_sign("s_" + c->first->get_owner()->get_id() + STR(unique_id), SM->get_circ(), boolean_port_type);
            ++unique_id;
            SM->add_connection(sig, or_gate->find_member("out1", port_o_K, or_gate));
            SM->add_connection(sig, c->first);
            structural_objectRef in = or_gate->find_member("in", port_vector_o_K, or_gate);
            auto* port = GetPointer<port_o>(in);
            port->add_n_ports(static_cast<unsigned int>(c->second.size()), in);
            unsigned int num = 0;
            for(auto a = c->second.begin(); a != c->second.end(); ++a, ++num)
            {
               SM->add_connection(*a, port->get_port(num));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added connections of " + c->first->get_path());
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added calls connections");
   std::map<structural_objectRef, structural_objectRef> sig;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding multi-unbounded controllers connections");
   for(auto state2mu : HLS->STG->get_mu_ctrls())
   {
      auto mu = state2mu.second;
      structural_objectRef mu_mod = mu->get_structural_obj();
      auto mut = GetPointer<multi_unbounded_obj>(mu);
      const auto& ops = mut->get_ops();
      structural_objectRef inOps = mu_mod->find_member("ops", port_vector_o_K, mu_mod);
      port_o* port = GetPointer<port_o>(inOps);
      auto j = 0u;
      for(const auto& op : ops)
      {
         generic_objRef fu_unit = HLS->Rfu->get(op);
         structural_objectRef fu_obj = fu_unit->get_structural_obj();
         structural_objectRef done = fu_obj->find_member(DONE_PORT_NAME, port_o_K, fu_obj);
         std::string sign_owner_id = done->get_owner()->get_id();
         if(done->get_kind() == port_vector_o_K)
         {
            THROW_ASSERT(GetPointer<port_o>(done)->get_ports_size() != 0, "port not correctly initialized" + done->get_path());
            THROW_ASSERT(GetPointer<funit_obj>(fu_unit), "unexpected port configuration");
            done = GetPointer<port_o>(done)->get_port(GetPointer<funit_obj>(fu_unit)->get_index() % GetPointer<port_o>(done)->get_ports_size());
            sign_owner_id = sign_owner_id + "_P" + done->get_id();
            THROW_ASSERT(done, "Missing done_port from function call " + fu_unit->get_string());
         }
         if(sig.find(done) == sig.end())
         {
            sig[done] = SM->add_sign("s_done_" + sign_owner_id, SM->get_circ(), boolean_port_type);
            SM->add_connection(done, sig[done]);
         }
         THROW_ASSERT(port->get_port(j), "port->get_port(j) not found");
         const auto& port_obj = port->get_port(j);
         SM->add_connection(port_obj, sig[done]);
         ++j;
      }
      HLS->Rconn->bind_selector_port(conn_binding::OUT, commandport_obj::MULTI_UNBOUNDED, mu, 0);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added multi-unbounded controllers connections");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Adding outputs");
   /// output signals to the controller for condition evaluation
   if(selectors.find(conn_binding::OUT) != selectors.end())
   {
      auto connection_binding_sets = selectors.find(conn_binding::OUT)->second;
      for(std::map<std::pair<generic_objRef, unsigned int>, generic_objRef>::const_iterator j = connection_binding_sets.begin(); j != connection_binding_sets.end(); ++j)
      {
         THROW_ASSERT(GetPointer<commandport_obj>(j->second), "Not valid command port");
         if(GetPointer<commandport_obj>(j->second)->get_command_type() == commandport_obj::SWITCH)
         {
            vertex op = GetPointer<commandport_obj>(j->second)->get_vertex();
            unsigned int var_written = HLSMgr->get_produced_value(HLS->functionId, op);
            structural_type_descriptorRef switch_port_type = structural_type_descriptorRef(new structural_type_descriptor(var_written, BH));
            structural_objectRef sel_obj = SM->add_port(GetPointer<commandport_obj>(j->second)->get_string(), port_o::OUT, circuit, switch_port_type);
            (j->second)->set_structural_obj(sel_obj);
         }
         else if(GetPointer<commandport_obj>(j->second)->get_command_type() == commandport_obj::MULTIIF)
         {
            vertex op = GetPointer<commandport_obj>(j->second)->get_vertex();
            std::vector<HLS_manager::io_binding_type> var_read = HLSMgr->get_required_values(HLS->functionId, op);
            auto vect_size = static_cast<unsigned int>(var_read.size());
            structural_type_descriptorRef multiif_port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", vect_size));
            structural_objectRef sel_obj = SM->add_port(GetPointer<commandport_obj>(j->second)->get_string(), port_o::OUT, circuit, multiif_port_type);
            (j->second)->set_structural_obj(sel_obj);
         }
         else if(GetPointer<commandport_obj>(j->second)->get_command_type() == commandport_obj::MULTI_UNBOUNDED)
         {
            structural_objectRef mu_mod = j->second->get_structural_obj();
            auto mu_obj = j->first.first;
            structural_objectRef alldone_command_port = SM->add_port(GetPointer<commandport_obj>(j->second)->get_string(), port_o::OUT, circuit, boolean_port_type);
            THROW_ASSERT(GetPointer<multi_unbounded_obj>(mu_obj), "unexpected condition");
            SM->add_connection(GetPointer<multi_unbounded_obj>(mu_obj)->get_out_sign(), alldone_command_port);
            (j->second)->set_structural_obj(alldone_command_port);
         }
         else
         {
            structural_objectRef sel_obj = SM->add_port(GetPointer<commandport_obj>(j->second)->get_string(), port_o::OUT, circuit, boolean_port_type);
            (j->second)->set_structural_obj(sel_obj);

            if(GetPointer<commandport_obj>(j->second)->get_command_type() == commandport_obj::UNBOUNDED)
            {
               vertex op = GetPointer<commandport_obj>(j->second)->get_vertex();
               generic_objRef fu_unit = HLS->Rfu->get(op);
               structural_objectRef fu_obj = fu_unit->get_structural_obj();
               structural_objectRef done = fu_obj->find_member(DONE_PORT_NAME, port_o_K, fu_obj);
               if(done)
               {
                  std::string sign_owner_id = done->get_owner()->get_id();
                  if(done->get_kind() == port_vector_o_K)
                  {
                     THROW_ASSERT(GetPointer<port_o>(done)->get_ports_size() != 0, "port not correctly initialized" + done->get_path());
                     THROW_ASSERT(GetPointer<funit_obj>(fu_unit), "unexpected port configuration");
                     done = GetPointer<port_o>(done)->get_port(GetPointer<funit_obj>(fu_unit)->get_index() % GetPointer<port_o>(done)->get_ports_size());
                     THROW_ASSERT(done, "Missing done_port from function call " + fu_unit->get_string());
                     sign_owner_id = sign_owner_id + "_P" + done->get_id();
                  }
                  if(sig.find(done) == sig.end())
                  {
                     sig[done] = SM->add_sign("s_done_" + sign_owner_id, SM->get_circ(), boolean_port_type);
                     SM->add_connection(done, sig[done]);
                  }
                  SM->add_connection(sig[done], sel_obj);
               }
            }
         }
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added outputs");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added command ports");
}

conn_binding::ConnectionTarget::ConnectionTarget(generic_objRef tgt, unsigned int tgt_port, unsigned int tgt_port_index) : std::tuple<generic_objRef, unsigned int, unsigned int>(std::make_tuple(tgt, tgt_port, tgt_port_index))
{
}
#if !HAVE_UNORDERED
bool conn_binding::ConnectionTarget::operator<(const ConnectionTarget& other) const
{
   /// Note we compare by using refcount; if they are different, we use name to sort, assuming that they are different
   if((std::get<0>(*this)) != (std::get<0>(other)))
   {
      THROW_ASSERT(std::get<0>(*this)->get_string() != std::get<0>(other)->get_string(), "Found generic object with same name " + std::get<0>(*this)->get_string());
      return std::get<0>(*this)->get_string() < std::get<0>(other)->get_string();
   }
   if(std::get<1>(*this) != std::get<1>(other))
      return std::get<1>(*this) < std::get<1>(other);
   if(std::get<2>(*this) != std::get<2>(other))
      return std::get<2>(*this) < std::get<2>(other);
   return false;
}

bool conn_binding::ConnectionSorter::operator()(const connection& x, const connection& y) const
{
   if(*(std::get<0>(x)) < *(std::get<0>(y)))
      return true;
   if(*(std::get<0>(y)) < *(std::get<0>(x)))
      return false;
   if(*(std::get<1>(x)) < *(std::get<1>(y)))
      return true;
   if(*(std::get<1>(y)) < *(std::get<1>(x)))
      return false;
   if(std::get<2>(x) < std::get<2>(y))
      return true;
   if(std::get<2>(y) < std::get<2>(x))
      return false;
   if(std::get<3>(x) < std::get<3>(y))
      return true;
   if(std::get<3>(y) < std::get<3>(x))
      return false;
   return false;
}
#endif
