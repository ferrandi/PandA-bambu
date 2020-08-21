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
 * @file RTL_characterization.cpp
 * @brief Class implementing RTL characterization
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Autoheader includes
#include "config_HAVE_FLOPOCO.hpp"

/// circuit include
#include "structural_objects.hpp"

/// design_flows include
#include "design_flow_graph.hpp"
#include "design_flow_manager.hpp"

/// design_flows/technology include
#include "technology_flow_step.hpp"
#include "technology_flow_step_factory.hpp"

/// HLS/module_allocation include
#include "allocation_information.hpp"

/// STD include
#include <string>

/// STL includes
#include <algorithm>
#include <list>

/// technology include
#include "parse_technology.hpp"

/// technology/physical_library
#include "technology_node.hpp"

#include "RTL_characterization.hpp"

#include "library_manager.hpp"
#include "technology_manager.hpp"

#include "LUT_model.hpp"
#include "NP_functionality.hpp"
#include "area_model.hpp"
#include "clb_model.hpp"
#include "structural_manager.hpp"
#include "target_device.hpp"
#include "target_manager.hpp"
#include "time_model.hpp"

#include "BackendFlow.hpp"

#include "HDL_manager.hpp"
#include "language_writer.hpp"

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "op_graph.hpp"
#include "polixml.hpp"
#include "xml_helper.hpp"

#if HAVE_FLOPOCO
#include "flopoco_wrapper.hpp"
#endif
#include "string_manipulation.hpp" // for GET_CLASS

#define PORT_VECTOR_N_PORTS 2

RTLCharacterization::RTLCharacterization(const target_managerRef _target, const std::string& _cells, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters),
      FunctionalUnitStep(_target, _design_flow_manager, _parameters),
      component(ComputeComponent(_cells)),
      cells(ComputeCells(_cells))
#ifndef NDEBUG
      ,
      dummy_synthesis(_parameters->IsParameter("dummy_synthesis") and _parameters->GetParameter<std::string>("dummy_synthesis") == "yes")
#endif
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

RTLCharacterization::~RTLCharacterization() = default;

void RTLCharacterization::Initialize()
{
   FunctionalUnitStep::Initialize();
   LM = TM->get_library_manager(TM->get_library(component));
   prev_area_characterization = area_modelRef();
   prev_timing_characterization = time_modelRef();
}

DesignFlowStep_Status RTLCharacterization::Exec()
{
   const target_deviceRef device = target->get_target_device();

   const auto functional_unit = LM->get_fu(component);
   AnalyzeFu(functional_unit);
   // fix_execution_time_std();
   fix_proxies_execution_time_std();
   // if(is_xilinx) fix_muxes();
   xwrite_device_file(device);
   return DesignFlowStep_Status::SUCCESS;
}

void RTLCharacterization::fix_muxes()
{
   technology_nodeRef f_unit_br = TM->get_fu(ASSIGN_VECTOR_BOOL_STD, LIBRARY_STD_FU);
   auto* fu_br = GetPointer<functional_unit>(f_unit_br);
   technology_nodeRef op_br_node = fu_br->get_operation(ASSIGN);
   auto* op_ASSIGN = GetPointer<operation>(op_br_node);
   double ASSIGN_exec_time = op_ASSIGN->time_m->get_execution_time();
   std::vector<unsigned int> bitsizes;
   bitsizes.push_back(8);
   bitsizes.push_back(16);
   bitsizes.push_back(32);
   bitsizes.push_back(64);
   const std::vector<unsigned int>::const_iterator b_it_end = bitsizes.end();
   for(std::vector<unsigned int>::const_iterator b_it = bitsizes.begin(); b_it != b_it_end; ++b_it)
   {
      std::string test_mul_mux_name =
          std::string(TEST_MUL_MUX_8) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it);
      technology_nodeRef f_unit_test = TM->get_fu(test_mul_mux_name, LIBRARY_STD_FU);
      if(!f_unit_test)
         continue;
      auto* fu_test = GetPointer<functional_unit>(f_unit_test);
      technology_nodeRef op_test_node = fu_test->get_operation(TEST_MUL_MUX_8);
      auto* op_test = GetPointer<operation>(op_test_node);
      double test_exec_time = op_test->time_m->get_execution_time();

      technology_nodeRef f_unit_mult = TM->get_fu(std::string(MULTIPLIER_STD) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it) + "_0", LIBRARY_STD_FU);
      auto* fu_mult = GetPointer<functional_unit>(f_unit_mult);
      technology_nodeRef op_mult_node = fu_mult->get_operation("mult_expr");
      auto* op_mult = GetPointer<operation>(op_mult_node);
      double mult_exec_time = op_mult->time_m->get_execution_time();

      technology_nodeRef f_unit_mux = TM->get_fu(std::string(MUX_GATE_STD) + "_1_" + STR(*b_it) + "_" + STR(*b_it) + "_" + STR(*b_it), LIBRARY_STD_FU);
      auto* fu_mux = GetPointer<functional_unit>(f_unit_mux);
      const functional_unit::operation_vec& ops = fu_mux->get_operations();
      for(const auto& op : ops)
      {
         auto* current_op = GetPointer<operation>(op);
         if(!current_op->time_m)
            continue;
         unsigned int curr_cycles = current_op->time_m->get_cycles();
         double curr_exec = current_op->time_m->get_execution_time();
         double new_exec_time = (test_exec_time - mult_exec_time) / 3 + ASSIGN_exec_time;
         if(new_exec_time > curr_exec)
            current_op->time_m->set_execution_time(new_exec_time, curr_cycles);
      }
   }
}

void RTLCharacterization::fix_execution_time_std()
{
   technology_nodeRef f_unit_br = TM->get_fu(ASSIGN_VECTOR_BOOL_STD, LIBRARY_STD_FU);
   auto* fu_br = GetPointer<functional_unit>(f_unit_br);
   technology_nodeRef op_br_node = fu_br->get_operation(ASSIGN);
   auto* op_br_LOAD = GetPointer<operation>(op_br_node);
   double ASSIGN_exec_time = op_br_LOAD->time_m->get_execution_time();
   if(LM->get_library_name() != LIBRARY_STD_FU && LM->get_library_name() != LIBRARY_PC && LM->get_library_name() != LIBRARY_STD)
      return;
   const auto tn = LM->get_fu(component);
   auto* current_fu = GetPointer<functional_unit>(tn);
   if(current_fu)
   {
      if((!parameters->isOption(OPT_component_name) || current_fu->get_operations_num() == 0) && completed.find(current_fu->functional_unit_name) == completed.end())
         return;
      const functional_unit::operation_vec& ops = current_fu->get_operations();
      for(const auto& op : ops)
      {
         auto* current_op = GetPointer<operation>(op);
         if(!current_op->time_m)
            continue;
         double curr_exec = current_op->time_m->get_execution_time();
         unsigned int curr_cycles = current_op->time_m->get_cycles();
         if(curr_exec - ASSIGN_exec_time < 0)
            current_op->time_m->set_execution_time(0, curr_cycles);
         else
            current_op->time_m->set_execution_time(curr_exec - ASSIGN_exec_time, curr_cycles);
      }
   }
}

void RTLCharacterization::fix_proxies_execution_time_std()
{
   std::vector<std::string> high_latency_postfix_list;
   high_latency_postfix_list.push_back(std::string("_3"));
   high_latency_postfix_list.push_back(std::string("_4"));
   for(auto high_latency_postfix : high_latency_postfix_list)
   {
      technology_nodeRef f_unit_br = TM->get_fu(ARRAY_1D_STD_BRAM, LIBRARY_STD_FU);
      auto* fu_br = GetPointer<functional_unit>(f_unit_br);
      technology_nodeRef op_br_node = fu_br->get_operation("STORE");
      auto* op_br_STORE = GetPointer<operation>(op_br_node);
      double STORE_exec_time = op_br_STORE->time_m->get_execution_time();
      technology_nodeRef f_unit_br_nn = TM->get_fu(ARRAY_1D_STD_BRAM_NN, LIBRARY_STD_FU);
      auto* fu_br_nn = GetPointer<functional_unit>(f_unit_br_nn);
      technology_nodeRef op_br_nn_node = fu_br_nn->get_operation("STORE");
      auto* op_br_nn_STORE = GetPointer<operation>(op_br_nn_node);
      double nn_STORE_exec_time = op_br_nn_STORE->time_m->get_execution_time();

      technology_nodeRef f_unit_br_hl = TM->get_fu(ARRAY_1D_STD_BRAM + high_latency_postfix, LIBRARY_STD_FU);
      auto* fu_br_hl = GetPointer<functional_unit>(f_unit_br_hl);
      technology_nodeRef op_br_node_hl = fu_br_hl->get_operation("STORE");
      auto* op_br_STORE_hl = GetPointer<operation>(op_br_node_hl);
      double STORE_exec_time_hl = op_br_STORE_hl->time_m->get_execution_time();
      technology_nodeRef f_unit_br_nn_hl = TM->get_fu(ARRAY_1D_STD_BRAM_NN + high_latency_postfix, LIBRARY_STD_FU);
      auto* fu_br_nn_hl = GetPointer<functional_unit>(f_unit_br_nn_hl);
      technology_nodeRef op_br_nn_node_hl = fu_br_nn_hl->get_operation("STORE");
      auto* op_br_nn_STORE_hl = GetPointer<operation>(op_br_nn_node_hl);
      double nn_STORE_exec_time_hl = op_br_nn_STORE_hl->time_m->get_execution_time();

      if(LM->get_library_name() != LIBRARY_STD_FU && LM->get_library_name() != LIBRARY_PC && LM->get_library_name() != LIBRARY_STD)
         return;
      const auto tn = LM->get_fu(component);
      auto* current_fu = GetPointer<functional_unit>(tn);
      if(!current_fu)
         return;
      std::string fu_name = current_fu->functional_unit_name;
      std::string memory_ctrl_type = current_fu->memory_ctrl_type;
      std::string bram_load_latency = current_fu->bram_load_latency;
      if(memory_ctrl_type == MEMORY_CTRL_TYPE_PROXY || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXY || fu_name == BMEMORY_STD || fu_name == BMEMORY_STD + (bram_load_latency != "2" ? high_latency_postfix : ""))
      {
         if((!parameters->isOption(OPT_component_name) || current_fu->get_operations_num() == 0) && completed.find(fu_name) == completed.end())
            return;
         const functional_unit::operation_vec& ops = current_fu->get_operations();
         for(const auto& op : ops)
         {
            auto* current_op = GetPointer<operation>(op);
            if(!current_op->time_m)
               continue;
            unsigned int curr_cycles = current_op->time_m->get_cycles();
            if(bram_load_latency == "2")
            {
               if(curr_cycles > 0)
                  current_op->time_m->set_stage_period(STORE_exec_time);
               else
                  current_op->time_m->set_execution_time(STORE_exec_time, curr_cycles);
            }
            else
            {
               if(curr_cycles > 0)
               {
                  current_op->time_m->set_stage_period(STORE_exec_time_hl);
               }
               else
                  current_op->time_m->set_execution_time(STORE_exec_time_hl, curr_cycles);
            }
         }
      }
      if(memory_ctrl_type == MEMORY_CTRL_TYPE_PROXYN || memory_ctrl_type == MEMORY_CTRL_TYPE_DPROXYN || fu_name == BMEMORY_STDN || fu_name == BMEMORY_STDN + (bram_load_latency != "2" ? high_latency_postfix : ""))
      {
         if((!parameters->isOption(OPT_component_name) || current_fu->get_operations_num() == 0) && completed.find(fu_name) == completed.end())
            return;
         const functional_unit::operation_vec& ops = current_fu->get_operations();
         for(const auto& op : ops)
         {
            auto* current_op = GetPointer<operation>(op);
            if(!current_op->time_m)
               continue;
            unsigned int curr_cycles = current_op->time_m->get_cycles();
            if(bram_load_latency == "2")
            {
               if(curr_cycles > 0)
                  current_op->time_m->set_stage_period(nn_STORE_exec_time);
               else
                  current_op->time_m->set_execution_time(nn_STORE_exec_time, curr_cycles);
            }
            else
            {
               if(curr_cycles > 0)
                  current_op->time_m->set_stage_period(nn_STORE_exec_time_hl);
               else
                  current_op->time_m->set_execution_time(nn_STORE_exec_time_hl, curr_cycles);
            }
         }
      }
   }
}

void RTLCharacterization::xwrite_device_file(const target_deviceRef device)
{
   std::string file_name = "characterization.xml";
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("target");

      device->xwrite(nodeRoot);

      xwrite_characterization(device, nodeRoot);

      document.write_to_file_formatted(file_name);
   }
   catch(const char* msg)
   {
      THROW_ERROR(std::string(msg));
   }
   catch(const std::string& msg)
   {
      THROW_ERROR(msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Exception caught: " + std::string(ex.what()));
   }
   catch(...)
   {
      THROW_ERROR("unknown exception");
   }
}

void RTLCharacterization::xwrite_characterization(const target_deviceRef device, xml_element* nodeRoot)
{
   xml_element* tmRoot = nodeRoot->add_child_element("technology");

   xml_element* lmRoot = tmRoot->add_child_element("library");
   xml_element* name_el = lmRoot->add_child_element("name");
   name_el->add_child_text(LM->get_library_name());

   const library_manager::fu_map_type& fus = LM->get_library_fu();
   for(const auto& cell : cells)
   {
      if(fus.find(cell) == fus.end())
      {
         THROW_ERROR(cell + " is not in any technology library");
      }
      technology_nodeRef tn = fus.find(cell)->second;
      auto* current_fu = GetPointer<functional_unit>(tn);
      if(!current_fu)
      {
         auto* current_fu_temp = GetPointer<functional_unit_template>(tn);
         if(current_fu_temp->specialized != "")
         {
            xml_element* template_el = lmRoot->add_child_element("template");
            current_fu_temp->xwrite(template_el, tn, parameters, device->get_type());
         }
      }
      else
      {
         if((!parameters->isOption(OPT_component_name) || current_fu->get_operations_num() == 0) && completed.find(current_fu->functional_unit_name) == completed.end())
            THROW_ERROR("");

         if(!current_fu->area_m)
         {
            /// set to the default value
            current_fu->area_m = area_model::create_model(device->get_type(), parameters);
         }

         xml_element* cell_el = lmRoot->add_child_element("cell");
         xml_element* cell_name_el = cell_el->add_child_element("name");
         cell_name_el->add_child_text(current_fu->get_name());

         xml_element* attribute_el = cell_el->add_child_element("attribute");
         WRITE_XNVM2("name", "area", attribute_el);
         WRITE_XNVM2("value_type", "float64", attribute_el);
         attribute_el->add_child_text(STR(current_fu->area_m->get_area_value()));
         auto* clb = GetPointer<clb_model>(current_fu->area_m);
         if(clb && clb->get_resource_value(clb_model::REGISTERS) != 0)
         {
            attribute_el = cell_el->add_child_element("attribute");
            WRITE_XNVM2("name", "REGISTERS", attribute_el);
            WRITE_XNVM2("value_type", "float64", attribute_el);
            attribute_el->add_child_text(STR(clb->get_resource_value(clb_model::REGISTERS)));
         }
         if(clb && clb->get_resource_value(clb_model::SLICE_LUTS) != 0)
         {
            attribute_el = cell_el->add_child_element("attribute");
            WRITE_XNVM2("name", "SLICE_LUTS", attribute_el);
            WRITE_XNVM2("value_type", "float64", attribute_el);
            attribute_el->add_child_text(STR(clb->get_resource_value(clb_model::SLICE_LUTS)));
         }
         if(clb && clb->get_resource_value(clb_model::SLICE) != 0)
         {
            attribute_el = cell_el->add_child_element("attribute");
            WRITE_XNVM2("name", "SLICE", attribute_el);
            WRITE_XNVM2("value_type", "float64", attribute_el);
            attribute_el->add_child_text(STR(clb->get_resource_value(clb_model::SLICE)));
         }
         if(clb && clb->get_resource_value(clb_model::LUT_FF_PAIRS) != 0)
         {
            attribute_el = cell_el->add_child_element("attribute");
            WRITE_XNVM2("name", "LUT_FF_PAIRS", attribute_el);
            WRITE_XNVM2("value_type", "float64", attribute_el);
            attribute_el->add_child_text(STR(clb->get_resource_value(clb_model::LUT_FF_PAIRS)));
         }
         if(clb && clb->get_resource_value(clb_model::DSP) != 0)
         {
            attribute_el = cell_el->add_child_element("attribute");
            WRITE_XNVM2("name", "DSP", attribute_el);
            WRITE_XNVM2("value_type", "float64", attribute_el);
            attribute_el->add_child_text(STR(clb->get_resource_value(clb_model::DSP)));
         }
         if(clb && clb->get_resource_value(clb_model::BRAM) != 0)
         {
            attribute_el = cell_el->add_child_element("attribute");
            WRITE_XNVM2("name", "BRAM", attribute_el);
            WRITE_XNVM2("value_type", "float64", attribute_el);
            attribute_el->add_child_text(STR(clb->get_resource_value(clb_model::BRAM)));
         }

         if(current_fu->fu_template_name != "" && current_fu->fu_template_parameters != "")
         {
            xml_element* template_el = cell_el->add_child_element("template");
            WRITE_XNVM2("name", current_fu->fu_template_name, template_el);
            WRITE_XNVM2("parameter", current_fu->fu_template_parameters, template_el);

            if(current_fu->characterizing_constant_value != "")
            {
               xml_element* constant_el = cell_el->add_child_element(GET_CLASS_NAME(characterizing_constant_value));
               constant_el->add_child_text(STR(current_fu->characterizing_constant_value));
            }
         }
         auto characterization_timestamp_el = cell_el->add_child_element("characterization_timestamp");
         characterization_timestamp_el->add_child_text(STR(TimeStamp::GetCurrentTimeStamp()));
         const functional_unit::operation_vec& ops = current_fu->get_operations();
         for(const auto& op : ops)
         {
            auto* current_op = GetPointer<operation>(op);
            current_op->xwrite(cell_el, op, parameters, device->get_type());
         }
         if(current_fu->CM && current_fu->CM->get_circ() && GetPointer<module>(current_fu->CM->get_circ()) && GetPointer<module>(current_fu->CM->get_circ())->get_specialized() != "")
         {
            if(current_fu->memory_type != "")
            {
               xml_element* item_el = cell_el->add_child_element(GET_CLASS_NAME(memory_type));
               item_el->add_child_text(STR(current_fu->memory_type));
            }
            if(current_fu->channels_type != "")
            {
               xml_element* item_el = cell_el->add_child_element(GET_CLASS_NAME(channels_type));
               item_el->add_child_text(STR(current_fu->channels_type));
            }
            if(current_fu->memory_ctrl_type != "")
            {
               xml_element* item_el = cell_el->add_child_element(GET_CLASS_NAME(memory_ctrl_type));
               item_el->add_child_text(STR(current_fu->memory_ctrl_type));
            }
            if(current_fu->bram_load_latency != "")
            {
               xml_element* item_el = cell_el->add_child_element(GET_CLASS_NAME(bram_load_latency));
               item_el->add_child_text(STR(current_fu->bram_load_latency));
            }
            current_fu->CM->xwrite(cell_el);
         }
      }
   }
}

void RTLCharacterization::resize_port(const structural_objectRef& port, unsigned int prec)
{
   if(port->get_typeRef()->type == structural_type_descriptor::BOOL)
      return;
   if(GetPointer<port_o>(port)->get_is_doubled())
   {
      if(port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_REAL)
         port_o::resize_std_port(2 * prec, 128 / (2 * prec), 0, port);
      else
         port_o::resize_std_port(2 * prec, 0, 0, port);
   }
   else if(GetPointer<port_o>(port)->get_is_halved())
   {
      if(port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_REAL)
         port_o::resize_std_port(prec / 2, 128 / (prec / 2), 0, port);
      else
         port_o::resize_std_port(prec / 2, 0, 0, port);
   }
   else if(port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_REAL)
      port_o::resize_std_port(prec, 128 / prec, 0, port);
   else
      port_o::resize_std_port(prec, 0, 0, port);
}

void RTLCharacterization::specialize_fu(const module* mod, unsigned int prec, unsigned int bus_data_bitsize, unsigned int bus_addr_bitsize, unsigned int bus_size_bitsize, unsigned int bus_tag_bitsize, size_t portsize_value)
{
   for(unsigned int i = 0; i < mod->get_in_port_size(); i++)
   {
      const structural_objectRef& port = mod->get_in_port(i);
      if(port->get_kind() == port_vector_o_K)
      {
         if(GetPointer<port_o>(port)->get_ports_size() == 0)
            GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(portsize_value), port);
         if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
            port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         else
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(port)->get_ports_size(); ++p)
               resize_port(GetPointer<port_o>(port)->get_port(p), prec);
         }
      }
      else
      {
         if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
            port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         else
            resize_port(port, prec);
      }
   }
   for(unsigned int i = 0; i < mod->get_out_port_size(); i++)
   {
      const structural_objectRef& port = mod->get_out_port(i);
      if(port->get_kind() == port_vector_o_K)
      {
         if(GetPointer<port_o>(port)->get_ports_size() == 0)
            GetPointer<port_o>(port)->add_n_ports(static_cast<unsigned int>(portsize_value), port);
         if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
            port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         else
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(port)->get_ports_size(); ++p)
               resize_port(GetPointer<port_o>(port)->get_port(p), prec);
         }
      }
      else
      {
         if(GetPointer<port_o>(port)->get_is_data_bus() || GetPointer<port_o>(port)->get_is_addr_bus() || GetPointer<port_o>(port)->get_is_size_bus() || GetPointer<port_o>(port)->get_is_tag_bus())
            port_o::resize_busport(bus_size_bitsize, bus_addr_bitsize, bus_data_bitsize, bus_tag_bitsize, port);
         else
            resize_port(port, prec);
      }
   }
}

void RTLCharacterization::add_input_register(structural_objectRef port_in, const std::string& register_library, const std::string& port_prefix, structural_objectRef reset_port, structural_objectRef circuit, structural_objectRef clock_port,
                                             structural_objectRef e_port, structural_managerRef SM)
{
   structural_objectRef r_signal;
   structural_objectRef reg_mod;
   if(port_in->get_typeRef()->type == structural_type_descriptor::INT)
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME_INT, register_library, circuit, TM);
   else if(port_in->get_typeRef()->type == structural_type_descriptor::UINT)
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME_UINT, register_library, circuit, TM);
   else if(port_in->get_typeRef()->type == structural_type_descriptor::REAL)
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME_REAL, register_library, circuit, TM);
   else
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME, register_library, circuit, TM);
   GetPointer<module>(reg_mod)->get_in_port(2)->type_resize(GET_TYPE_SIZE(port_in));
   GetPointer<module>(reg_mod)->get_out_port(0)->type_resize(GET_TYPE_SIZE(port_in));

   structural_objectRef port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
   SM->add_connection(clock_port, port_ck);

   structural_objectRef reset_ck = reg_mod->find_member(RESET_PORT_NAME, port_o_K, reg_mod);
   if(reset_ck)
      SM->add_connection(reset_port, reset_ck);

   r_signal = SM->add_sign(port_prefix + "_SIGI1", circuit, port_in->get_typeRef());
   SM->add_connection(e_port, r_signal);
   SM->add_connection(GetPointer<module>(reg_mod)->get_in_port(2), r_signal);

   r_signal = SM->add_sign(port_prefix + "_SIGI2", circuit, port_in->get_typeRef());
   SM->add_connection(GetPointer<module>(reg_mod)->get_out_port(0), r_signal);
   SM->add_connection(port_in, r_signal);
}

void RTLCharacterization::add_output_register(structural_managerRef SM, structural_objectRef e_port, structural_objectRef circuit, structural_objectRef reset_port, structural_objectRef port_out, const std::string& port_prefix,
                                              structural_objectRef clock_port, const std::string& register_library)
{
   structural_objectRef r_signal;
   structural_objectRef reg_mod;
   if(port_out->get_typeRef()->type == structural_type_descriptor::INT)
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME_INT, register_library, circuit, TM);
   else if(port_out->get_typeRef()->type == structural_type_descriptor::UINT)
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME_UINT, register_library, circuit, TM);
   else if(port_out->get_typeRef()->type == structural_type_descriptor::REAL)
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME_REAL, register_library, circuit, TM);
   else
      reg_mod = SM->add_module_from_technology_library(port_prefix + "_REG", register_AR_NORETIME, register_library, circuit, TM);
   GetPointer<module>(reg_mod)->get_in_port(2)->type_resize(GET_TYPE_SIZE(port_out));
   GetPointer<module>(reg_mod)->get_out_port(0)->type_resize(GET_TYPE_SIZE(port_out));

   structural_objectRef port_ck = reg_mod->find_member(CLOCK_PORT_NAME, port_o_K, reg_mod);
   SM->add_connection(clock_port, port_ck);

   structural_objectRef reset_ck = reg_mod->find_member(RESET_PORT_NAME, port_o_K, reg_mod);
   if(reset_ck)
      SM->add_connection(reset_port, reset_ck);

   r_signal = SM->add_sign(port_prefix + "_SIGO1", circuit, port_out->get_typeRef());
   SM->add_connection(port_out, r_signal);
   SM->add_connection(GetPointer<module>(reg_mod)->get_in_port(2), r_signal);
   r_signal = SM->add_sign(port_prefix + "_SIGO2", circuit, port_out->get_typeRef());
   SM->add_connection(GetPointer<module>(reg_mod)->get_out_port(0), r_signal);
   SM->add_connection(e_port, r_signal);
}

bool RTLCharacterization::HasToBeExecuted() const
{
   return true;
}

const std::string RTLCharacterization::GetSignature() const
{
   return "RTLCharacterization";
}

const std::string RTLCharacterization::GetName() const
{
   return "RTLCharacterization";
}

void RTLCharacterization::ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type)
{
   switch(relationship_type)
   {
      case DesignFlowStep::DEPENDENCE_RELATIONSHIP:
      {
         const DesignFlowGraphConstRef design_flow_graph = design_flow_manager.lock()->CGetDesignFlowGraph();
         const auto* technology_flow_step_factory = GetPointer<const TechnologyFlowStepFactory>(design_flow_manager.lock()->CGetDesignFlowStepFactory("Technology"));
         const std::string technology_flow_signature = TechnologyFlowStep::ComputeSignature(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         const vertex technology_flow_step = design_flow_manager.lock()->GetDesignFlowStep(technology_flow_signature);
         const DesignFlowStepRef technology_design_flow_step =
             technology_flow_step ? design_flow_graph->CGetDesignFlowStepInfo(technology_flow_step)->design_flow_step : technology_flow_step_factory->CreateTechnologyFlowStep(TechnologyFlowStep_Type::LOAD_TECHNOLOGY);
         relationship.insert(technology_design_flow_step);
         break;
      }
      case DesignFlowStep::INVALIDATION_RELATIONSHIP:
      case DesignFlowStep::PRECEDENCE_RELATIONSHIP:
      {
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
}

const DesignFlowStepFactoryConstRef RTLCharacterization::CGetDesignFlowStepFactory() const
{
   THROW_UNREACHABLE("Not implemented");
   return DesignFlowStepFactoryConstRef();
}

void RTLCharacterization::AnalyzeCell(functional_unit* fu, const unsigned int prec, const std::vector<std::string>& portsize_parameters, const size_t portsize_index, const std::vector<std::string>& pipe_parameters, const size_t stage_index,
                                      const unsigned int constPort, const bool is_commutative, size_t max_lut_size)
{
   const auto fu_name = fu->get_name();
   const auto fu_base_name = fu->fu_template_name != "" ? fu->fu_template_name : fu_name;
   if(cells.find(fu_name) != cells.end())
   {
      size_t n_portsize_parameters = portsize_parameters.size();
      size_t n_pipe_parameters = pipe_parameters.size();

      const structural_objectRef obj = fu->CM->get_circ();
      unsigned int n_ports = GetPointer<module>(obj)->get_in_port_size();
      const NP_functionalityRef NPF = GetPointer<module>(obj)->get_NP_functionality();
      const bool isTemplate = fu->fu_template_parameters != "";
      if(constPort < n_ports)
      {
         /// some modules has to be characterized with respect to a different constant value
         if(isTemplate && fu->characterizing_constant_value == "")
            fu->characterizing_constant_value = STR(6148914691236517205);
      }
      else
         fu->characterizing_constant_value = "";
#ifndef NDEBUG
      const bool assertion_argument = NPF->exist_NP_functionality(NP_functionality::VERILOG_PROVIDED)
#if HAVE_FLOPOCO
                                      || NPF->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED)
#endif
                                      || NPF->exist_NP_functionality(NP_functionality::VHDL_PROVIDED) || NPF->exist_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED);
      THROW_ASSERT(assertion_argument, "Verilog, VHDL, SystemVerilog or Flopoco description not provided for functional unit " + fu_name);
#endif
      structural_managerRef SM = structural_managerRef(new structural_manager(parameters));
      /// main circuit type
      auto top_wrapper_name = "top" + fu_name + "_wrapper";
      boost::replace_all(top_wrapper_name, "__", "");
      structural_type_descriptorRef module_type = structural_type_descriptorRef(new structural_type_descriptor(top_wrapper_name));
      /// setting top circuit component
      SM->set_top_info(top_wrapper_name, module_type);
      structural_objectRef circuit = SM->get_circ();
      THROW_ASSERT(circuit, "Top circuit is missing");
      structural_objectRef template_circuit = SM->add_module_from_technology_library(fu_base_name + "_inst0", fu_base_name, LM->get_library_name(), circuit, TM);

      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " - Generating HDL of functional unit " + fu_name);
      auto* spec_module = GetPointer<module>(template_circuit);

      std::string memory_type = fu->memory_type;
      std::string channels_type = fu->channels_type;
      unsigned int BRAM_BITSIZE = 16;
      const target_deviceRef device = target->get_target_device();
      if(BRAM_BITSIZE > device->get_parameter<unsigned int>("BRAM_bitsize_max"))
         BRAM_BITSIZE = device->get_parameter<unsigned int>("BRAM_bitsize_max");
      unsigned int ALIGNED_BITSIZE = 2 * BRAM_BITSIZE;
      unsigned int BUS_DATA_BITSIZE = 2 * BRAM_BITSIZE;
      unsigned int BUS_ADDR_BITSIZE = 15;
      unsigned int BUS_SIZE_BITSIZE = 7;
      unsigned int BUS_TAG_BITSIZE = 8;
      unsigned int NUMBER_OF_BYTES_ALLOCATED = 1024;
      if(memory_type == MEMORY_TYPE_ASYNCHRONOUS)
         NUMBER_OF_BYTES_ALLOCATED = NUMBER_OF_BYTES_ALLOCATED / 16;
      specialize_fu(spec_module, prec, BUS_DATA_BITSIZE, BUS_ADDR_BITSIZE, BUS_SIZE_BITSIZE, BUS_TAG_BITSIZE, n_portsize_parameters > 0 ? boost::lexical_cast<unsigned int>(portsize_parameters[portsize_index]) : PORT_VECTOR_N_PORTS);

      if(fu_base_name == "MC_FU") /// add further specializations for this module
      {
         spec_module->SetParameter("EXECUTION_TIME", STR(2));
         spec_module->SetParameter("BITSIZE", STR(8));
         spec_module->SetParameter("INITIATION_TIME", STR(1));
      }
      else if(memory_type != "")
      {
         unsigned int base_address = 0;
         std::string init_filename = "array_ref_" + boost::lexical_cast<std::string>(base_address) + ".mem";
         unsigned int counter = 0;
         unsigned int nbyte_on_memory = BRAM_BITSIZE / 8;
         unsigned int elts_size = BUS_DATA_BITSIZE;
         unsigned int vec_size = NUMBER_OF_BYTES_ALLOCATED / (elts_size / 8);
         if(memory_type == MEMORY_TYPE_ASYNCHRONOUS)
         {
            BRAM_BITSIZE = elts_size;
            nbyte_on_memory = elts_size / 8;
            base_address = 0;
         }
         if(memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS || memory_type == MEMORY_TYPE_SYNCHRONOUS_SDS_BUS)
         {
            BRAM_BITSIZE = elts_size;
            nbyte_on_memory = elts_size / 8;
         }
         if(memory_type == MEMORY_TYPE_SYNCHRONOUS_UNALIGNED &&
            (channels_type.find(CHANNELS_TYPE_MEM_ACC_NN) != std::string::npos || (channels_type.find(CHANNELS_TYPE_MEM_ACC_N1) != std::string::npos && channels_type.find(CHANNELS_TYPE_MEM_ACC_11) == std::string::npos)))
         {
            std::ofstream init_file_a(GetPath(("a_" + init_filename).c_str()));
            std::ofstream init_file_b(GetPath(("b_" + init_filename).c_str()));
            bool is_even = true;
            for(unsigned int i = 0; i < vec_size; ++i)
            {
               for(unsigned int j = 0; j < BRAM_BITSIZE; ++j)
               {
                  long int random_value = random();
                  std::string bit_val = (random_value & 1) == 0 ? "0" : "1";
                  if(is_even)
                     init_file_a << bit_val;
                  else
                     init_file_b << bit_val;
                  counter++;
                  if(counter % (nbyte_on_memory * 8) == 0)
                  {
                     if(is_even)
                        init_file_a << std::endl;
                     else
                        init_file_b << std::endl;
                     is_even = !is_even;
                  }
               }
            }
            init_file_a.close();
            init_file_b.close();
         }
         else
         {
            std::ofstream init_file(GetPath(init_filename.c_str()));
            for(unsigned int i = 0; i < vec_size; ++i)
            {
               for(unsigned int j = 0; j < elts_size; ++j)
               {
                  long int random_value = random();
                  std::string bit_val = (random_value & 1) == 0 ? "0" : "1";
                  init_file << bit_val;
                  counter++;
                  if(counter % (nbyte_on_memory * 8) == 0)
                     init_file << std::endl;
               }
            }
            init_file.close();
         }
         spec_module->SetParameter("address_space_begin", STR(base_address));
         spec_module->SetParameter("address_space_rangesize", STR((elts_size / 8) * vec_size));
         spec_module->SetParameter("USE_SPARSE_MEMORY", "1");
         if(memory_type == MEMORY_TYPE_SYNCHRONOUS_UNALIGNED &&
            (channels_type.find(CHANNELS_TYPE_MEM_ACC_NN) != std::string::npos || (channels_type.find(CHANNELS_TYPE_MEM_ACC_N1) != std::string::npos && channels_type.find(CHANNELS_TYPE_MEM_ACC_11) == std::string::npos)))
         {
            spec_module->SetParameter("MEMORY_INIT_file_a", "\"\"a_" + init_filename + "\"\"");
            spec_module->SetParameter("MEMORY_INIT_file_b", "\"\"b_" + init_filename + "\"\"");
         }
         else
            spec_module->SetParameter("MEMORY_INIT_file", "\"\"" + init_filename + "\"\"");
         spec_module->SetParameter("n_elements", STR(vec_size));
         spec_module->SetParameter("data_size", STR(elts_size));
         spec_module->SetParameter("BRAM_BITSIZE", STR(BRAM_BITSIZE));
         spec_module->SetParameter("BUS_PIPELINED", "1");
         spec_module->SetParameter("PRIVATE_MEMORY", "0");
      }
      else if(fu_base_name == MEMLOAD_STD)
         spec_module->SetParameter("base_address", "8");
      else if(fu_base_name == MEMSTORE_STD)
         spec_module->SetParameter("base_address", "8");
      structural_objectRef e_port, one_port;

      if(n_pipe_parameters > 0)
      {
         spec_module->SetParameter(PIPE_PARAMETER, pipe_parameters[stage_index]);
         PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " - PIPE_PARAMETER=" + pipe_parameters[stage_index]);
      }
      if(NPF)
      {
         std::vector<std::string> params;
         NPF->get_library_parameters(params);
         for(const auto param : params)
         {
            if(param == "PRECISION")
            {
               unsigned int precision_bitsize = prec;
               precision_bitsize = std::max(8u, precision_bitsize);
               spec_module->SetParameter("PRECISION", boost::lexical_cast<std::string>(precision_bitsize));
            }
            else if(param == "ALIGNED_BITSIZE")
            {
               spec_module->SetParameter("ALIGNED_BITSIZE", boost::lexical_cast<std::string>(ALIGNED_BITSIZE));
            }
            else if(param == "LSB_PARAMETER")
            {
               spec_module->SetParameter("LSB_PARAMETER", boost::lexical_cast<std::string>(0));
            }
            THROW_ASSERT(template_circuit->find_member(param, port_o_K, template_circuit) || template_circuit->find_member(param, port_vector_o_K, template_circuit) || spec_module->ExistsParameter(param),
                         "parameter not yet specialized: " + param + " for module " + spec_module->get_typeRef()->get_name());
         }
      }

      structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
      one_port = SM->add_constant(fu_name + "_constant_" + STR(1), circuit, bool_type, STR(1));
      std::string register_library = TM->get_library(register_AR_NORETIME);
      structural_objectRef clock_port, reset_port;

      /// add clock and reset
      for(unsigned int i = 0; i < spec_module->get_in_port_size(); i++)
      {
         const structural_objectRef port_in = spec_module->get_in_port(i);
         if(port_in->get_id() == CLOCK_PORT_NAME)
         {
            clock_port = SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            SM->add_connection(port_in, clock_port);
         }
         if(port_in->get_id() == RESET_PORT_NAME)
         {
            reset_port = SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            SM->add_connection(port_in, reset_port);
         }
         else if(port_in->get_id() == START_PORT_NAME)
         {
            e_port = SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            SM->add_connection(port_in, e_port);
         }
      }
      if(!clock_port)
      {
         clock_port = SM->add_port(CLOCK_PORT_NAME, port_o::IN, circuit, bool_type);
      }
      if(!reset_port)
      {
         reset_port = SM->add_port(RESET_PORT_NAME, port_o::IN, circuit, bool_type);
      }

      for(unsigned int i = 0; i < spec_module->get_in_port_size(); i++)
      {
         structural_objectRef port_in = spec_module->get_in_port(i);
         if(port_in->get_id() == CLOCK_PORT_NAME || port_in->get_id() == RESET_PORT_NAME || port_in->get_id() == START_PORT_NAME)
            continue;
         if(fu_base_name == LUT_EXPR_STD && i == 0)
         {
            resize_port(port_in, 64);
            e_port = SM->add_constant("constant_0", circuit, port_in->get_typeRef(), STR(0xFF7F3F1F0F070301));
            SM->add_connection(port_in, e_port);
         }
         else if(fu_base_name == LUT_EXPR_STD && i > max_lut_size)
         {
            e_port = SM->add_constant("constant_" + STR(i), circuit, port_in->get_typeRef(), STR(0));
            SM->add_connection(port_in, e_port);
         }
         else if(isTemplate && i == constPort)
         {
            THROW_ASSERT(fu->characterizing_constant_value != "", "expected a value");
            e_port = SM->add_constant("constant_" + STR(constPort), circuit, port_in->get_typeRef(), fu->characterizing_constant_value);
            SM->add_connection(port_in, e_port);
         }
         else if(false)
         {
            if(port_in->get_kind() == port_vector_o_K)
            {
               for(unsigned int p = 0; p < GetPointer<port_o>(port_in)->get_ports_size(); ++p)
               {
                  e_port = SM->add_constant("constant_" + STR(i) + "_" + STR(p), circuit, port_in->get_typeRef(), STR(BUS_DATA_BITSIZE));
                  SM->add_connection(GetPointer<port_o>(port_in)->get_port(p), e_port);
               }
            }
            else
            {
               e_port = SM->add_constant("constant_" + STR(i), circuit, port_in->get_typeRef(), STR(BUS_DATA_BITSIZE));
               SM->add_connection(port_in, e_port);
            }
         }
         else
         {
            if(port_in->get_kind() == port_vector_o_K)
               e_port = SM->add_port_vector(GetPointer<port_o>(port_in)->get_id(), port_o::IN, GetPointer<port_o>(port_in)->get_ports_size(), circuit, GetPointer<port_o>(port_in)->get_port(0)->get_typeRef());
            else
               e_port = SM->add_port(GetPointer<port_o>(port_in)->get_id(), port_o::IN, circuit, port_in->get_typeRef());
            std::string port_prefix = GetPointer<port_o>(port_in)->get_id();

            /// add register on inputs
            if(port_in->get_kind() == port_vector_o_K)
            {
               for(unsigned int p = 0; p < GetPointer<port_o>(port_in)->get_ports_size(); ++p)
               {
                  add_input_register(GetPointer<port_o>(port_in)->get_port(p), register_library, port_prefix + GetPointer<port_o>(port_in)->get_port(p)->get_id(), reset_port, circuit, clock_port, GetPointer<port_o>(e_port)->get_port(p), SM);
               }
            }
            else
               add_input_register(port_in, register_library, port_prefix, reset_port, circuit, clock_port, e_port, SM);
         }
      }

#if HAVE_FLOPOCO
      bool is_doubled_out = false;
      bool is_halved_out = false;
#endif
      for(unsigned int i = 0; i < spec_module->get_out_port_size(); i++)
      {
         structural_objectRef port_out = spec_module->get_out_port(i);
#if HAVE_FLOPOCO
         if(GetPointer<port_o>(port_out)->get_is_doubled())
            is_doubled_out = true;
         else if(GetPointer<port_o>(port_out)->get_is_halved())
            is_halved_out = true;
#endif
         if(port_out->get_kind() == port_vector_o_K)
            e_port = SM->add_port_vector(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, GetPointer<port_o>(port_out)->get_ports_size(), circuit, GetPointer<port_o>(port_out)->get_port(0)->get_typeRef());
         else
            e_port = SM->add_port(GetPointer<port_o>(port_out)->get_id(), port_o::OUT, circuit, port_out->get_typeRef());
         std::string port_prefix = GetPointer<port_o>(port_out)->get_id();

         /// add register on outputs
         if(port_out->get_kind() == port_vector_o_K)
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(port_out)->get_ports_size(); ++p)
            {
               add_output_register(SM, GetPointer<port_o>(e_port)->get_port(p), circuit, reset_port, GetPointer<port_o>(port_out)->get_port(p), port_prefix + GetPointer<port_o>(port_out)->get_port(p)->get_id(), clock_port, register_library);
            }
         }
         else
            add_output_register(SM, e_port, circuit, reset_port, port_out, port_prefix, clock_port, register_library);
      }

      // get the wrapped circuit.
      HDL_managerRef HDL = HDL_managerRef(new HDL_manager(HLS_managerRef(), device, parameters));
      std::list<std::string> hdl_files, aux_files;
      std::list<structural_objectRef> circuits;
      circuits.push_back(circuit);
      HDL->hdl_gen(fu_name, circuits, false, hdl_files, aux_files);
      int PipelineDepth = -1;
#if HAVE_FLOPOCO
      if(n_pipe_parameters > 0 && NPF && NPF->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) && HDL->get_flopocowrapper())
      {
         if(is_doubled_out)
            PipelineDepth = static_cast<int>(HDL->get_flopocowrapper()->get_FUPipelineDepth(fu_base_name, prec, 2 * prec, pipe_parameters[stage_index]));
         else if(is_halved_out)
            PipelineDepth = static_cast<int>(HDL->get_flopocowrapper()->get_FUPipelineDepth(fu_base_name, prec, prec / 2, pipe_parameters[stage_index]));
         else
            PipelineDepth = static_cast<int>(HDL->get_flopocowrapper()->get_FUPipelineDepth(fu_base_name, prec, prec, pipe_parameters[stage_index]));
      }
#endif
      /// generate the synthesis scripts
      BackendFlowRef flow = BackendFlow::CreateFlow(parameters, "Characterization", target);
      flow->GenerateSynthesisScripts(fu->get_name(), SM, hdl_files, aux_files);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, "Performing characterization of functional unit " + fu_name);
#ifndef NDEBUG
      if(not dummy_synthesis)
#endif
      {
         if(constPort < n_ports && is_commutative && constPort > has_first_synthesis_id && prev_area_characterization && prev_timing_characterization)
            fu->area_m = prev_area_characterization;
         else
         {
            flow->ExecuteSynthesis();
            has_first_synthesis_id = constPort;
            /// the synthesis has been successfully completed
            /// setting the used resources
            fu->area_m = flow->get_used_resources();
         }
      }
#ifndef NDEBUG
      else
      {
         fu->area_m = area_model::create_model(device->get_type(), parameters);
      }
#endif
      /// setting the timing values for each operation
      const functional_unit::operation_vec& ops = fu->get_operations();
      for(const auto& op : ops)
      {
         auto* new_op = GetPointer<operation>(op);
         time_modelRef synthesis_results;
#ifndef NDEBUG
         if(not dummy_synthesis)
#endif
         {
            if(constPort < n_ports && is_commutative && constPort > has_first_synthesis_id && prev_area_characterization && prev_timing_characterization)
               synthesis_results = prev_timing_characterization;
            else
               synthesis_results = flow->get_timing_results();
         }
#ifndef NDEBUG
         else
         {
            synthesis_results = time_model::create_model(device->get_type(), parameters);
            synthesis_results->set_execution_time(7.75, time_model::cycles_time_DEFAULT);
         }
#endif
         double exec_time = 0.0;
         if(synthesis_results)
            exec_time = synthesis_results->get_execution_time();

         if(!new_op->time_m)
            new_op->time_m = time_model::create_model(device->get_type(), parameters);

         if(n_pipe_parameters > 0)
         {
            new_op->time_m->set_stage_period(time_model::stage_period_DEFAULT);
            new_op->time_m->set_execution_time(time_model::execution_time_DEFAULT, time_model::cycles_time_DEFAULT);
            const ControlStep ii_default(time_model::initiation_time_DEFAULT);
            new_op->time_m->set_initiation_time(ii_default);

            unsigned int n_cycles;
            n_cycles = boost::lexical_cast<unsigned int>(pipe_parameters[stage_index]);
            new_op->pipe_parameters = pipe_parameters[stage_index];

            if(n_cycles > 0 && PipelineDepth != 0)
            {
               new_op->time_m->set_stage_period(exec_time);
               const ControlStep ii(1u);
               new_op->time_m->set_initiation_time(ii);
               if(PipelineDepth == -1)
                  new_op->time_m->set_execution_time(exec_time, n_cycles + 1);
               else
                  new_op->time_m->set_execution_time(exec_time, static_cast<unsigned int>(PipelineDepth) + 1);
            }
            else if(PipelineDepth == 0)
               new_op->time_m->set_execution_time(exec_time, time_model::cycles_time_DEFAULT);
            else
               new_op->time_m->set_execution_time(exec_time, n_cycles);
         }
         else if(new_op->time_m->get_cycles() == 0)
         {
            new_op->time_m->set_execution_time(exec_time, time_model::cycles_time_DEFAULT);
         }
         else
         {
            new_op->time_m->set_stage_period(exec_time);
         }
      }

#ifndef NDEBUG
      if(not dummy_synthesis)
#endif
      {
         if(constPort < n_ports && is_commutative && constPort == has_first_synthesis_id && flow->get_used_resources() && flow->get_timing_results())
         {
            prev_area_characterization = flow->get_used_resources();
            prev_timing_characterization = flow->get_timing_results();
            THROW_ASSERT(prev_area_characterization, "expected a previous synthesis result");
            THROW_ASSERT(prev_timing_characterization, "expected a previous synthesis result");
         }
      }
      completed.insert(fu->functional_unit_name);
   }
   else
   {
      prev_area_characterization = area_modelRef();
      prev_timing_characterization = time_modelRef();
   }
}

const std::string RTLCharacterization::ComputeComponent(const std::string& input) const
{
   std::vector<std::string> component_cell = SplitString(input, ",");
   THROW_ASSERT(component_cell.size() > 0, input);
   std::vector<std::string> component_or_cell = SplitString(component_cell[0], "-");
   THROW_ASSERT(component_or_cell.size() == 2, component_or_cell[0]);
   return component_or_cell[0];
}

const CustomSet<std::string> RTLCharacterization::ComputeCells(const std::string& input) const
{
   CustomSet<std::string> ret;
   std::vector<std::string> component_cells = SplitString(input, ",");
   for(const auto& component_cell : component_cells)
   {
      std::vector<std::string> component_or_cell = SplitString(component_cell, "-");
      ;
      THROW_ASSERT(component_or_cell.size() == 2, component_cell);
      THROW_ASSERT(component_or_cell[0] == component, component_or_cell[0] + " vs " + component);
      ret.insert(component_or_cell[1]);
   }
   return ret;
}
