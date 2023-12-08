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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file technology_node.cpp
 * @brief Class implementation of the technology node description.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "technology_node.hpp"
#include "Parameter.hpp"
#include "area_info.hpp"
#include "config_HAVE_CIRCUIT_BUILT.hpp"
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "exceptions.hpp"
#include "library_manager.hpp"
#include "polixml.hpp"
#include "string_manipulation.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"
#include "time_info.hpp"
#include "xml_helper.hpp"
#include <algorithm>
#include <list>
#include <utility>

simple_indent technology_node::PP('[', ']', 3);

technology_node::technology_node() = default;

technology_node::~technology_node() = default;

operation::operation() : commutative(false), bounded(true), primary_inputs_registered(false)
{
}

operation::~operation() = default;

void operation::xload(const xml_element* Enode, const technology_nodeRef fu, const ParameterConstRef Param)
{
   THROW_ASSERT(CE_XVM(operation_name, Enode), "An operation must have a name");
   /// name of the operation
   LOAD_XVM(operation_name, Enode);
   /// commutative property
   if(CE_XVM(commutative, Enode))
   {
      LOAD_XVM(commutative, Enode);
   }

   if(CE_XVM(bounded, Enode))
   {
      LOAD_XVM(bounded, Enode);
   }

   if(CE_XVM(primary_inputs_registered, Enode))
   {
      LOAD_XVM(primary_inputs_registered, Enode);
   }

   /// parsing of supported types
   if(CE_XVM(supported_types, Enode))
   {
      std::string supported_types_string;
      LOAD_XVFM(supported_types_string, Enode, supported_types);
      std::vector<std::string> types = SplitString(supported_types_string, "|");
      for(const auto& type : types)
      {
         if(type == "")
         {
            THROW_ERROR("wrong XML syntax for supported_types attribute: null type description in \"" +
                        supported_types_string + "\" [" + operation_name + "]");
         }
         std::string type_name;
         std::vector<unsigned int> type_precs;
         std::vector<std::string> type_name_to_precs = SplitString(type, ":");
         if(type_name_to_precs.size() != 2)
         {
            THROW_ERROR("wrong XML syntax for supported_types attribute around \":\" \"" + type + "\" [" +
                        operation_name + "]");
         }
         type_name = type_name_to_precs[0];
         if(type_name == "")
         {
            THROW_ERROR("wrong XML syntax for supported_types attribute: missing the supported type - \"" + type +
                        "\" [" + operation_name + "]");
         }

         if(type_name_to_precs[1] != "*")
         {
            std::vector<std::string> precs = SplitString(type_name_to_precs[1], ",");
            ;
            for(auto single_prec = precs.begin(); single_prec != precs.end() && *single_prec != ""; ++single_prec)
            {
               auto type_uint = static_cast<unsigned>(std::stoul(*single_prec));
               type_precs.push_back(type_uint);
            }
         }
         supported_types.insert(std::make_pair(type_name, type_precs));
      }
   }
   if(CE_XVM(pipe_parameters, Enode))
   {
      LOAD_XVM(pipe_parameters, Enode);
   }
   if(CE_XVM(portsize_parameters, Enode))
   {
      LOAD_XVM(portsize_parameters, Enode);
   }

   /// time characterization
   time_m = time_info::factory(Param);
   double execution_time = time_info::execution_time_DEFAULT;
   auto initiation_time = from_strongtype_cast<unsigned int>(time_info::initiation_time_DEFAULT);
   unsigned int cycles = time_info::cycles_time_DEFAULT;
   double stage_period = time_info::stage_period_DEFAULT;
   if(CE_XVM(execution_time, Enode))
   {
      LOAD_XVM(execution_time, Enode);
   }
   if(CE_XVM(initiation_time, Enode))
   {
      LOAD_XVM(initiation_time, Enode);
   }
   if(CE_XVM(cycles, Enode))
   {
      LOAD_XVM(cycles, Enode);
   }
   if(CE_XVM(stage_period, Enode))
   {
      LOAD_XVM(stage_period, Enode);
   }
   bool synthesis_dependent = false;
   if(CE_XVM(synthesis_dependent, Enode))
   {
      LOAD_XVM(synthesis_dependent, Enode);
   }
   if(synthesis_dependent && cycles)
   {
      double clock_period = GetPointer<functional_unit>(fu)->get_clock_period();
      if(clock_period == 0.0)
      {
         THROW_ERROR("Missing clock period for operation \"" + operation_name + "\" in unit " + fu->get_name());
      }
      double clock_period_resource_fraction = GetPointer<functional_unit>(fu)->get_clock_period_resource_fraction();
      execution_time = cycles * clock_period * clock_period_resource_fraction;
   }
   time_m->set_execution_time(execution_time, cycles);
   const ControlStep ii(initiation_time);
   time_m->set_initiation_time(ii);
   time_m->set_synthesis_dependent(synthesis_dependent);
   time_m->set_stage_period(stage_period);
   const xml_node::node_list list_int = Enode->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
      {
         continue;
      }
   }
}

void operation::xwrite(xml_element* rootnode, const technology_nodeRef, const ParameterConstRef)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());

   /// name of the operation
   WRITE_XVM(operation_name, Enode);
   /// commutative property
   if(commutative)
   {
      WRITE_XVM(commutative, Enode);
   }
   if(!bounded)
   {
      WRITE_XVM(bounded, Enode);
   }

   /// supported types
   auto it_end = supported_types.end();
   auto it_begin = supported_types.begin();
   std::string supported_types_string;
   for(auto it = it_begin; it != it_end; ++it)
   {
      if(it != it_begin)
      {
         supported_types_string += "|";
      }
      supported_types_string += it->first + ":";
      if(it->second.size() == 0)
      {
         supported_types_string += "*";
      }
      else
      {
         auto prec_end = it->second.end();
         auto prec_begin = it->second.begin();
         for(auto prec_it = prec_begin; prec_it != prec_end; ++prec_it)
         {
            if(prec_it != prec_begin)
            {
               supported_types_string += ",";
            }
            supported_types_string += STR(*prec_it);
         }
      }
   }
   if(supported_types_string != "")
   {
      WRITE_XNVM(supported_types, supported_types_string, Enode);
   }
   if(pipe_parameters != "")
   {
      WRITE_XVM(pipe_parameters, Enode);
   }
   if(portsize_parameters != "")
   {
      WRITE_XVM(portsize_parameters, Enode);
   }

   /// timing characterization, if any
   if(time_m)
   {
      if(time_m->get_cycles() != time_info::cycles_time_DEFAULT)
      {
         unsigned int cycles = time_m->get_cycles();
         WRITE_XVM(cycles, Enode);
      }
      else if(time_m->get_execution_time() != time_info::execution_time_DEFAULT)
      {
         double execution_time = time_m->get_execution_time();
         WRITE_XVM(execution_time, Enode);
      }

      if(time_m->get_initiation_time() != time_info::initiation_time_DEFAULT)
      {
         auto initiation_time = from_strongtype_cast<unsigned int>(time_m->get_initiation_time());
         WRITE_XVM(initiation_time, Enode);
      }
      if(time_m->get_stage_period() != time_info::stage_period_DEFAULT)
      {
         double stage_period = time_m->get_stage_period();
         WRITE_XVM(stage_period, Enode);
      }
      if(time_m->get_synthesis_dependent())
      {
         bool synthesis_dependent = time_m->get_synthesis_dependent();
         WRITE_XVM(synthesis_dependent, Enode);
      }
      if(primary_inputs_registered)
      {
         WRITE_XVM(primary_inputs_registered, Enode);
      }
   }
}

void operation::print(std::ostream& os) const
{
   os << " [OP: " << operation_name << " " << (time_m ? STR(time_m->get_execution_time()) : "(n.a.)") << " "
      << (time_m ? STR(time_m->get_initiation_time()) : "(n.a.)") << " ";
   os << (commutative ? " commutative" : " non-commutative");
   os << (bounded ? " bounded" : " unbounded") << "]";
}

bool operation::is_type_supported(const std::string& type_name) const
{
   /// if there is at least one supported type, the given type has to be in the list
   return supported_types.empty() || supported_types.count(type_name);
}

bool operation::is_type_supported(const std::string& type_name, unsigned long long type_prec) const
{
   if(!supported_types.empty())
   {
      if(!is_type_supported(type_name))
      {
         return false;
      }
      /// check also for the precision
      auto supported_type = supported_types.find(type_name);
      if(!supported_type->second.empty() && std::find(supported_type->second.begin(), supported_type->second.end(),
                                                      type_prec) == supported_type->second.end())
      {
         return false;
      }
   }
   return true;
}

bool operation::is_type_supported(const std::string& type_name, const std::vector<unsigned long long>& type_prec,
                                  const std::vector<unsigned long long>& /*type_n_element*/) const
{
   const auto max_prec = type_prec.empty() ? 0 : *max_element(type_prec.begin(), type_prec.end());
   return is_type_supported(type_name, max_prec);
}

std::string operation::get_type_supported_string() const
{
   std::string result;
   auto supported_type_it_end = supported_types.end();
   for(auto supported_type_it = supported_types.begin(); supported_type_it != supported_type_it_end;
       ++supported_type_it)
   {
      if(supported_type_it != supported_types.begin())
      {
         result += "|";
      }
      result += supported_type_it->first;
   }
   return result;
}

functional_unit::functional_unit()
    : logical_type(UNKNOWN), clock_period(0), clock_period_resource_fraction(1), characterization_timestamp()
{
}

functional_unit::functional_unit(const xml_nodeRef _XML_description)
    : logical_type(UNKNOWN),
      clock_period(0),
      clock_period_resource_fraction(1),
      XML_description(_XML_description),
      characterization_timestamp()
{
}

functional_unit::~functional_unit() = default;

void functional_unit::set_clock_period(double _clock_period)
{
   THROW_ASSERT(_clock_period > 0.0, "Clock period must be greater than zero");
   if(std::find(ordered_attributes.begin(), ordered_attributes.end(), "clock_period") == ordered_attributes.end())
   {
      ordered_attributes.push_back("clock_period");
   }
   std::vector<attributeRef> content;
   content.push_back(attributeRef(new attribute("float64", STR(_clock_period))));
   attributes["clock_period"] = attributeRef(new attribute(content));
   clock_period = _clock_period;
}

void functional_unit::set_clock_period_resource_fraction(double _clock_period_resource_fraction)
{
   if(std::find(ordered_attributes.begin(), ordered_attributes.end(), "clock_period_resource_fraction") ==
      ordered_attributes.end())
   {
      ordered_attributes.push_back("clock_period_resource_fraction");
   }
   std::vector<attributeRef> content;
   content.push_back(attributeRef(new attribute("float64", STR(_clock_period_resource_fraction))));
   attributes["clock_period_resource_fraction"] = attributeRef(new attribute(content));
   clock_period_resource_fraction = _clock_period_resource_fraction;
}

void functional_unit::print(std::ostream& os) const
{
   PP(os, "FU:\n[");
   PP(os, functional_unit_name + "\n");
   if(memory_type != "")
   {
      PP(os, "memory_type: " + memory_type + "\n");
   }
   if(list_of_operation.size() > 0)
   {
      std::copy(list_of_operation.begin(), list_of_operation.end(),
                std::ostream_iterator<const technology_nodeRef>(os, ""));
      PP(os, "\n");
   }
   if(area_m)
   {
      PP(os, "A: " + std::to_string(area_m->get_area_value()) + "\n");
   }
#if HAVE_CIRCUIT_BUILT
   if(CM)
   {
      CM->print(os);
   }
#endif
   PP(os, "]\n");
}

technology_nodeRef functional_unit::get_operation(const std::string& op_name) const
{
   auto i = op_name_to_op.find(op_name);
   if(i == op_name_to_op.end())
   {
      return technology_nodeRef();
   }
   return i->second;
}

void functional_unit::xload(const xml_element* Enode, const technology_nodeRef fu, const ParameterConstRef Param)
{
#ifndef NDEBUG
   auto debug_level = Param->get_class_debug_level(GET_CLASS(*this));
#endif
#if HAVE_CIRCUIT_BUILT
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
#endif

   logical_type = UNKNOWN;
   const xml_node::node_list list_int = Enode->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
      {
         continue;
      }
      if(EnodeC->get_name() == "name")
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("name is missing for " + EnodeC->get_name());
         }
         functional_unit_name = text->get_content();
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Started reading the module: " + functional_unit_name);
      }
      else if(EnodeC->get_name() == "template")
      {
         /*
          * Getting functional unit template name and parameters. It
          * will be responsibility of higher layer to retrieve a
          * reference to the template looking in the library.
          */
         fu_template_name = EnodeC->get_attribute("name")->get_value();
         fu_template_parameters = EnodeC->get_attribute("parameter")->get_value();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(characterizing_constant_value))
      {
         /*
          * Getting the constant value used to characterize the
          * specialization of a functional unit template.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("characterizing_constant_value is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("characterizing_constant_value is missing for " + EnodeC->get_name());
         }
         characterizing_constant_value = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_type))
      {
         /*
          * Getting the memory type the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("memory_type is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("memory_type is missing for " + EnodeC->get_name());
         }
         memory_type = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(channels_type))
      {
         /*
          * Getting the channels type the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("channels_type is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("channels_type is missing for " + EnodeC->get_name());
         }
         channels_type = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_ctrl_type))
      {
         /*
          * Getting the memory controller type the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("memory_ctrl_type is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("memory_ctrl_type is missing for " + EnodeC->get_name());
         }
         memory_ctrl_type = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(bram_load_latency))
      {
         /*
          * Getting the bram load latency the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("bram_load_latency is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("bram_load_latency is missing for " + EnodeC->get_name());
         }
         bram_load_latency = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(component_timing_alias))
      {
         /*
          * Getting the bram load latency the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("component_timing_alias is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("component_timing_alias is missing for " + EnodeC->get_name());
         }
         component_timing_alias = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(specialized))
      {
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(no_constant_characterization))
      {
      }
      else if(EnodeC->get_name() == "operation")
      {
         /// to be analyzed after all the other information
      }
      else if(EnodeC->get_name() == "characterization_timestamp")
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("Timestamp is missing for " + EnodeC->get_name());
         }
         if(text->get_content() == "")
         {
            THROW_ERROR("Timestamp is missing for " + EnodeC->get_name());
         }
         characterization_timestamp = TimeStamp(text->get_content());
      }
#if HAVE_CIRCUIT_BUILT
      else if(EnodeC->get_name() == "circuit")
      {
         if(!CM)
         {
            CM = structural_managerRef(new structural_manager(Param));
            structural_type_descriptorRef build_type =
                structural_type_descriptorRef(new structural_type_descriptor(functional_unit_name));
            CM->set_top_info(functional_unit_name, build_type);
         }
         // top must be a component_o
         const xml_node::node_list listC = EnodeC->get_children();
         for(const auto& iterC : listC)
         {
            const auto* EnodeCC = GetPointer<const xml_element>(iterC);
            if(!EnodeCC)
            {
               continue;
            }
            if(EnodeCC->get_name() == GET_CLASS_NAME(component_o))
            {
               CM->get_circ()->xload(EnodeCC, CM->get_circ(), CM);
            }
         }
      }
#endif
      else if(EnodeC->get_name() == "attribute")
      {
         attribute::xload(EnodeC, ordered_attributes, attributes);
         if(attributes.find("clock_period") != attributes.end())
         {
            clock_period = attributes["clock_period"]->get_content<double>();
         }
         if(attributes.find("clock_period_resource_fraction") != attributes.end())
         {
            clock_period_resource_fraction = attributes["clock_period_resource_fraction"]->get_content<double>();
         }
      }
      else
      {
         THROW_ERROR("functional_unit - not yet supported: " + EnodeC->get_name());
      }
   }

   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
      {
         continue;
      }
      if(EnodeC->get_name() == "operation")
      {
         technology_nodeRef op_curr = technology_nodeRef(new operation);
         op_curr->xload(EnodeC, fu, Param);
         add(op_curr);
      }
   }

   if(get_operations_num() == 0)
   {
      technology_nodeRef op_curr = technology_nodeRef(new operation);
      GetPointer<operation>(op_curr)->operation_name = functional_unit_name;
      add(op_curr);
   }

   area_m = area_info::factory(Param);
   if(attributes.find("area") != attributes.end())
   {
      area_m->set_area_value(attributes["area"]->get_content<double>());
   }
   if(attributes.find("REGISTERS") != attributes.end())
   {
      area_m->set_resource_value(area_info::REGISTERS, attributes["REGISTERS"]->get_content<double>());
   }
   if(attributes.find("SLICE_LUTS") != attributes.end())
   {
      area_m->set_resource_value(area_info::SLICE_LUTS, attributes["SLICE_LUTS"]->get_content<double>());
   }
   if(attributes.find("SLICE") != attributes.end())
   {
      area_m->set_resource_value(area_info::SLICE, attributes["SLICE"]->get_content<double>());
   }
   if(attributes.find("LUT_FF_PAIRS") != attributes.end())
   {
      area_m->set_resource_value(area_info::LUT_FF_PAIRS, attributes["LUT_FF_PAIRS"]->get_content<double>());
   }
   if(attributes.find("DSP") != attributes.end())
   {
      area_m->set_resource_value(area_info::DSP, attributes["DSP"]->get_content<double>());
   }
   if(attributes.find("BRAM") != attributes.end())
   {
      area_m->set_resource_value(area_info::BRAM, attributes["BRAM"]->get_content<double>());
   }
   if(attributes.find("DRAM") != attributes.end())
   {
      area_m->set_resource_value(area_info::DRAM, attributes["DRAM"]->get_content<double>());
   }
}

void functional_unit::xwrite(xml_element* rootnode, const technology_nodeRef tn, const ParameterConstRef Param)
{
   xml_element* xml_name = rootnode->add_child_element("name");
   /// functional unit name
   xml_name->add_child_text(functional_unit_name);

   /// add the operation related to the unit name if there is not anything else
   if(!list_of_operation.size())
   {
      technology_nodeRef curr_op = technology_nodeRef(new operation);
      GetPointer<operation>(curr_op)->operation_name = functional_unit_name;
      add(curr_op);
   }

   /// area attributes
   if(area_m)
   {
      double area_value = area_m->get_area_value();
      if(std::find(ordered_attributes.begin(), ordered_attributes.end(), "area") == ordered_attributes.end())
      {
         ordered_attributes.push_back("area");
      }
      attributes["area"] = attributeRef(new attribute(attribute::FLOAT64, std::to_string(area_value)));
      if(area_m)
      {
         if(area_m->get_resource_value(area_info::REGISTERS) != 0.0)
         {
            attributes["REGISTERS"] = attributeRef(
                new attribute(attribute::FLOAT64, std::to_string(area_m->get_resource_value(area_info::REGISTERS))));
         }
         if(area_m->get_resource_value(area_info::SLICE_LUTS) != 0.0)
         {
            attributes["SLICE_LUTS"] = attributeRef(
                new attribute(attribute::FLOAT64, std::to_string(area_m->get_resource_value(area_info::SLICE_LUTS))));
         }
         if(area_m->get_resource_value(area_info::LUT_FF_PAIRS) != 0.0)
         {
            attributes["LUT_FF_PAIRS"] = attributeRef(
                new attribute(attribute::FLOAT64, std::to_string(area_m->get_resource_value(area_info::LUT_FF_PAIRS))));
         }
         if(area_m->get_resource_value(area_info::DSP) != 0.0)
         {
            attributes["DSP"] = attributeRef(
                new attribute(attribute::FLOAT64, std::to_string(area_m->get_resource_value(area_info::DSP))));
         }
         if(area_m->get_resource_value(area_info::BRAM) != 0.0)
         {
            attributes["BRAM"] = attributeRef(
                new attribute(attribute::FLOAT64, std::to_string(area_m->get_resource_value(area_info::BRAM))));
         }
         if(area_m->get_resource_value(area_info::DRAM) != 0.0)
         {
            attributes["DRAM"] = attributeRef(
                new attribute(attribute::FLOAT64, std::to_string(area_m->get_resource_value(area_info::DRAM))));
         }
      }
   }

   /// dumping of attributes
   for(const auto& ordered_attribute : ordered_attributes)
   {
      const attributeRef attr = attributes[ordered_attribute];
      attr->xwrite(rootnode, ordered_attribute);
   }

   /// template stuff
   if(fu_template_name != "" && fu_template_parameters != "")
   {
      xml_element* template_el = rootnode->add_child_element("template");
      WRITE_XNVM2("name", fu_template_name, template_el);
      WRITE_XNVM2("parameter", fu_template_parameters, template_el);

      if(characterizing_constant_value != "")
      {
         xml_element* constant_el = rootnode->add_child_element(GET_CLASS_NAME(characterizing_constant_value));
         constant_el->add_child_text(characterizing_constant_value);
      }
   }

   if(memory_type != "")
   {
      xml_element* constant_el = rootnode->add_child_element(GET_CLASS_NAME(memory_type));
      constant_el->add_child_text(memory_type);
   }
   if(channels_type != "")
   {
      xml_element* constant_el = rootnode->add_child_element(GET_CLASS_NAME(channels_type));
      constant_el->add_child_text(channels_type);
   }
   if(memory_ctrl_type != "")
   {
      xml_element* constant_el = rootnode->add_child_element(GET_CLASS_NAME(memory_ctrl_type));
      constant_el->add_child_text(memory_ctrl_type);
   }
   if(bram_load_latency != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(bram_load_latency));
      xml_specialized->add_child_text(bram_load_latency);
   }
   if(component_timing_alias != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(component_timing_alias));
      xml_specialized->add_child_text(component_timing_alias);
   }
   auto xml_characterization_timestamp = rootnode->add_child_element("characterization_timestamp");
   xml_characterization_timestamp->add_child_text(STR(characterization_timestamp));
   /// operation stuff
   auto it_end = list_of_operation.end();
   for(auto it = list_of_operation.begin(); it != it_end; ++it)
   {
      GetPointer<operation>(*it)->xwrite(rootnode, tn, Param);
   }

   /// circuit stuff
#if HAVE_CIRCUIT_BUILT
   if(CM && CM->get_circ())
   {
      CM->xwrite(rootnode, tn);
   }
#endif
}

functional_unit_template::functional_unit_template() : FU(new functional_unit), no_constant_characterization(false)
{
}

functional_unit_template::functional_unit_template(const xml_nodeRef XML_description)
    : FU(new functional_unit(XML_description)), no_constant_characterization(false)
{
}

void functional_unit_template::xload(const xml_element* Enode, const technology_nodeRef tnd,
                                     const ParameterConstRef Param)
{
   const xml_node::node_list list_int = Enode->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
      {
         continue;
      }
      if(EnodeC->get_name() == GET_CLASS_NAME(specialized))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         }
         specialized = text->get_content();
         xml_node::convert_escaped(specialized);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(characterizing_constant_value))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         }
         characterizing_constant_value = text->get_content();
         xml_node::convert_escaped(characterizing_constant_value);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_type))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         }
         memory_type = text->get_content();
         xml_node::convert_escaped(memory_type);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(channels_type))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         }
         channels_type = text->get_content();
         xml_node::convert_escaped(channels_type);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_ctrl_type))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         }
         memory_ctrl_type = text->get_content();
         xml_node::convert_escaped(memory_ctrl_type);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(bram_load_latency))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
         {
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         }
         bram_load_latency = text->get_content();
         xml_node::convert_escaped(bram_load_latency);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(no_constant_characterization))
      {
         no_constant_characterization = true;
      }
   }
   FU->xload(Enode, tnd, Param);
}

void functional_unit_template::xwrite(xml_element* rootnode, const technology_nodeRef tnd,
                                      const ParameterConstRef Param)
{
   if(specialized != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element("specialized");
      xml_specialized->add_child_text(specialized);
   }
   if(characterizing_constant_value != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(characterizing_constant_value));
      xml_specialized->add_child_text(characterizing_constant_value);
   }
   if(memory_type != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(memory_type));
      xml_specialized->add_child_text(memory_type);
   }
   if(channels_type != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(channels_type));
      xml_specialized->add_child_text(channels_type);
   }
   if(memory_ctrl_type != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(memory_ctrl_type));
      xml_specialized->add_child_text(memory_ctrl_type);
   }
   if(bram_load_latency != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element(GET_CLASS_NAME(bram_load_latency));
      xml_specialized->add_child_text(bram_load_latency);
   }
   if(no_constant_characterization)
   {
      rootnode->add_child_element(GET_CLASS_NAME(no_constant_characterization));
   }
   FU->xwrite(rootnode, tnd, Param);
}

void functional_unit_template::print(std::ostream& os) const
{
   FU->print(os);
}
