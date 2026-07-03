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
 * @file HDLGeneratorManager.cpp
 * @brief Implementation of the manager responsible for instantiating and driving HDL generators.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#include "HDLGeneratorManager.hpp"

#include "HDLGenerator.hpp"
#include "Parameter.hpp"
#include "area_info.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "ir_helper.hpp"
#include "ir_manager.hpp"
#include "ir_node.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "var_pp_functor.hpp"

#include <filesystem>
#include <iosfwd>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "kmp_bambu_names.h"

#define VAL(str) #str
#define TOSTRING(str) VAL(str)

namespace
{
   void copy_generated_module_metadata(module_o* target_module, const module_o* source_module,
                                       const unsigned int multiplicity)
   {
      target_module->set_generated();
      target_module->set_description(source_module->get_description());
      target_module->set_copyright(source_module->get_copyright());
      target_module->set_authors(source_module->get_authors());
      target_module->set_license(source_module->get_license());
      for(const auto& module_parameter : source_module->GetParameters())
      {
         target_module->AddParameter(module_parameter.first,
                                     source_module->GetDefaultParameter(module_parameter.first));
         target_module->SetParameter(module_parameter.first, module_parameter.second);
      }
      target_module->set_multi_unit_multiplicity(multiplicity);
   }

   structural_objectRef create_generated_port(const structural_managerRef& CM, const structural_objectRef& top,
                                              const structural_objectRef& source_port,
                                              const port_o::port_direction direction, const unsigned int multiplicity,
                                              const unsigned int n_ports,
                                              const structural_type_descriptorRef& type_descr = nullptr,
                                              const std::string& port_name = "")
   {
      const auto effective_name = port_name.empty() ? source_port->get_id() : port_name;
      const auto effective_type = type_descr ? type_descr : source_port->get_typeRef();
      if(source_port->get_kind() == port_vector_o_K)
      {
         if(multiplicity)
         {
            const auto ports_size = GetPointerS<const port_o>(source_port)->get_ports_size();
            THROW_ASSERT(multiplicity == ports_size,
                         "unexpected condition " + STR(multiplicity) + " " + STR(ports_size));
            return CM->add_port_vector(effective_name, direction, ports_size, top, effective_type);
         }
         return CM->add_port_vector(effective_name, direction, n_ports, top, effective_type);
      }
      return CM->add_port(effective_name, direction, top, effective_type);
   }

   HDLWriter_Language select_writer_language(const ParameterConstRef& parameters, const std::string& fu_name,
                                             const module_o* fu_module)
   {
      const auto np = fu_module->get_NP_functionality();
      const auto required_language = parameters->getOption<HDLWriter_Language>(OPT_writer_language);
      if(required_language == HDLWriter_Language::VERILOG &&
         np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
      {
         return HDLWriter_Language::VERILOG;
      }
      if(required_language == HDLWriter_Language::VHDL && np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
      {
         return HDLWriter_Language::VHDL;
      }
      if(parameters->isOption(OPT_mixed_design) && !parameters->getOption<bool>(OPT_mixed_design))
      {
         THROW_ERROR("Missing VHDL GENERATOR for " + fu_name);
      }
      if(!np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) &&
         !np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
      {
         THROW_ERROR("Missing GENERATOR for " + fu_name);
      }
      return np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) ? HDLWriter_Language::VERILOG :
                                                                               HDLWriter_Language::VHDL;
   }
} // namespace

HDLGeneratorManager::HDLGeneratorManager(const HLS_managerRef _HLSMgr, const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr), parameters(_parameters), debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

std::string HDLGeneratorManager::getModuleNameSuffix(
    unsigned int firstIndexToSpecialize,
    const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables) const
{
   static const std::regex invalid_module_name_chars("[^[:alnum:]_]+");
   std::string fu_suffix = "";
   const auto IRM = HLSMgr->get_ir_manager();
   auto index = 0U;
   for(const auto& required_variable : required_variables)
   {
      if(index >= firstIndexToSpecialize)
      {
         auto cur_var_node = IRM->GetIRNode(std::get<0>(required_variable));
         auto cur_var_type = ir_helper::CGetType(cur_var_node);
         auto cur_var_typesize = ir_helper::Size(cur_var_type);
         auto cur_var_type_name = ir_helper::NormalizeTypename(ir_helper::PrintType(cur_var_type));
         cur_var_type_name = std::regex_replace(cur_var_type_name, invalid_module_name_chars, "_");
         fu_suffix += "_" + cur_var_type_name + STR(resize_1_8_pow2(cur_var_typesize));
      }
      ++index;
   }
   return fu_suffix;
}

std::string
HDLGeneratorManager::GenerateHDL(const std::string& hdl_template, structural_objectRef mod,
                                 const FunctionBehaviorConstRef FB, gc_vertex_descriptor op_v,
                                 const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables,
                                 HDLWriter_Language language)
{
   THROW_ASSERT(FB, "Expected valid FunctionBehavior");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                 "HDLGeneratorManager @ Loading generator class '" << hdl_template << "'...");

   const auto module_generator = HDLGenerator::Create(hdl_template, HLSMgr);
   THROW_ASSERT(module_generator, "Unknown module generator required: " + hdl_template);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "HDLGeneratorManager @ Starting dynamic HDL generation...");

   const auto behavioral_helper = FB->CGetBehavioralHelper();
   std::vector<HDLGenerator::parameter> _p;
   if(required_variables.size())
   {
      auto portNum = 0U;
      for(const auto& required_variable : required_variables)
      {
         const auto typeRef = structural_type_descriptorRef(
             new structural_type_descriptor(std::get<0>(required_variable), behavioral_helper));
         struct HDLGenerator::parameter param;
         param.name = "in" + STR(portNum + 1U);
         param.type = typeRef->get_name();
         const auto dataSize = typeRef->vector_size != 0U ? typeRef->vector_size : typeRef->size;
         param.type_size = resize_1_8_pow2(dataSize);
         param.alignment = 0U;
         _p.push_back(std::move(param));
         portNum++;
      }
   }

   std::stringstream HDLOutput;
   module_generator->Exec(HDLOutput, mod, behavioral_helper->get_function_index(), op_v, _p, language);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "HDLGeneratorManager @ HDL code generated successfully!");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "HDLGeneratorManager @ The generated Dynamic-HDL is:");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, HDLOutput.str());

   return HDLOutput.str();
}

technology_nodeRef HDLGeneratorManager::specialize_fu(const std::string& fu_name, gc_vertex_descriptor ve,
                                                      const FunctionBehaviorConstRef FB, const std::string& libraryId,
                                                      const std::string& new_fu_name)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specializing: " + fu_name + " as " + new_fu_name);

   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto behavioral_helper = FB->CGetBehavioralHelper();
   const auto required_variables = HLSMgr->get_required_values(behavioral_helper->get_function_index(), ve);
   const auto libraryManager = HLS_D->get_technology_manager()->get_library_manager(libraryId);
   const auto techNode_obj = libraryManager->get_fu(fu_name);
   THROW_ASSERT(techNode_obj->get_kind() == functional_unit_K, "");
   const auto structManager_obj = GetPointerS<const functional_unit>(techNode_obj)->CM;
   const auto fu_obj = structManager_obj->get_circ();
   const auto fu_module = GetPointer<const module_o>(fu_obj);
   THROW_ASSERT(fu_module, "");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Found variable component: " + fu_name);
   auto param_list = fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY);
   const auto multiplicitiy = fu_module->get_multi_unit_multiplicity();
   const auto n_ports = FB->GetChannelsNumber();

   const structural_managerRef CM(new structural_manager(parameters));
   const structural_type_descriptorRef module_type(new structural_type_descriptor(new_fu_name));
   CM->set_top_info(new_fu_name, module_type);
   const auto top = CM->get_circ();
   const auto top_module = GetPointerS<module_o>(top);
   copy_generated_module_metadata(top_module, fu_module, multiplicitiy);
   if(fu_module->get_NP_functionality()->exist_NP_functionality(NP_functionality::IP_COMPONENT))
   {
      CM->add_NP_functionality(top, NP_functionality::IP_COMPONENT,
                               fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::IP_COMPONENT));
   }

   auto toSkip = 0U;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding input ports");
   for(auto i = 0U; i < fu_module->get_in_port_size(); i++)
   {
      const auto curr_port = fu_module->get_in_port(i);
      const auto port_name = curr_port->get_id();
      if(port_name == CLOCK_PORT_NAME || port_name == RESET_PORT_NAME || port_name == START_PORT_NAME)
      {
         ++toSkip;
      }
      if(GetPointerS<port_o>(curr_port)->get_is_var_args())
      {
         auto portNum = 1U;
         auto indexPort = 0U;
         for(const auto& required_variable : required_variables)
         {
            if(indexPort >= (i - toSkip))
            {
               const auto gen_port_name = "in" + STR(portNum + i - toSkip);
               const auto var = std::get<0>(required_variable);
               const auto dt = structural_type_descriptorRef(new structural_type_descriptor(var, behavioral_helper));
               if(dt->vector_size == 0)
               {
                  dt->size = resize_1_8_pow2(dt->size);
               }
               else
               {
                  dt->vector_size = resize_1_8_pow2(dt->vector_size);
               }

               create_generated_port(CM, top, curr_port, port_o::IN, multiplicitiy, n_ports, dt, gen_port_name);
               param_list += " " + gen_port_name;
               ++portNum;
            }
            ++indexPort;
         }
      }
      else
      {
         const auto gen_port = create_generated_port(CM, top, curr_port, port_o::IN, multiplicitiy, n_ports);
         curr_port->copy(gen_port);
      }
   }

   CM->add_NP_functionality(top, NP_functionality::LIBRARY, new_fu_name + " " + param_list);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");
   for(auto i = 0U; i < fu_module->get_out_port_size(); i++)
   {
      const auto curr_port = fu_module->get_out_port(i);
      const auto gen_port = create_generated_port(CM, top, curr_port, port_o::OUT, multiplicitiy, n_ports);
      curr_port->copy(gen_port);
   }
   const auto cfg = FB->GetOpGraph(FunctionBehavior::CFG);
   if(cfg.CGetNodeInfo(ve).GetOperation() == TOSTRING(KMP_FORK_CALL))
   {
      const auto TM = HLSMgr->get_ir_manager();
      const auto TechM = HLS_D->get_technology_manager();
      const auto omp_lambda_addr = *(++(cfg.CGetNodeInfo(ve).actual_parameters.begin()));
      const auto omp_lambda_fnode = ir_helper::GetBaseVariable(TM->GetIRNode(omp_lambda_addr));
      const auto omp_lambda_function_name = ir_helper::GetFunctionName(omp_lambda_fnode);

      const auto curr_IP_COMPONENT =
          top_module->get_NP_functionality()->exist_NP_functionality(NP_functionality::IP_COMPONENT) ?
              omp_lambda_function_name + "," +
                  top_module->get_NP_functionality()->get_NP_functionality(NP_functionality::IP_COMPONENT) :
              omp_lambda_function_name;
      CM->add_NP_functionality(top, NP_functionality::IP_COMPONENT, curr_IP_COMPONENT);
      const auto omp_lamba_library = TechM->get_library(omp_lambda_function_name);
      const auto omp_lamba_Manager = TechM->get_library_manager(omp_lamba_library);
      const auto omp_lamba_techNode_obj = omp_lamba_Manager->get_fu(omp_lambda_function_name);
      const auto omp_lamba_structManager_obj = GetPointer<functional_unit>(omp_lamba_techNode_obj)->CM;
      const auto omp_lamba_fu_obj = omp_lamba_structManager_obj->get_circ();
      const auto omp_lamba_fu_module = GetPointer<module_o>(omp_lamba_fu_obj);

      for(auto j = 0U; j < omp_lamba_fu_module->get_in_port_size(); j++)
      {
         const auto port_in = omp_lamba_fu_module->get_in_port(j);
         const auto port_obj = GetPointer<port_o>(port_in);
         if(port_obj->get_is_memory() && !port_obj->get_is_slave())
         {
            THROW_ASSERT(port_in->get_kind() == port_vector_o_K || n_ports == 1,
                         "unexpected condition: " + STR(n_ports));
            const auto ext_port =
                n_ports == 1 ?
                    CM->add_port(port_obj->get_id(), port_o::IN, top, port_in->get_typeRef()) :
                    CM->add_port_vector(port_obj->get_id(), port_o::IN, n_ports, top, port_in->get_typeRef());
            port_o::fix_port_properties(port_in, ext_port);
         }
      }
      for(auto j = 0U; j < omp_lamba_fu_module->get_out_port_size(); j++)
      {
         const auto port_out = omp_lamba_fu_module->get_out_port(j);
         const auto port_obj = GetPointer<port_o>(port_out);
         if(port_obj->get_is_memory() && !port_obj->get_is_slave())
         {
            THROW_ASSERT(port_out->get_kind() == port_vector_o_K || n_ports == 1,
                         "unexpected condition: " + STR(n_ports));
            const auto ext_port =
                n_ports == 1 ?
                    CM->add_port(port_obj->get_id(), port_o::OUT, top, port_out->get_typeRef()) :
                    CM->add_port_vector(port_obj->get_id(), port_o::OUT, n_ports, top, port_out->get_typeRef());
            port_o::fix_port_properties(port_out, ext_port);
         }
      }
      memory::propagate_memory_parameters(omp_lamba_fu_obj, CM);
   }

   const auto writer = select_writer_language(parameters, fu_name, fu_module);
   const auto hdl_template = fu_module->get_NP_functionality()->get_NP_functionality(
       writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_GENERATOR : NP_functionality::VHDL_GENERATOR);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + ": Generating dynamic HDL code");
   const auto hdl_code = GenerateHDL(hdl_template, top, FB, ve, required_variables, writer);

   CM->add_NP_functionality(top,
                            writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED :
                                                                    NP_functionality::VHDL_PROVIDED,
                            hdl_code);

   const technology_nodeRef new_techNode_obj(new functional_unit);
   {
      const auto fu = GetPointerS<functional_unit>(new_techNode_obj);
      if(GetPointerS<const functional_unit>(techNode_obj)->area_m)
      {
         fu->area_m = std::make_shared<area_info>();
      }
      fu->functional_unit_name = new_fu_name;
      fu->CM = CM;
      const auto op_vec = GetPointerS<const functional_unit>(techNode_obj)->get_operations();
      for(const auto& techNode_fu : op_vec)
      {
         fu->add(techNode_fu);
      }
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + " created successfully");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specialization completed");
   return new_techNode_obj;
}

technology_nodeRef HDLGeneratorManager::create_generic_module(const std::string& fu_name, gc_vertex_descriptor ve,
                                                              const FunctionBehaviorConstRef FB,
                                                              const std::string& libraryId,
                                                              const std::string& new_fu_name)
{
   THROW_ASSERT(FB, "Expected valid FunctionBehavior");
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechM = HLS_D->get_technology_manager();

   const auto libraryManager = TechM->get_library_manager(libraryId);
   const auto techNode_obj = libraryManager->get_fu(fu_name);
   THROW_ASSERT(techNode_obj->get_kind() == functional_unit_K, "");
   const auto structManager_obj = GetPointerS<const functional_unit>(techNode_obj)->CM;
   const auto fu_obj = structManager_obj->get_circ();
   const auto fu_module = GetPointer<const module_o>(fu_obj);
   THROW_ASSERT(fu_module, "");
   const auto multiplicitiy = fu_module->get_multi_unit_multiplicity();
   const auto behavioral_helper = FB->CGetBehavioralHelper();
   const auto n_ports = FB->GetChannelsNumber();

   const structural_managerRef CM(new structural_manager(parameters));
   technology_nodeRef new_techNode_obj;
   {
      TechM->add_resource(libraryId, new_fu_name, CM);
      new_techNode_obj = TechM->get_fu(new_fu_name, libraryId);
      const auto fu = GetPointerS<functional_unit>(new_techNode_obj);
      fu->area_m = std::make_shared<area_info>();
      fu->area_m->resources[area_info::AREA] = 0;
      const auto& op_vec = GetPointerS<functional_unit>(techNode_obj)->get_operations();
      for(const auto& techNode_fu : op_vec)
      {
         fu->add(techNode_fu);
      }
   }

   CM->set_top_info(new_fu_name, structural_type_descriptorRef(new structural_type_descriptor(new_fu_name)));
   const auto top = CM->get_circ();
   THROW_ASSERT(top, "");
   const auto top_module = GetPointerS<module_o>(top);
   copy_generated_module_metadata(top_module, fu_module, multiplicitiy);
   const auto NP_parameters =
       new_fu_name + " " + fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY);
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   if(fu_module->get_NP_functionality()->exist_NP_functionality(NP_functionality::IP_COMPONENT))
   {
      CM->add_NP_functionality(top, NP_functionality::IP_COMPONENT,
                               fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::IP_COMPONENT));
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding input ports");
   for(auto i = 0U; i < fu_module->get_in_port_size(); i++)
   {
      const auto curr_port = fu_module->get_in_port(i);
      THROW_ASSERT(!GetPointer<const port_o>(curr_port)->get_is_var_args(), "unexpected condition");
      const auto gen_port = create_generated_port(CM, top, curr_port, port_o::IN, multiplicitiy, n_ports);
      curr_port->copy(gen_port);
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");
   for(auto i = 0U; i < fu_module->get_out_port_size(); i++)
   {
      const auto curr_port = fu_module->get_out_port(i);
      const auto gen_port = create_generated_port(CM, top, curr_port, port_o::OUT, multiplicitiy, n_ports);
      curr_port->copy(gen_port);
   }

   const auto writer = select_writer_language(parameters, fu_name, fu_module);
   const auto hdl_template = fu_module->get_NP_functionality()->get_NP_functionality(
       writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_GENERATOR : NP_functionality::VHDL_GENERATOR);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + ": Generating dynamic HDL code");
   std::vector<std::tuple<unsigned int, unsigned int>> required_variables;
   const auto hdl_code = GenerateHDL(hdl_template, top, FB, ve, required_variables, writer);
   CM->add_NP_functionality(top,
                            writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED :
                                                                    NP_functionality::VHDL_PROVIDED,
                            hdl_code);
   return new_techNode_obj;
}
