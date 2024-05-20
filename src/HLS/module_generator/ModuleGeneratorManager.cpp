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
 * @file ModuleGeneratorManager.cpp
 * @brief
 *
 *
 *
 * @author Alessandro Nacci <alenacci@gmail.com>
 * @author Gianluca Durelli <durellinux@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ModuleGeneratorManager.hpp"

#include "ModuleGenerator.hpp"
#include "Parameter.hpp"
#include "area_info.hpp"
#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "constant_strings.hpp"
#include "fileIO.hpp"
#include "function_behavior.hpp"
#include "hls_device.hpp"
#include "hls_manager.hpp"
#include "language_writer.hpp"
#include "library_manager.hpp"
#include "math_function.hpp"
#include "memory.hpp"
#include "memory_cs.hpp"
#include "op_graph.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include "config_BOOST_INCLUDE_DIR.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

ModuleGeneratorManager::ModuleGeneratorManager(const HLS_managerRef _HLSMgr, const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr), parameters(_parameters), debug_level(_parameters->get_class_debug_level(GET_CLASS(*this)))
{
}

ModuleGeneratorManager::~ModuleGeneratorManager() = default;

#define NAMESEPARATOR "_"

structural_type_descriptorRef
ModuleGeneratorManager::getDataType(unsigned int variable, const FunctionBehaviorConstRef function_behavior) const
{
   return structural_type_descriptorRef(
       new structural_type_descriptor(variable, function_behavior->CGetBehavioralHelper()));
}

static unsigned long long resize_to_8_or_greater(unsigned long long value)
{
   return std::max(8ULL, ceil_pow2(value));
}

std::string ModuleGeneratorManager::get_specialized_name(
    unsigned int firstIndexToSpecialize, const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables,
    const FunctionBehaviorConstRef FB) const
{
   std::string fu_name = "";
   auto index = 0U;
   for(const auto& required_variable : required_variables)
   {
      if(index >= firstIndexToSpecialize)
      {
         const auto typeRef = getDataType(std::get<0>(required_variable), FB);
         const auto dataSize = typeRef->vector_size != 0U ? typeRef->vector_size : typeRef->size;
         fu_name += NAMESEPARATOR + typeRef->get_name() + STR(resize_to_8_or_greater(dataSize));
      }
      ++index;
   }
   return fu_name;
}

std::string ModuleGeneratorManager::GenerateHDL(
    const std::string& hdl_template, structural_objectRef mod, unsigned int function_id, vertex op_v,
    const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables, HDLWriter_Language language)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                 "ModuleGeneratorManager @ Loading generator class '" << hdl_template << "'...");

   const auto module_generator = ModuleGenerator::Create(hdl_template, HLSMgr);
   THROW_ASSERT(module_generator, "Unknown module generator required: " + hdl_template);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "ModuleGeneratorManager @ Starting dynamic HDL generation...");

   std::vector<ModuleGenerator::parameter> _p;
   if(required_variables.size())
   {
      auto portNum = 0U;
      const auto FB = HLSMgr->CGetFunctionBehavior(function_id);
      for(const auto& required_variable : required_variables)
      {
         const auto typeRef = getDataType(std::get<0>(required_variable), FB);
         struct ModuleGenerator::parameter param;
         param.name = "in" + STR(portNum + 1U);
         param.type = typeRef->get_name();
         const auto dataSize = typeRef->vector_size != 0U ? typeRef->vector_size : typeRef->size;
         param.type_size = resize_to_8_or_greater(dataSize);
         param.alignment = 0U;
         _p.push_back(std::move(param));
         portNum++;
      }
   }

   std::stringstream HDLOutput;
   module_generator->Exec(HDLOutput, mod, function_id, op_v, _p, language);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "ModuleGeneratorManager @ HDL code generated successfully!");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "ModuleGeneratorManager @ The generated Dynamic-HDL is:");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, HDLOutput.str());

   return HDLOutput.str();
}

void ModuleGeneratorManager::add_port_parameters(structural_objectRef gen_port, structural_objectRef original_port)
{
   original_port->copy(gen_port);
   gen_port->get_typeRef()->size = original_port->get_typeRef()->size;
   gen_port->get_typeRef()->vector_size = original_port->get_typeRef()->vector_size;
}

void ModuleGeneratorManager::specialize_fu(const std::string& fu_name, vertex ve, const FunctionBehaviorConstRef FB,
                                           const std::string& libraryId, const std::string& new_fu_name,
                                           std::map<std::string, technology_nodeRef>& new_fu)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specializing: " + fu_name + " as " + new_fu_name);

   if(new_fu.count(new_fu_name))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + " already in the library");
   }
   else
   {
      const auto HLS_D = HLSMgr->get_HLS_device();

      const auto libraryManager = HLS_D->get_technology_manager()->get_library_manager(libraryId);
      const auto techNode_obj = libraryManager->get_fu(fu_name);
      THROW_ASSERT(techNode_obj->get_kind() == functional_unit_K, "");
      const auto structManager_obj = GetPointerS<const functional_unit>(techNode_obj)->CM;
      const auto fu_obj = structManager_obj->get_circ();
      const auto fu_module = GetPointer<const module>(fu_obj);
      THROW_ASSERT(fu_module, "");
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Found variable component: " + fu_name);
      const auto required_variables =
          FB ? HLSMgr->get_required_values(FB->CGetBehavioralHelper()->get_function_index(), ve) :
               std::vector<HLS_manager::io_binding_type>();
      auto param_list = fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY);
      const auto multiplicitiy = fu_module->get_multi_unit_multiplicity();
      const auto n_ports = FB ? FB->GetChannelsNumber() : 1U;

      const structural_managerRef CM(new structural_manager(parameters));
      const structural_type_descriptorRef module_type(new structural_type_descriptor(new_fu_name));
      CM->set_top_info(new_fu_name, module_type);
      const auto top = CM->get_circ();
      const auto top_module = GetPointerS<module>(top);
      top_module->set_generated();
      /// add description and license
      top_module->set_description(fu_module->get_description());
      top_module->set_copyright(fu_module->get_copyright());
      top_module->set_authors(fu_module->get_authors());
      top_module->set_license(fu_module->get_license());
      for(const auto& module_parameter : fu_module->GetParameters())
      {
         top_module->AddParameter(module_parameter.first, fu_module->GetDefaultParameter(module_parameter.first));
         top_module->SetParameter(module_parameter.first, module_parameter.second);
      }
      top_module->set_multi_unit_multiplicity(multiplicitiy);
      if(fu_module->get_NP_functionality()->exist_NP_functionality(NP_functionality::IP_COMPONENT))
      {
         CM->add_NP_functionality(
             top, NP_functionality::IP_COMPONENT,
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
                  const auto dt = getDataType(var, FB);
                  /// normalize type
                  if(dt->vector_size == 0)
                  {
                     dt->size = resize_to_8_or_greater(dt->size);
                  }
                  else
                  {
                     dt->vector_size = resize_to_8_or_greater(dt->vector_size);
                  }

                  structural_objectRef gen_port;
                  if(curr_port->get_kind() == port_vector_o_K)
                  {
                     const auto ps = GetPointerS<port_o>(curr_port)->get_ports_size();
                     THROW_ASSERT(multiplicitiy == ps, "unexpected condition " + STR(multiplicitiy) + " " + STR(ps));
                     gen_port = CM->add_port_vector(gen_port_name, port_o::IN, ps, top, dt);
                  }
                  else
                  {
                     gen_port = CM->add_port(gen_port_name, port_o::IN, top, dt);
                  }
                  gen_port->get_typeRef()->size = dt->size;
                  gen_port->get_typeRef()->vector_size = dt->vector_size;
                  param_list = param_list + " " + gen_port_name;
                  ++portNum;
               }
               ++indexPort;
            }
         }
         else
         {
            structural_objectRef gen_port;
            if(curr_port->get_kind() == port_vector_o_K)
            {
               if(multiplicitiy)
               {
                  const auto ps = GetPointerS<const port_o>(curr_port)->get_ports_size();
                  THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
                  gen_port = CM->add_port_vector(port_name, port_o::IN, ps, top, curr_port->get_typeRef());
               }
               else
               {
                  gen_port = CM->add_port_vector(port_name, port_o::IN, n_ports, top, curr_port->get_typeRef());
               }
            }
            else
            {
               gen_port = CM->add_port(port_name, port_o::IN, top, curr_port->get_typeRef());
            }
            add_port_parameters(gen_port, curr_port);
         }
      }

      CM->add_NP_functionality(top, NP_functionality::LIBRARY, new_fu_name + " " + param_list);

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");
      for(auto i = 0U; i < fu_module->get_out_port_size(); i++)
      {
         const auto curr_port = fu_module->get_out_port(i);
         structural_objectRef gen_port;
         if(curr_port->get_kind() == port_vector_o_K)
         {
            if(multiplicitiy)
            {
               const auto ps = GetPointerS<const port_o>(curr_port)->get_ports_size();
               THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
               gen_port = CM->add_port_vector(curr_port->get_id(), port_o::OUT, ps, top, curr_port->get_typeRef());
            }
            else
            {
               gen_port = CM->add_port_vector(curr_port->get_id(), port_o::OUT, n_ports, top, curr_port->get_typeRef());
            }
         }
         else
         {
            gen_port = CM->add_port(curr_port->get_id(), port_o::OUT, top, curr_port->get_typeRef());
         }
         add_port_parameters(gen_port, curr_port);
      }

      const auto writer = [&]() -> HDLWriter_Language {
         /// default language
         const auto np = fu_module->get_NP_functionality();
         const auto required_language = parameters->getOption<HDLWriter_Language>(OPT_writer_language);
         if(required_language == HDLWriter_Language::VERILOG &&
            np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
         {
            return HDLWriter_Language::VERILOG;
         }
         if(required_language == HDLWriter_Language::VHDL &&
            np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
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
         if(np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
         {
            return HDLWriter_Language::VERILOG;
         }
         else
         {
            return HDLWriter_Language::VHDL;
         }
      }();

      const auto hdl_template = fu_module->get_NP_functionality()->get_NP_functionality(
          writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_GENERATOR :
                                                  NP_functionality::VHDL_GENERATOR);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + ": Generating dynamic HDL code");
      const auto hdl_code = GenerateHDL(hdl_template, top, FB ? FB->CGetBehavioralHelper()->get_function_index() : 0U,
                                        ve, required_variables, writer);

      CM->add_NP_functionality(top,
                               writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED :
                                                                       NP_functionality::VHDL_PROVIDED,
                               hdl_code);

      const technology_nodeRef new_techNode_obj(new functional_unit);
      {
         const auto fu = GetPointerS<functional_unit>(new_techNode_obj);
         if(GetPointerS<const functional_unit>(techNode_obj)->area_m)
         {
            fu->area_m = area_info::factory(parameters);
         }
         fu->functional_unit_name = new_fu_name;
         fu->CM = CM;
         const auto op_vec = GetPointerS<const functional_unit>(techNode_obj)->get_operations();
         for(const auto& techNode_fu : op_vec)
         {
            fu->add(techNode_fu);
         }
      }
      new_fu.insert(std::make_pair(new_fu_name, new_techNode_obj));
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + " created successfully");
   }
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specialization completed");
}

void ModuleGeneratorManager::create_generic_module(const std::string& fu_name, vertex ve,
                                                   const FunctionBehaviorConstRef FB, const std::string& libraryId,
                                                   const std::string& new_fu_name)
{
   const auto HLS_D = HLSMgr->get_HLS_device();
   const auto TechM = HLS_D->get_technology_manager();

   const auto libraryManager = TechM->get_library_manager(libraryId);
   const auto techNode_obj = libraryManager->get_fu(fu_name);
   THROW_ASSERT(techNode_obj->get_kind() == functional_unit_K, "");
   const auto structManager_obj = GetPointerS<const functional_unit>(techNode_obj)->CM;
   const auto fu_obj = structManager_obj->get_circ();
   const auto fu_module = GetPointer<const module>(fu_obj);
   THROW_ASSERT(fu_module, "");
   const auto multiplicitiy = fu_module->get_multi_unit_multiplicity();
   const auto n_ports = FB ? FB->GetChannelsNumber() : 1U;

   const structural_managerRef CM(new structural_manager(parameters));
   {
      TechM->add_resource(libraryId, new_fu_name, CM);
      const auto fu = GetPointerS<functional_unit>(TechM->get_fu(new_fu_name, libraryId));
      fu->area_m = area_info::factory(parameters);
      fu->area_m->set_area_value(0);
      const auto& op_vec = GetPointerS<functional_unit>(techNode_obj)->get_operations();
      for(const auto& techNode_fu : op_vec)
      {
         fu->add(techNode_fu);
      }
   }

   CM->set_top_info(new_fu_name, structural_type_descriptorRef(new structural_type_descriptor(new_fu_name)));
   const auto top = CM->get_circ();
   THROW_ASSERT(top, "");
   const auto top_module = GetPointerS<module>(top);
   top_module->set_generated();
   /// add description and license
   top_module->set_description(fu_module->get_description());
   top_module->set_copyright(fu_module->get_copyright());
   top_module->set_authors(fu_module->get_authors());
   top_module->set_license(fu_module->get_license());
   for(const auto& module_parameter : fu_module->GetParameters())
   {
      top_module->AddParameter(module_parameter.first, fu_module->GetDefaultParameter(module_parameter.first));
      top_module->SetParameter(module_parameter.first, module_parameter.second);
   }
   top_module->set_multi_unit_multiplicity(multiplicitiy);
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
      structural_objectRef gen_port;
      const auto port_name = curr_port->get_id();
      if(curr_port->get_kind() == port_vector_o_K)
      {
         if(multiplicitiy)
         {
            const auto ps = GetPointerS<const port_o>(curr_port)->get_ports_size();
            THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
            gen_port = CM->add_port_vector(port_name, port_o::IN, ps, top, curr_port->get_typeRef());
         }
         else
         {
            gen_port = CM->add_port_vector(port_name, port_o::IN, n_ports, top, curr_port->get_typeRef());
         }
      }
      else
      {
         gen_port = CM->add_port(port_name, port_o::IN, top, curr_port->get_typeRef());
      }
      add_port_parameters(gen_port, curr_port);
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");
   for(auto i = 0U; i < fu_module->get_out_port_size(); i++)
   {
      const auto curr_port = fu_module->get_out_port(i);
      structural_objectRef gen_port;
      if(curr_port->get_kind() == port_vector_o_K)
      {
         if(multiplicitiy)
         {
            const auto ps = GetPointerS<const port_o>(curr_port)->get_ports_size();
            THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
            gen_port = CM->add_port_vector(curr_port->get_id(), port_o::OUT, ps, top, curr_port->get_typeRef());
         }
         else
         {
            gen_port = CM->add_port_vector(curr_port->get_id(), port_o::OUT, n_ports, top, curr_port->get_typeRef());
         }
      }
      else
      {
         gen_port = CM->add_port(curr_port->get_id(), port_o::OUT, top, curr_port->get_typeRef());
      }
      add_port_parameters(gen_port, curr_port);
   }

   const auto writer = [&]() -> HDLWriter_Language {
      /// default language
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
      if(np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
      {
         return HDLWriter_Language::VERILOG;
      }
      else
      {
         return HDLWriter_Language::VHDL;
      }
   }();
   const auto hdl_template = fu_module->get_NP_functionality()->get_NP_functionality(
       writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_GENERATOR : NP_functionality::VHDL_GENERATOR);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + ": Generating dynamic HDL code");
   std::vector<std::tuple<unsigned int, unsigned int>> required_variables;
   const auto hdl_code = GenerateHDL(hdl_template, top, FB ? FB->CGetBehavioralHelper()->get_function_index() : 0U, ve,
                                     required_variables, writer);
   CM->add_NP_functionality(top,
                            writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED :
                                                                    NP_functionality::VHDL_PROVIDED,
                            hdl_code);
}
