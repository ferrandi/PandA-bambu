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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file functional_unit_step.cpp
 * @brief Abstract class to iterate over all the cells of a template
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

/// Autoheader include
#include "config_HAVE_FLOPOCO.hpp"

/// Header class
#include "functional_unit_step.hpp"

/// circuit include
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

/// HLS/scheduling include
#include "schedule.hpp"

/// STD include
#include <string>

/// STL includes
#include "custom_map.hpp"
#include "custom_set.hpp"
#include <vector>

/// technology include
#include "target_manager.hpp"
#include "technology_manager.hpp"

/// technology/physical_library include
#include "library_manager.hpp"
#include "technology_node.hpp"

/// technology/physical_library/models includes
#include "area_model.hpp"
#include "time_model.hpp"

/// technology/target_device include
#include "target_device.hpp"

/// utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "math_function.hpp"
#include "string_manipulation.hpp"
#include <boost/algorithm/string/case_conv.hpp>

FunctionalUnitStep::FunctionalUnitStep(const target_managerRef _target, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters), TM(_target->get_technology_manager()), target(_target), has_first_synthesis_id(0)
{
}

FunctionalUnitStep::~FunctionalUnitStep() = default;

void FunctionalUnitStep::AnalyzeFu(const technology_nodeRef f_unit)
{
   const auto LM = TM->get_library(f_unit->get_name());
   const target_deviceRef device = target->get_target_device();

   bool is_commutative = true;

   CustomOrderedSet<unsigned int> precision;
   std::map<unsigned int, std::vector<std::string>> pipe_parameters;
   std::map<unsigned int, std::vector<std::string>> portsize_parameters;
   auto* fu_curr = GetPointer<functional_unit>(f_unit);
   if(fu_curr && fu_curr->fu_template_name != "")
      return; /// previous characterization is not considered
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing " + f_unit->get_name());
   bool isTemplate = false;
   bool no_constants = false;
   if(!fu_curr && GetPointer<functional_unit_template>(f_unit))
   {
      fu_curr = GetPointer<functional_unit>(GetPointer<functional_unit_template>(f_unit)->FU);
      isTemplate = true;
      no_constants = GetPointer<functional_unit_template>(f_unit)->no_constant_characterization;
   }
   THROW_ASSERT(fu_curr, "unexpected condition");
   std::string fu_name = fu_curr->functional_unit_name;
   std::string fu_base_name = fu_name;
   const functional_unit::operation_vec& Ops = fu_curr->get_operations();
   auto ops_end = Ops.end();
   if(fu_base_name == READ_COND_STD)
      precision.insert(1);
   else
   {
      for(auto ops = Ops.begin(); ops != ops_end; ++ops)
      {
         auto* curr_op = GetPointer<operation>(*ops);
         is_commutative = is_commutative && curr_op->commutative;
         std::map<std::string, std::vector<unsigned int>>::const_iterator supported_type_it_end = curr_op->supported_types.end();
         if(curr_op->supported_types.begin() == curr_op->supported_types.end())
         {
            if(isTemplate)
            {
               precision.insert(1);
               precision.insert(8);
               precision.insert(16);
               precision.insert(64);
            }
            precision.insert(32);
         }
         else
         {
            for(std::map<std::string, std::vector<unsigned int>>::const_iterator supported_type_it = curr_op->supported_types.begin(); supported_type_it != supported_type_it_end; ++supported_type_it)
            {
               auto prec_it_end = supported_type_it->second.end();
               auto prec_it = supported_type_it->second.begin();
               if(prec_it == prec_it_end)
               {
                  if(isTemplate)
                  {
                     precision.insert(1);
                     precision.insert(8);
                     precision.insert(16);
                     precision.insert(64);
                  }
                  precision.insert(32);
               }
               else
               {
                  for(; prec_it != prec_it_end; ++prec_it)
                     precision.insert(*prec_it);
               }
            }
         }
         std::string pipe_parameters_str = curr_op->pipe_parameters;

         if(pipe_parameters_str != "")
         {
            std::vector<std::string> parameters_split = SplitString(pipe_parameters_str, "|");
            const std::vector<std::string>::const_iterator pp_it_end = parameters_split.end();
            for(std::vector<std::string>::const_iterator pp_it = parameters_split.begin(); pp_it != pp_it_end; ++pp_it)
            {
               std::vector<std::string> precision_pipe_param_pair = SplitString(*pp_it, ":");
               THROW_ASSERT(precision_pipe_param_pair.size() == 2, "malformed pipe parameter string");
               std::vector<std::string> pipe_params = SplitString(precision_pipe_param_pair[1], ",");
               THROW_ASSERT(pipe_params.size() > 0, "malformed pipe parameter string");
               if(precision_pipe_param_pair[0] == "*")
               {
                  for(unsigned int prec : precision)
                     for(std::vector<std::string>::const_iterator param_it = pipe_params.begin(), param_it_end = pipe_params.end(); param_it != param_it_end; ++param_it)
                        if(std::find(pipe_parameters[prec].begin(), pipe_parameters[prec].end(), *param_it) == pipe_parameters[prec].end())
                           pipe_parameters[prec].push_back(*param_it);
               }
               else if(precision_pipe_param_pair[0] == "DSPs_y_sizes")
               {
                  for(const auto& DSP_y : DSP_y_to_DSP_x)
                  {
                     for(const auto& pipe_param : pipe_params)
                     {
                        pipe_parameters[boost::lexical_cast<unsigned int>(DSP_y.first)].push_back(pipe_param);
                        precision.insert(boost::lexical_cast<unsigned int>(DSP_y.first));
                     }
                  }
               }
               else if(precision.find(boost::lexical_cast<unsigned int>(precision_pipe_param_pair[0])) != precision.end())
               {
                  for(std::vector<std::string>::const_iterator param_it = pipe_params.begin(), param_it_end = pipe_params.end(); param_it != param_it_end; ++param_it)
                     if(std::find(pipe_parameters[boost::lexical_cast<unsigned int>(precision_pipe_param_pair[0])].begin(), pipe_parameters[boost::lexical_cast<unsigned int>(precision_pipe_param_pair[0])].end(), *param_it) ==
                        pipe_parameters[boost::lexical_cast<unsigned int>(precision_pipe_param_pair[0])].end())
                        pipe_parameters[boost::lexical_cast<unsigned int>(precision_pipe_param_pair[0])].push_back(*param_it);
               }
               else
                  THROW_ERROR("malformed pipe parameter string");
            }
         }
         std::string portsize_parameters_str = curr_op->portsize_parameters;
         if(portsize_parameters_str != "")
         {
            std::vector<std::string> parameters_split = SplitString(portsize_parameters_str, "|");
            const std::vector<std::string>::const_iterator pp_it_end = parameters_split.end();
            for(std::vector<std::string>::const_iterator pp_it = parameters_split.begin(); pp_it != pp_it_end; ++pp_it)
            {
               std::vector<std::string> precision_portsize_param_pair = SplitString(*pp_it, ":");
               THROW_ASSERT(precision_portsize_param_pair.size() == 2, "malformed portsize parameter string");
               std::vector<std::string> portsize_params = SplitString(precision_portsize_param_pair[1], ",");
               THROW_ASSERT(portsize_params.size() > 0, "malformed portsize parameter string");
               if(precision_portsize_param_pair[0] == "*")
               {
                  for(unsigned int prec : precision)
                     for(std::vector<std::string>::const_iterator param_it = portsize_params.begin(), param_it_end = portsize_params.end(); param_it != param_it_end; ++param_it)
                        if(std::find(portsize_parameters[prec].begin(), portsize_parameters[prec].end(), *param_it) == portsize_parameters[prec].end())
                           portsize_parameters[prec].push_back(*param_it);
               }
               else if(precision.find(boost::lexical_cast<unsigned int>(precision_portsize_param_pair[0])) != precision.end())
               {
                  for(std::vector<std::string>::const_iterator param_it = portsize_params.begin(), param_it_end = portsize_params.end(); param_it != param_it_end; ++param_it)
                     if(std::find(portsize_parameters[boost::lexical_cast<unsigned int>(precision_portsize_param_pair[0])].begin(), portsize_parameters[boost::lexical_cast<unsigned int>(precision_portsize_param_pair[0])].end(), *param_it) ==
                        portsize_parameters[boost::lexical_cast<unsigned int>(precision_portsize_param_pair[0])].end())
                        portsize_parameters[boost::lexical_cast<unsigned int>(precision_portsize_param_pair[0])].push_back(*param_it);
               }
               else
                  THROW_ERROR("malformed portsize parameter string");
            }
         }
      }
   }

   if(Ops.begin() == Ops.end())
      is_commutative = false;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Computed parameters");

   for(unsigned int prec : precision)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering precision " + STR(prec) + " bits");
      if(fu_curr->CM)
      {
         const structural_objectRef obj = fu_curr->CM->get_circ();
         NP_functionalityRef NPF = GetPointer<module>(obj)->get_NP_functionality();
         size_t max_lut_size = static_cast<size_t>(-1);
         if(device->has_parameter("max_lut_size"))
         {
            max_lut_size = device->get_parameter<size_t>("max_lut_size");
         }
#if HAVE_FLOPOCO
         std::string vendor;
         if(device->has_parameter("vendor"))
         {
            vendor = device->get_parameter<std::string>("vendor");
            boost::algorithm::to_lower(vendor);
         }
         bool is_xilinx = vendor == "xilinx";
         bool is_lattice = vendor == "lattice";
         bool is_altera = vendor == "altera";
         bool is_nanoxplore = vendor == "nanoxplore";
#endif

         if(!(NPF->exist_NP_functionality(NP_functionality::VERILOG_PROVIDED)
#if HAVE_FLOPOCO
              || (NPF->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED) && (is_xilinx || is_altera || is_lattice || is_nanoxplore))
#endif
              || (NPF->exist_NP_functionality(NP_functionality::VHDL_PROVIDED)) || (NPF->exist_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED))) ||
            fu_base_name == LUT_GATE_STD || fu_base_name == AND_GATE_STD || fu_base_name == NAND_GATE_STD || fu_base_name == OR_GATE_STD || fu_base_name == NOR_GATE_STD || fu_base_name == XOR_GATE_STD || fu_base_name == XNOR_GATE_STD ||
            fu_base_name == "split_signal" || fu_base_name == "FSL_handler" ||
            fu_base_name == "extract_bit_expr_FU"
            //|| fu_base_name != "mult_expr_DSP"
            //|| fu_base_name != "trunc_div_expr_FU"
            //|| fu_base_name != "fp_fix_trunc_expr_FU"
            //|| fu_base_name == "fp_log_FU"
            || fu_base_name.find(CONSTANT_STD) != std::string::npos)
         {
         }
         else
         {
            const module* mod = GetPointer<module>(obj);
            unsigned int n_ports = mod->get_in_port_size();
            /// check for a single port
            unsigned int n_port_to_be_specialized = 0;
            for(unsigned int i = 0; i < n_ports; ++i)
            {
               structural_objectRef port_c = mod->get_in_port(i);
               if(port_c &&
                  (port_c->get_id() == CLOCK_PORT_NAME || port_c->get_id() == RESET_PORT_NAME || port_c->get_id() == START_PORT_NAME || (GetPointer<port_o>(port_c) && GetPointer<port_o>(port_c)->get_is_memory()) || port_c->get_id().find("sel_") == 0))
                  continue;
               ++n_port_to_be_specialized;
            }
            unsigned int constPort;
            /// check if the resource can be pipelined
            size_t n_pipe_parameters = pipe_parameters[prec].size();
            size_t n_iterations_pipe = n_pipe_parameters > 1 ? n_pipe_parameters : 1;
            size_t n_portsize_parameters = portsize_parameters[prec].size();
            size_t n_iterations_portsize = n_portsize_parameters > 1 ? n_portsize_parameters : 1;

            for(size_t portsize_index = 0; portsize_index < n_iterations_portsize; ++portsize_index)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Considering portsize_index " + STR(portsize_index));
               for(size_t stage_index = 0; stage_index < n_iterations_pipe; ++stage_index)
               {
                  if(n_port_to_be_specialized <= 1 || !isTemplate || fu_base_name == MUX_GATE_STD || fu_base_name == DEMUX_GATE_STD || no_constants)
                     constPort = n_ports; // Set constPort to in_port_size to immediately stop the loop after one iteration.
                  else
                     constPort = 0;
                  has_first_synthesis_id = n_ports + 2;
                  for(; constPort < n_ports + 1; ++constPort)
                  {
                     structural_objectRef port_c = n_ports > constPort ? mod->get_in_port(constPort) : structural_objectRef();
                     if(port_c && (port_c->get_id() == CLOCK_PORT_NAME || port_c->get_id() == RESET_PORT_NAME || port_c->get_id() == START_PORT_NAME || (GetPointer<port_o>(port_c) && GetPointer<port_o>(port_c)->get_is_memory()) ||
                                   port_c->get_id().find("sel_") == 0))
                        continue;
                     std::string template_parameters;
                     fu_name = fu_base_name;
                     template_parameters = "";
                     if(isTemplate)
                     {
                        for(unsigned int iport = 0; iport < n_ports; ++iport)
                        {
                           structural_objectRef port = mod->get_in_port(iport);
                           if(port->get_id() == CLOCK_PORT_NAME || port->get_id() == RESET_PORT_NAME || port->get_id() == START_PORT_NAME || (GetPointer<port_o>(port) && GetPointer<port_o>(port)->get_is_memory()) || port->get_id().find("sel_") == 0)
                              continue;
                           if(template_parameters != "")
                              template_parameters += " ";
                           THROW_ASSERT(port, "expected a port");
                           if(port->get_typeRef()->type == structural_type_descriptor::BOOL)
                           {
                              fu_name += "_" + STR(1);
                              template_parameters += STR(1);
                           }
                           else if(iport == 1 and (fu_base_name == "widen_mult_expr_FU" or fu_base_name == "ui_widen_mult_expr_FU" or fu_base_name == "mult_expr_FU" or fu_base_name == "ui_mult_expr_FU") and
                                   DSP_y_to_DSP_x.find(prec) != DSP_y_to_DSP_x.end())
                           {
                              fu_name += "_" + STR(DSP_y_to_DSP_x.find(prec)->second);
                              template_parameters += STR(DSP_y_to_DSP_x.find(prec)->second);
                           }
                           else if(iport != constPort)
                           {
                              fu_name += "_" + STR(prec);
                              template_parameters += STR(prec);
                              if(port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_REAL)
                              {
                                 fu_name += "_" + STR(128 / prec);
                                 template_parameters += " " + STR(128 / prec);
                              }
                           }
                           else
                           {
                              fu_name += "_" + STR(0);
                              template_parameters += STR(0);
                           }
                        }
                        // output port
                        for(unsigned int oport = 0; oport < mod->get_out_port_size(); ++oport)
                        {
                           structural_objectRef port = mod->get_out_port(oport);
                           THROW_ASSERT(port, "expected a port");
                           if(port->get_id() == DONE_PORT_NAME || (GetPointer<port_o>(port)->get_is_memory()))
                              continue;
                           if((fu_base_name == "widen_mult_expr_FU" or fu_base_name == "ui_widen_mult_expr_FU") and DSP_y_to_DSP_x.find(prec) != DSP_y_to_DSP_x.end())
                           {
                              fu_name += "_" + STR(prec + DSP_y_to_DSP_x.find(prec)->second);
                              template_parameters += " " + STR(prec + DSP_y_to_DSP_x.find(prec)->second);
                           }
                           else if((fu_base_name == "mult_expr_FU" or fu_base_name == "ui_mult_expr_FU") and DSP_y_to_DSP_x.find(prec) != DSP_y_to_DSP_x.end())
                           {
                              fu_name += "_" + STR(resize_to_1_8_16_32_64_128_256_512(prec));
                              template_parameters += " " + STR(resize_to_1_8_16_32_64_128_256_512(prec));
                           }
                           else if(GetPointer<port_o>(port)->get_is_doubled())
                           {
                              fu_name += "_" + STR(2 * prec);
                              template_parameters += " " + STR(2 * prec);
                           }
                           else if(GetPointer<port_o>(port)->get_is_halved())
                           {
                              fu_name += "_" + STR(prec / 2);
                              template_parameters += " " + STR(prec / 2);
                           }
                           else
                           {
                              fu_name += "_" + STR(prec);
                              template_parameters += " " + STR(prec);
                           }
                           if(port->get_typeRef()->type == structural_type_descriptor::VECTOR_INT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_UINT || port->get_typeRef()->type == structural_type_descriptor::VECTOR_REAL)
                           {
                              if(GetPointer<port_o>(port)->get_is_doubled())
                              {
                                 fu_name += "_" + STR(128 / (2 * prec));
                                 template_parameters += " " + STR(128 / (2 * prec));
                              }
                              else if(GetPointer<port_o>(port)->get_is_halved())
                              {
                                 fu_name += "_" + STR(128 / (prec / 2));
                                 template_parameters += " " + STR(128 / (prec / 2));
                              }
                              else
                              {
                                 fu_name += "_" + STR(128 / prec);
                                 template_parameters += " " + STR(128 / prec);
                              }
                           }
                        }
                     }
                     if(n_pipe_parameters > 0)
                     {
                        fu_name += "_" + pipe_parameters[prec][stage_index];
                        template_parameters += " " + pipe_parameters[prec][stage_index];
                     }
                     if(n_portsize_parameters > 0)
                     {
                        fu_name += "_" + portsize_parameters[prec][portsize_index];
                        template_parameters += " " + portsize_parameters[prec][portsize_index];
                     }

                     functional_unit* fu;
                     technology_nodeRef tn = TM->get_fu(fu_name, LM);
                     if(!tn)
                     {
                        // Analyzing a template, specializations of that template won't be found in the library.
                        technology_nodeRef fun_unit;
                        if(GetPointer<functional_unit_template>(f_unit))
                           fun_unit = GetPointer<functional_unit_template>(f_unit)->FU;
                        else
                           fun_unit = f_unit;
                        tn = create_template_instance(fun_unit, fu_name, device, prec);
                        fu = GetPointer<functional_unit>(tn);
                        fu->fu_template_parameters = template_parameters;
                        TM->get_library_manager(LM)->add(tn);
                     }
                     else
                        fu = GetPointer<functional_unit>(tn);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing cell " + fu->get_name());
                     AnalyzeCell(fu, prec, portsize_parameters.find(prec)->second, portsize_index, pipe_parameters.find(prec)->second, stage_index, constPort, is_commutative, max_lut_size);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed cell " + fu->get_name());
                  }
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered portsize_index " + STR(portsize_index));
            }
         }
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Considered precision " + STR(prec) + " bits");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed " + f_unit->get_name());
}

void FunctionalUnitStep::Initialize()
{
   const target_deviceRef device = target->get_target_device();
   if(device->has_parameter("DSPs_y_sizes"))
   {
      THROW_ASSERT(device->has_parameter("DSPs_x_sizes"), "device description is not complete");
      std::string DSPs_x_sizes = device->get_parameter<std::string>("DSPs_x_sizes");
      std::string DSPs_y_sizes = device->get_parameter<std::string>("DSPs_y_sizes");
      std::vector<std::string> DSPs_x_sizes_vec = SplitString(DSPs_x_sizes, ",");
      std::vector<std::string> DSPs_y_sizes_vec = SplitString(DSPs_y_sizes, ",");
      for(size_t DSP_index = 0; DSP_index < DSPs_y_sizes_vec.size(); DSP_index++)
      {
         const auto DSPs_x_value = boost::lexical_cast<unsigned int>(DSPs_x_sizes_vec[DSP_index]);
         const auto DSPs_y_value = boost::lexical_cast<unsigned int>(DSPs_y_sizes_vec[DSP_index]);
         DSP_y_to_DSP_x[DSPs_y_value] = DSPs_x_value;
      }
   }
}

technology_nodeRef FunctionalUnitStep::create_template_instance(const technology_nodeRef& fu_template, std::string& name, const target_deviceRef& device, unsigned int prec)
{
   auto* curr_fu = GetPointer<functional_unit>(fu_template);
   THROW_ASSERT(curr_fu, "Null functional unit template");

   auto* specialized_fu = new functional_unit;
   specialized_fu->functional_unit_name = name;
   specialized_fu->fu_template_name = curr_fu->functional_unit_name;
   specialized_fu->characterizing_constant_value = curr_fu->characterizing_constant_value;
   specialized_fu->memory_type = curr_fu->memory_type;
   specialized_fu->channels_type = curr_fu->channels_type;
   specialized_fu->memory_ctrl_type = curr_fu->memory_ctrl_type;
   specialized_fu->CM = curr_fu->CM;
   specialized_fu->XML_description = curr_fu->XML_description;

   for(auto itr = curr_fu->get_operations().begin(), end = curr_fu->get_operations().end(); itr < end; ++itr)
   {
      auto* const op = GetPointer<operation>(*itr);
      auto* new_op = new operation;
      new_op->operation_name = op->operation_name;
      new_op->bounded = op->bounded;

      new_op->time_m = time_model::create_model(device->get_type(), parameters);
      if(op->time_m)
      {
         new_op->time_m->set_execution_time(op->time_m->get_execution_time(), op->time_m->get_cycles());
         new_op->time_m->set_synthesis_dependent(op->time_m->get_synthesis_dependent());
         const ControlStep ii(op->time_m->get_initiation_time());
         new_op->time_m->set_initiation_time(ii);
      }
      new_op->commutative = op->commutative;
      std::map<std::string, std::vector<unsigned int>>::const_iterator supported_type_it_end = op->supported_types.end();
      if(op->supported_types.begin() != op->supported_types.end())
      {
         for(std::map<std::string, std::vector<unsigned int>>::const_iterator supported_type_it = op->supported_types.begin(); supported_type_it != supported_type_it_end; ++supported_type_it)
         {
            new_op->supported_types[supported_type_it->first].push_back(prec);
         }
      }
      specialized_fu->add(technology_nodeRef(new_op));
   }

   specialized_fu->area_m = area_model::create_model(device->get_type(), parameters);

   return technology_nodeRef(specialized_fu);
}
