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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file classic_datapath.cpp
 * @brief Base class for usual datapath creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "classic_datapath.hpp"

#include "BambuParameter.hpp"
#include "Parameter.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "commandport_obj.hpp"
#include "conn_binding.hpp"
#include "copyrights_strings.hpp"
#include "custom_map.hpp"
#include "dataport_obj.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "function_behavior.hpp"
#include "generic_obj.hpp"
#include "hls.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "mux_conn.hpp"
#include "mux_obj.hpp"
#include "reg_binding.hpp"
#include "schedule.hpp"
#include "state_transition_graph_manager.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include <list>
#include <regex>
#include <string>

classic_datapath::classic_datapath(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr,
                                   unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager,
                                   const HLSFlowStep_Type _hls_flow_step_type)
    : datapath_creator(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

classic_datapath::~classic_datapath() = default;

DesignFlowStep_Status classic_datapath::InternalExec()
{
   /// Test on previous steps. They checks if schedule and connection binding have been performed. If they didn't,
   /// circuit cannot be created.
   THROW_ASSERT(HLS->Rfu, "Functional units not allocated");
   THROW_ASSERT(HLS->Rreg, "Register allocation not performed");
   THROW_ASSERT(HLS->Rconn, "Connection allocation not performed");
   /// Test on memory allocation
   THROW_ASSERT(HLSMgr->Rmem, "Memory allocation not performed");

   /// main circuit type
   const auto FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto fsymbol = FB->CGetBehavioralHelper()->GetMangledFunctionName();
   structural_type_descriptorRef module_type(new structural_type_descriptor("datapath_" + fsymbol));

   /// top circuit creation
   HLS->datapath = structural_managerRef(new structural_manager(HLS->Param));

   HLS->datapath->set_top_info("Datapath_i", module_type);
   const auto datapath_cir = HLS->datapath->get_circ();

   // Now the top circuit is created, just as an empty box. <circuit> is a reference to the structural object that
   // will contain all the circuit components

   datapath_cir->set_black_box(false);

   /// Set some descriptions and legal stuff
   GetPointer<module>(datapath_cir)
       ->set_description("Datapath RTL description for " + FB->CGetBehavioralHelper()->get_function_name());
   GetPointer<module>(datapath_cir)->set_copyright(GENERATED_COPYRIGHT);
   GetPointer<module>(datapath_cir)->set_authors("Component automatically generated by bambu");
   GetPointer<module>(datapath_cir)->set_license(GENERATED_LICENSE);

   /// add clock and reset to circuit. It increments in_port number and update in_port_map
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding clock and reset ports");
   structural_objectRef clock, reset;
   add_clock_reset(clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// add all input ports
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding ports for primary inputs and outputs");
   add_ports();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// add registers, connecting them to clock and reset ports
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding registers");
   HLS->Rreg->add_to_SM(clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   /// allocate functional units
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding functional units");
   HLS->Rfu->add_to_SM(HLSMgr, HLS, clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding multi-unbounded controllers");
   HLS->STG->add_to_SM(clock, reset);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");

   const auto func_arch = HLSMgr->module_arch->GetArchitecture(fsymbol);
   const auto is_dataflow_top =
       func_arch && func_arch->attrs.find(FunctionArchitecture::func_dataflow_top) != func_arch->attrs.end() &&
       func_arch->attrs.find(FunctionArchitecture::func_dataflow_top)->second == "1";
   if(is_dataflow_top)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding dataflow module interfaces");
      const auto CGM = HLSMgr->CGetCallGraphManager();
      const auto dataflow_module_ids = CGM->get_called_by(funId);
      const auto funIdStr = std::to_string(funId);
      std::map<std::string, const FunctionArchitecture::iface_attrs*> iface_attrs;
      std::map<std::string, std::vector<structural_objectRef>> iface_ports;
      const auto manage_port = [&](const structural_objectRef& port) {
         const auto port_name = port->get_id();
         const std::regex regx(R"(_(DF_bambu_(\d+)_\d+FO\d+)_.*)");
         std::cmatch what;
         if(std::regex_search(port_name.data(), what, regx))
         {
            const auto owner_id = what[2].str();
            if(funIdStr == owner_id)
            {
               const auto bundle_id = what[1].str();
               iface_ports[bundle_id].push_back(port);
            }
         }
      };
      // Gather dataflow module ports grouped by interface bundle
      for(unsigned int n = 0; n < GetPointer<module>(datapath_cir)->get_internal_objects_size(); ++n)
      {
         const auto member = GetPointer<module>(datapath_cir)->get_internal_object(n);
         const auto mod_id = GET_TYPE_NAME(member);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Analyze module " + mod_id);
         const auto msymbol = (mod_id.size() && mod_id.front() == '_') ? mod_id.substr(1) : mod_id;
         const auto march = HLSMgr->module_arch->GetArchitecture(msymbol);
         THROW_ASSERT(march || !HLSMgr->get_tree_manager()->GetFunction(msymbol),
                      "Expected function architecture for function " + msymbol);
         if(march)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "---Dataflow module");
            for(const auto& [bundle, attrs] : march->ifaces)
            {
               iface_attrs.emplace(bundle, &attrs);
            }
            for(unsigned int p = 0; p < GetPointer<module>(member)->get_in_port_size(); ++p)
            {
               manage_port(GetPointer<module>(member)->get_in_port(p));
            }
            for(unsigned int p = 0; p < GetPointer<module>(member)->get_out_port_size(); ++p)
            {
               manage_port(GetPointer<module>(member)->get_out_port(p));
            }
            for(unsigned int p = 0; p < GetPointer<module>(member)->get_in_out_port_size(); ++p)
            {
               manage_port(GetPointer<module>(member)->get_in_out_port(p));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
      }
      for(auto& [iface, ports] : iface_ports)
      {
         const auto attrs = iface_attrs.at(iface);
         structural_objectRef dataflow_if;
         const auto& iface_mode = attrs->at(FunctionArchitecture::iface_mode);
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding dataflow " + iface_mode + " " + iface);
         if(iface_mode == "fifo")
         {
            dataflow_if = HLS->datapath->add_module_from_technology_library(
                iface, "dataflow_fifo", LIBRARY_STD_DATAFLOW, datapath_cir, HLS->HLS_D->get_technology_manager());
         }
         else
         {
            THROW_ERROR("Dataflow interface not supported: " + iface_mode);
         }
         const auto if_clock = GetPointer<module>(dataflow_if)->find_member("clock", port_o_K, dataflow_if);
         const auto if_reset = GetPointer<module>(dataflow_if)->find_member("reset", port_o_K, dataflow_if);
         HLS->datapath->add_connection(clock, if_clock);
         HLS->datapath->add_connection(reset, if_reset);
         for(auto& port : ports)
         {
            const auto port_name = port->get_id();
            const auto port_suffix = port_name.substr(iface.size() + 2);
            const auto if_port = GetPointer<module>(dataflow_if)->find_member(port_suffix, port_o_K, dataflow_if);
            THROW_ASSERT(if_port, "Expected port " + port_suffix + " in dataflow interface module " +
                                      GET_TYPE_NAME(dataflow_if));
            // port_o::fix_port_properties(port, if_port);
            structural_objectRef if_sign;
            if(if_port->get_kind() == port_vector_o_K)
            {
               port_o::resize_std_port(GetPointerS<port_o>(port)->get_ports_size() * STD_GET_SIZE(port->get_typeRef()),
                                       0U, DEBUG_LEVEL_NONE, if_port);
               if_sign = HLS->datapath->add_sign_vector(iface + "_" + port_suffix,
                                                        GetPointerS<port_o>(port)->get_ports_size(), datapath_cir,
                                                        port->get_typeRef());
            }
            else
            {
               port_o::resize_std_port(STD_GET_SIZE(port->get_typeRef()), 0U, DEBUG_LEVEL_NONE, if_port);
               if_sign = HLS->datapath->add_sign(iface + "_" + port_suffix, datapath_cir, port->get_typeRef());
            }
            HLS->datapath->add_connection(port, if_sign);
            HLS->datapath->add_connection(if_sign, if_port);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
   }

   /// allocate interconnections
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Adding interconnections");
   HLS->Rconn->add_to_SM(HLSMgr, HLS, HLS->datapath);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--");
   unsigned int n_elements = GetPointer<module>(datapath_cir)->get_internal_objects_size();
   if(n_elements == 0)
   {
      structural_objectRef dummy_gate = HLS->datapath->add_module_from_technology_library(
          "dummy_REG", flipflop_SR, LIBRARY_STD, datapath_cir, HLS->HLS_D->get_technology_manager());
      structural_objectRef port_ck = dummy_gate->find_member(CLOCK_PORT_NAME, port_o_K, dummy_gate);
      if(port_ck)
      {
         HLS->datapath->add_connection(clock, port_ck);
      }
      structural_objectRef port_rst = dummy_gate->find_member(RESET_PORT_NAME, port_o_K, dummy_gate);
      if(port_rst)
      {
         HLS->datapath->add_connection(reset, port_rst);
      }
   }
   /// circuit is now complete. circuit manager can be initialized and dot representation can be created
   HLS->datapath->INIT(true);
   if(parameters->getOption<bool>(OPT_print_dot))
   {
      HLS->datapath->WriteDot(FB->CGetBehavioralHelper()->get_function_name() + "/HLS_Datapath.dot",
                              structural_manager::COMPLETE_G);
   }
   return DesignFlowStep_Status::SUCCESS;
}

void classic_datapath::add_clock_reset(structural_objectRef& clock_obj, structural_objectRef& reset_obj)
{
   const auto& SM = this->HLS->datapath;
   const auto circuit = SM->get_circ();

   /// define boolean type for clock and reset signal
   structural_type_descriptorRef port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * Start adding clock signal...");
   /// add clock port
   clock_obj = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, port_type);
   GetPointer<port_o>(clock_obj)->set_is_clock(true);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    Clock signal added!");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * Start adding reset signal...");
   /// add reset port
   reset_obj = SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, port_type);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "    Reset signal added!");

   return;
}

void classic_datapath::add_ports()
{
   bool need_start_done = false;
   const structural_managerRef SM = this->HLS->datapath;
   const structural_objectRef circuit = SM->get_circ();
   const FunctionBehaviorConstRef FB = HLSMgr->CGetFunctionBehavior(funId);
   const auto curr_address_bitsize = HLSMgr->get_address_bitsize();
   const BehavioralHelperConstRef BH = FB->CGetBehavioralHelper();

   const std::list<unsigned int>& function_parameters = BH->get_parameters();
   for(auto const function_parameter : function_parameters)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "-->Adding port for parameter: " + BH->PrintVariable(function_parameter) + " IN");
      generic_objRef port_obj;
      if(HLS->Rconn)
      {
         conn_binding::direction_type direction = conn_binding::IN;
         port_obj = HLS->Rconn->get_port(function_parameter, direction);
      }
      structural_type_descriptorRef port_type;
      if(HLSMgr->Rmem->has_base_address(function_parameter) &&
         !HLSMgr->Rmem->has_parameter_base_address(function_parameter, HLS->functionId) &&
         !HLSMgr->Rmem->is_parm_decl_stored(function_parameter))
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor("bool", curr_address_bitsize));
      }
      else
      {
         port_type = structural_type_descriptorRef(new structural_type_descriptor(function_parameter, BH));
      }
      if(HLSMgr->Rmem->has_base_address(function_parameter) && (HLSMgr->Rmem->is_parm_decl_stored(function_parameter) ||
                                                                HLSMgr->Rmem->is_parm_decl_copied(function_parameter)))
      {
         need_start_done = true;
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---type is: " + port_type->get_name());
      std::string prefix = "in_port_";
      port_o::port_direction port_direction = port_o::IN;
      structural_objectRef p_obj =
          SM->add_port(prefix + BH->PrintVariable(function_parameter), port_direction, circuit, port_type);
      if(HLS->Rconn)
      {
         port_obj->set_structural_obj(p_obj);
         port_obj->set_out_sign(p_obj);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Added");
   }
   if(HLS->Rconn)
   {
      unsigned int num = 0;
      for(auto& c : HLS->Rconn->get_constant_objs())
      {
         generic_objRef constant_obj = c.second;
         structural_objectRef const_obj = SM->add_module_from_technology_library(
             "const_" + STR(num), CONSTANT_STD, LIBRARY_STD, circuit, HLS->HLS_D->get_technology_manager());

         std::string value = std::get<0>(c.first);
         std::string param = std::get<1>(c.first);
         std::string trimmed_value;
         unsigned long long precision;
         if(param.size() == 0)
         {
            trimmed_value = "\"" + std::get<0>(c.first) + "\"";
            precision = static_cast<unsigned int>(value.size());
         }
         else
         {
            trimmed_value = param;
            memory::add_memory_parameter(SM, param, std::get<0>(c.first));
            precision = GetPointer<dataport_obj>(constant_obj)->get_bitsize();
         }
         const_obj->SetParameter("value", trimmed_value);
         constant_obj->set_structural_obj(const_obj);
         std::string name = "out_const_" + std::to_string(num);
         structural_type_descriptorRef sign_type =
             structural_type_descriptorRef(new structural_type_descriptor("bool", precision));
         structural_objectRef sign = SM->add_sign(name, circuit, sign_type);
         structural_objectRef out_port = const_obj->find_member("out1", port_o_K, const_obj);
         // customize output port size
         out_port->type_resize(precision);
         SM->add_connection(sign, out_port);
         constant_obj->set_out_sign(sign);
         num++;
      }
   }
   const unsigned int return_type_index = BH->GetFunctionReturnType(BH->get_function_index());
   if(return_type_index)
   {
      PRINT_DBG_STRING(DEBUG_LEVEL_PEDANTIC, debug_level, "Return type: " + BH->print_type(return_type_index));

      generic_objRef port_obj;
      if(HLS->Rconn)
      {
         port_obj = HLS->Rconn->get_port(return_type_index, conn_binding::OUT);
      }
      structural_type_descriptorRef port_type =
          structural_type_descriptorRef(new structural_type_descriptor(return_type_index, BH));
      structural_objectRef p_obj = SM->add_port(RETURN_PORT_NAME, port_o::OUT, circuit, port_type);
      if(HLS->Rconn)
      {
         port_obj->set_structural_obj(p_obj);
      }
   }
   /// add start and done when needed
   if(need_start_done)
   {
      structural_type_descriptorRef bool_type =
          structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      SM->add_port(START_PORT_NAME, port_o::IN, circuit, bool_type);
      SM->add_port(DONE_PORT_NAME, port_o::OUT, circuit, bool_type);
   }
}
