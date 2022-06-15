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
 *              Copyright (C) 2004-2022 Politecnico di Milano
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
/// header include
#include "ModuleGeneratorManager.hpp"

/// Autoheader include
#include "config_BOOST_INCLUDE_DIR.hpp"

///. include
#include "Parameter.hpp"

/// behavior include
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// circuit includes
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// design_flows/backend/ToHDL
#include "language_writer.hpp"

/// HLS include
#include "hls_manager.hpp"

/// HLS/memory include
#include "memory.hpp"
#include "memory_cs.hpp"

/// STD includes
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <iosfwd>
#include <string>

/// STL include
#include <tuple>
#include <utility>
#include <vector>

/// technology include
#include "technology_manager.hpp"

/// technology/physical_library includes
#include "library_manager.hpp"
#include "technology_node.hpp"

/// technology/physical_library/models include
#include "area_model.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"
#include "tree_reindex.hpp"

#include "ModuleGenerator.hpp"
#include "call_graph_manager.hpp"
#include "hls_target.hpp"

#include "compiler_wrapper.hpp"
#include "constant_strings.hpp"
#include "fileIO.hpp"
#include "math_function.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

ModuleGeneratorManager::ModuleGeneratorManager(const HLS_managerRef _HLSMgr, const ParameterConstRef _parameters)
    : HLSMgr(_HLSMgr),
      parameters(_parameters),
      debug_level(_parameters->get_class_debug_level(GET_CLASS(*this))),
      output_directory(parameters->getOption<std::string>(OPT_output_directory) + "/module-generation/")
{
   if(!boost::filesystem::exists(output_directory))
   {
      boost::filesystem::create_directories(output_directory);
   }
}

ModuleGeneratorManager::~ModuleGeneratorManager() = default;

#define NAMESEPARATOR "_"

structural_type_descriptorRef
ModuleGeneratorManager::getDataType(unsigned int variable, const FunctionBehaviorConstRef function_behavior) const
{
   return structural_type_descriptorRef(
       new structural_type_descriptor(variable, function_behavior->CGetBehavioralHelper()));
}

static unsigned int resize_to_8_or_greater(unsigned int value)
{
   if(value < 8)
   {
      return 8;
   }
   else
   {
      return resize_to_1_8_16_32_64_128_256_512(value);
   }
}

std::string
ModuleGeneratorManager::get_specialized_name(unsigned int firstIndexToSpecialize,
                                             std::vector<std::tuple<unsigned int, unsigned int>>& required_variables,
                                             const FunctionBehaviorConstRef FB) const
{
   std::string fuName = "";
   unsigned int index = 0;
   for(auto& required_variable : required_variables)
   {
      if(index >= firstIndexToSpecialize)
      {
         unsigned int dataSize = getDataType(std::get<0>(required_variable), FB)->vector_size != 0 ?
                                     getDataType(std::get<0>(required_variable), FB)->vector_size :
                                     getDataType(std::get<0>(required_variable), FB)->size;
         structural_type_descriptorRef typeRef = getDataType(std::get<0>(required_variable), FB);
         fuName = fuName + NAMESEPARATOR + typeRef->get_name() + STR(resize_to_8_or_greater(dataSize));
      }
      ++index;
   }
   return fuName;
}

std::string ModuleGeneratorManager::GenerateHDL(
    const std::string& hdl_template, const module* mod, unsigned int function_id, vertex op_v,
    const std::vector<std::tuple<unsigned int, unsigned int>>& required_variables, HDLWriter_Language language)
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level,
                 "ModuleGeneratorManager @ Loading generator class '" << hdl_template << "'...");

   const auto module_generator = ModuleGenerator::Create(hdl_template, HLSMgr);
   THROW_ASSERT(module_generator, "Unknown module generator required: " + hdl_template);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "ModuleGeneratorManager @ Starting dynamic hdl generation...");

   std::vector<ModuleGenerator::parameter> _p;
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

void ModuleGeneratorManager::add_port_parameters(structural_objectRef generated_port,
                                                 structural_objectRef original_port)
{
   original_port->copy(generated_port);
   generated_port->get_typeRef()->size = original_port->get_typeRef()->size;
   generated_port->get_typeRef()->vector_size = original_port->get_typeRef()->vector_size;
}

void ModuleGeneratorManager::specialize_fu(std::string fuName, vertex ve, std::string libraryId,
                                           const FunctionBehaviorConstRef FB, std::string new_fu_name,
                                           std::map<std::string, technology_nodeRef>& new_fu)
{
   const auto HLS_T = HLSMgr->get_HLS_target();
   const auto dv_type = HLS_T->get_target_device()->get_type();

   const auto libraryManager = HLS_T->get_technology_manager()->get_library_manager(libraryId);
   const auto techNode_obj = libraryManager->get_fu(fuName);
   THROW_ASSERT(techNode_obj->get_kind() == functional_unit_K, "");
   const auto structManager_obj = GetPointerS<const functional_unit>(techNode_obj)->CM;
   const auto fu_obj = structManager_obj->get_circ();
   const auto fu_module = GetPointer<const module>(fu_obj);
   THROW_ASSERT(fu_module, "");
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Found variable component: " + fuName);
   const auto required_variables = HLSMgr->get_required_values(FB->CGetBehavioralHelper()->get_function_index(), ve);

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specializing: " + fuName + " as " + new_fu_name);

   if(new_fu.find(new_fu_name) != new_fu.end())
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + " already in the library");
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specialization completed");
   }
   else
   {
      structural_objectRef top;
      structural_managerRef CM;
      unsigned int n_ports =
          parameters->isOption(OPT_channels_number) ? parameters->getOption<unsigned int>(OPT_channels_number) : 0;

      std::string NP_parameters;

      // std::cout<<"Start creation"<<std::endl;

      CM = structural_managerRef(new structural_manager(parameters));
      structural_type_descriptorRef module_type =
          structural_type_descriptorRef(new structural_type_descriptor(new_fu_name));
      CM->set_top_info(new_fu_name, module_type);
      top = CM->get_circ();
      GetPointer<module>(top)->set_generated();
      /// add description and license
      GetPointer<module>(top)->set_description(fu_module->get_description());
      GetPointer<module>(top)->set_copyright(fu_module->get_copyright());
      GetPointer<module>(top)->set_authors(fu_module->get_authors());
      GetPointer<module>(top)->set_license(fu_module->get_license());
      for(const auto& module_parameter : fu_module->GetParameters())
      {
         GetPointer<module>(top)->AddParameter(module_parameter.first,
                                               fu_module->GetDefaultParameter(module_parameter.first));
         GetPointer<module>(top)->SetParameter(module_parameter.first, module_parameter.second);
      }
      auto multiplicitiy = fu_module->get_multi_unit_multiplicity();
      GetPointer<module>(top)->set_multi_unit_multiplicity(multiplicitiy);

      // std::cout<<"Module created, adding ports"<<std::endl;

      std::string param_list = fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY);

      /*Adding ports*/
      auto inPortSize = static_cast<unsigned int>(fu_module->get_in_port_size());
      auto outPortSize = static_cast<unsigned int>(fu_module->get_out_port_size());

      structural_objectRef generated_port;
      std::string port_name = "";
      unsigned int currentPort = 0;
      unsigned int toSkip = 0;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding input ports");
      for(currentPort = 0; currentPort < inPortSize; currentPort++)
      {
         structural_objectRef curr_port = fu_module->get_in_port(currentPort);
         if(port_name == CLOCK_PORT_NAME || port_name == RESET_PORT_NAME || port_name == START_PORT_NAME)
         {
            ++toSkip;
         }
         if(GetPointer<port_o>(curr_port)->get_is_var_args())
         {
            unsigned portNum = 1;
            unsigned indexPort = 0;
            for(auto& required_variable : required_variables)
            {
               if(indexPort >= (currentPort - toSkip))
               {
                  unsigned int var = std::get<0>(required_variable);
                  structural_type_descriptorRef dt = getDataType(var, FB);
                  /// normalize type
                  if(dt->vector_size == 0)
                  {
                     dt->size = resize_to_8_or_greater(dt->size);
                  }
                  else
                  {
                     dt->vector_size = resize_to_8_or_greater(dt->vector_size);
                  }

                  port_name = "in" + STR(portNum + currentPort - toSkip);
                  if(curr_port->get_kind() == port_vector_o_K)
                  {
                     auto ps = GetPointer<port_o>(curr_port)->get_ports_size();
                     THROW_ASSERT(multiplicitiy == ps, "unexpected condition " + STR(multiplicitiy) + " " + STR(ps));
                     generated_port = CM->add_port_vector(port_name, port_o::IN, ps, top, dt);
                  }
                  else
                  {
                     generated_port = CM->add_port(port_name, port_o::IN, top, dt);
                  }
                  generated_port->get_typeRef()->size = dt->size;
                  generated_port->get_typeRef()->vector_size = dt->vector_size;
                  param_list = param_list + " " + port_name;
                  portNum++;
               }
               ++indexPort;
               // std::cout<<"Added port NAME: "<<generated_port->get_id()<<" TYPE:
               // "<<generated_port->get_typeRef()->get_name()<<" CLOCK:
               // "<<GetPointer<port_o>(generated_port)->get_is_clock()<<"
               // DATA_SIZE:"<<STR(generated_port->get_typeRef()->size)<<"
               // VECTOR_SIZE:"<<STR(generated_port->get_typeRef()->vector_size)<<std::endl;
            }
         }
         else
         {
            port_name = curr_port->get_id();
            if(curr_port->get_kind() == port_vector_o_K)
            {
               if(multiplicitiy)
               {
                  auto ps = GetPointer<port_o>(curr_port)->get_ports_size();
                  THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
                  generated_port = CM->add_port_vector(port_name, port_o::IN, ps, top, curr_port->get_typeRef());
               }
               else
               {
                  generated_port = CM->add_port_vector(port_name, port_o::IN, n_ports, top, curr_port->get_typeRef());
               }
            }
            else
            {
               generated_port = CM->add_port(port_name, port_o::IN, top, curr_port->get_typeRef());
            }
            add_port_parameters(generated_port, curr_port);
            // std::cout<<"Added port NAME: "<<generated_port->get_id()<<" TYPE:
            // "<<generated_port->get_typeRef()->get_name()<<" CLOCK:
            // "<<GetPointer<port_o>(generated_port)->get_is_clock()<<"
            // DATA_SIZE:"<<STR(generated_port->get_typeRef()->size)<<"
            // VECTOR_SIZE:"<<STR(generated_port->get_typeRef()->vector_size)<<std::endl;
         }
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");

      for(currentPort = 0; currentPort < outPortSize; currentPort++)
      {
         structural_objectRef curr_port = fu_module->get_out_port(currentPort);
         if(curr_port->get_kind() == port_vector_o_K)
         {
            if(multiplicitiy)
            {
               auto ps = GetPointer<port_o>(curr_port)->get_ports_size();
               THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
               generated_port =
                   CM->add_port_vector(curr_port->get_id(), port_o::OUT, ps, top, curr_port->get_typeRef());
            }
            else
            {
               generated_port =
                   CM->add_port_vector(curr_port->get_id(), port_o::OUT, n_ports, top, curr_port->get_typeRef());
            }
         }
         else
         {
            generated_port = CM->add_port(curr_port->get_id(), port_o::OUT, top, curr_port->get_typeRef());
         }
         add_port_parameters(generated_port, curr_port);
      }

      NP_parameters = new_fu_name + std::string(" ") + param_list;
      CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
      if(fu_module->get_NP_functionality()->exist_NP_functionality(NP_functionality::IP_COMPONENT))
      {
         CM->add_NP_functionality(
             top, NP_functionality::IP_COMPONENT,
             fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::IP_COMPONENT));
      }

      const auto np = fu_module->get_NP_functionality();
      const auto writer = [&]() -> HDLWriter_Language {
         /// default language
         const auto required_language =
             static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
         if(required_language == HDLWriter_Language::VERILOG and
            np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR))
         {
            return HDLWriter_Language::VERILOG;
         }
         if(required_language == HDLWriter_Language::VHDL and
            np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
         {
            return HDLWriter_Language::VHDL;
         }
         if(parameters->isOption(OPT_mixed_design) && not parameters->getOption<bool>(OPT_mixed_design))
         {
            THROW_ERROR("Missing VHDL GENERATOR for " + fuName);
         }
         if(not np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) and
            not np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
         {
            THROW_ERROR("Missing GENERATOR for " + fuName);
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

      std::string hdl_template = fu_module->get_NP_functionality()->get_NP_functionality(
          writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_GENERATOR :
                                                  NP_functionality::VHDL_GENERATOR);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + ": Generating dynamic hdl code");
      std::string hdl_code =
          GenerateHDL(hdl_template, GetPointer<module>(top), FB->CGetBehavioralHelper()->get_function_index(), ve,
                      required_variables, writer);

      CM->add_NP_functionality(top,
                               writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED :
                                                                       NP_functionality::VHDL_PROVIDED,
                               hdl_code);

      technology_nodeRef new_techNode_obj = technology_nodeRef(new functional_unit);
      if(GetPointer<functional_unit>(techNode_obj)->area_m)
      {
         GetPointer<functional_unit>(new_techNode_obj)->area_m = area_model::create_model(dv_type, parameters);
      }
      GetPointer<functional_unit>(new_techNode_obj)->functional_unit_name = new_fu_name;
      GetPointer<functional_unit>(new_techNode_obj)->CM = CM;

      new_fu.insert(std::make_pair(new_fu_name, new_techNode_obj));

      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, new_fu_name + " created successfully");

      std::vector<technology_nodeRef> op_vec = GetPointer<functional_unit>(techNode_obj)->get_operations();
      for(auto techNode_fu : op_vec)
      {
         GetPointer<functional_unit>(new_techNode_obj)->add(techNode_fu);
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Specialization completed");
   }
}

void ModuleGeneratorManager::create_generic_module(const std::string& fuName, const std::string& libraryId,
                                                   const std::string& new_fu_name)
{
   const auto HLS_T = HLSMgr->get_HLS_target();
   const auto TechM = HLS_T->get_technology_manager();
   const auto dv_type = HLS_T->get_target_device()->get_type();

   const auto libraryManager = TechM->get_library_manager(libraryId);
   const auto techNode_obj = libraryManager->get_fu(fuName);
   THROW_ASSERT(techNode_obj->get_kind() == functional_unit_K, "");
   const auto structManager_obj = GetPointerS<const functional_unit>(techNode_obj)->CM;
   const auto fu_obj = structManager_obj->get_circ();
   const auto fu_module = GetPointer<const module>(fu_obj);
   THROW_ASSERT(fu_module, "");
   structural_objectRef top;
   structural_managerRef CM;
   const auto n_ports =
       parameters->isOption(OPT_channels_number) ? parameters->getOption<unsigned int>(OPT_channels_number) : 0;

   CM = structural_managerRef(new structural_manager(parameters));
   structural_type_descriptorRef module_type =
       structural_type_descriptorRef(new structural_type_descriptor(new_fu_name));
   CM->set_top_info(new_fu_name, module_type);
   top = CM->get_circ();
   GetPointer<module>(top)->set_generated();
   /// add description and license
   GetPointer<module>(top)->set_description(fu_module->get_description());
   GetPointer<module>(top)->set_copyright(fu_module->get_copyright());
   GetPointer<module>(top)->set_authors(fu_module->get_authors());
   GetPointer<module>(top)->set_license(fu_module->get_license());
   for(const auto& module_parameter : fu_module->GetParameters())
   {
      GetPointer<module>(top)->AddParameter(module_parameter.first,
                                            fu_module->GetDefaultParameter(module_parameter.first));
      GetPointer<module>(top)->SetParameter(module_parameter.first, module_parameter.second);
   }
   auto multiplicitiy = fu_module->get_multi_unit_multiplicity();
   GetPointer<module>(top)->set_multi_unit_multiplicity(multiplicitiy);

   std::string param_list = fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::LIBRARY);

   /*Adding ports*/
   auto inPortSize = static_cast<unsigned int>(fu_module->get_in_port_size());
   auto outPortSize = static_cast<unsigned int>(fu_module->get_out_port_size());

   const FunctionBehaviorConstRef FB;
   structural_objectRef generated_port;
   std::string port_name = "";
   unsigned int currentPort = 0;
   unsigned int toSkip = 0;
   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding input ports");
   for(currentPort = 0; currentPort < inPortSize; currentPort++)
   {
      structural_objectRef curr_port = fu_module->get_in_port(currentPort);
      if(port_name == CLOCK_PORT_NAME || port_name == RESET_PORT_NAME || port_name == START_PORT_NAME)
      {
         ++toSkip;
      }
      THROW_ASSERT(!GetPointer<port_o>(curr_port)->get_is_var_args(), "unexpected condition");
      port_name = curr_port->get_id();
      if(curr_port->get_kind() == port_vector_o_K)
      {
         if(multiplicitiy)
         {
            auto ps = GetPointer<port_o>(curr_port)->get_ports_size();
            THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
            generated_port = CM->add_port_vector(port_name, port_o::IN, ps, top, curr_port->get_typeRef());
         }
         else
         {
            generated_port = CM->add_port_vector(port_name, port_o::IN, n_ports, top, curr_port->get_typeRef());
         }
      }
      else
      {
         generated_port = CM->add_port(port_name, port_o::IN, top, curr_port->get_typeRef());
      }
      add_port_parameters(generated_port, curr_port);
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Adding output ports");

   for(currentPort = 0; currentPort < outPortSize; currentPort++)
   {
      structural_objectRef curr_port = fu_module->get_out_port(currentPort);
      if(curr_port->get_kind() == port_vector_o_K)
      {
         if(multiplicitiy)
         {
            auto ps = GetPointer<port_o>(curr_port)->get_ports_size();
            THROW_ASSERT(multiplicitiy == ps, "unexpected condition");
            generated_port = CM->add_port_vector(curr_port->get_id(), port_o::OUT, ps, top, curr_port->get_typeRef());
         }
         else
         {
            generated_port =
                CM->add_port_vector(curr_port->get_id(), port_o::OUT, n_ports, top, curr_port->get_typeRef());
         }
      }
      else
      {
         generated_port = CM->add_port(curr_port->get_id(), port_o::OUT, top, curr_port->get_typeRef());
      }
      add_port_parameters(generated_port, curr_port);
   }

   const auto NP_parameters = new_fu_name + std::string(" ") + param_list;
   CM->add_NP_functionality(top, NP_functionality::LIBRARY, NP_parameters);
   if(fu_module->get_NP_functionality()->exist_NP_functionality(NP_functionality::IP_COMPONENT))
   {
      CM->add_NP_functionality(top, NP_functionality::IP_COMPONENT,
                               fu_module->get_NP_functionality()->get_NP_functionality(NP_functionality::IP_COMPONENT));
   }

   const auto np = fu_module->get_NP_functionality();
   const auto writer = [&]() -> HDLWriter_Language {
      /// default language
      const auto required_language =
          static_cast<HDLWriter_Language>(parameters->getOption<unsigned int>(OPT_writer_language));
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
         THROW_ERROR("Missing VHDL GENERATOR for " + fuName);
      }
      if(!np->exist_NP_functionality(NP_functionality::VERILOG_GENERATOR) &&
         !np->exist_NP_functionality(NP_functionality::VHDL_GENERATOR))
      {
         THROW_ERROR("Missing GENERATOR for " + fuName);
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
   const auto hdl_code =
       GenerateHDL(hdl_template, GetPointer<module>(top), FB->CGetBehavioralHelper()->get_function_index(), nullptr,
                   required_variables, writer);
   CM->add_NP_functionality(top,
                            writer == HDLWriter_Language::VERILOG ? NP_functionality::VERILOG_PROVIDED :
                                                                    NP_functionality::VHDL_PROVIDED,
                            hdl_code);

   technology_nodeRef new_techNode_obj = technology_nodeRef(new functional_unit);
   GetPointer<functional_unit>(new_techNode_obj)->functional_unit_name = new_fu_name;
   GetPointer<functional_unit>(new_techNode_obj)->CM = CM;
   TechM->add_resource(libraryId, new_fu_name, CM);
   auto* fu = GetPointer<functional_unit>(TechM->get_fu(new_fu_name, libraryId));
   fu->area_m = area_model::create_model(dv_type, parameters);
   fu->area_m->set_area_value(0);
   const auto& op_vec = GetPointer<functional_unit>(techNode_obj)->get_operations();
   for(auto techNode_fu : op_vec)
   {
      fu->add(techNode_fu);
   }
}
