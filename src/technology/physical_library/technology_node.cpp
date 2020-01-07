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
 * @file technology_node.cpp
 * @brief Class implementation of the technology node description.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_BOOLEAN_PARSER_BUILT.hpp"
#include "config_HAVE_CIRCUIT_BUILT.hpp"
#include "config_HAVE_CMOS_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"

#include "technology_node.hpp"

#include "area_model.hpp"
#include "target_device.hpp"
#include "time_model.hpp"

#include "clb_model.hpp"
#if HAVE_CMOS_BUILT
#include "liberty_model.hpp"
#endif
#include "library_manager.hpp"
#include "technology_manager.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#if HAVE_CMOS_BUILT
#if HAVE_EXPERIMENTAL
#include "TimingGenlib.hpp"
#include "TimingModel.hpp"
#endif
#include "timing_group.hpp"
#endif
#include "Parameter.hpp"
#include "exceptions.hpp"
#include "polixml.hpp"
#include "xml_helper.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

/// STL include
#include "custom_set.hpp"
#include <algorithm>
#include <list>
#include <utility>

/// utility include
#include "string_manipulation.hpp"

simple_indent technology_node::PP('[', ']', 3);

technology_node::technology_node() = default;

technology_node::~technology_node() = default;

operation::operation() : commutative(false), bounded(true), primary_inputs_registered(false)
{
}

operation::~operation() = default;

void operation::xload(const xml_element* Enode, const technology_nodeRef fu, const ParameterConstRef Param, const target_deviceRef device)
{
   THROW_ASSERT(CE_XVM(operation_name, Enode), "An operation must have a name");
   /// name of the operation
   LOAD_XVM(operation_name, Enode);
   /// commutative property
   if(CE_XVM(commutative, Enode))
      LOAD_XVM(commutative, Enode);

   if(CE_XVM(bounded, Enode))
   {
      LOAD_XVM(bounded, Enode);
   }

   if(CE_XVM(primary_inputs_registered, Enode))
      LOAD_XVM(primary_inputs_registered, Enode);

   /// parsing of supported types
   if(CE_XVM(supported_types, Enode))
   {
      std::string supported_types_string;
      LOAD_XVFM(supported_types_string, Enode, supported_types);
      std::vector<std::string> types = SplitString(supported_types_string, "|");
      for(std::vector<std::string>::const_iterator type = types.begin(); type != types.end(); ++type)
      {
         if(*type == "")
            THROW_ERROR("wrong XML syntax for supported_types attribute: null type description in \"" + supported_types_string + "\" [" + operation_name + "]");
         std::string type_name;
         std::vector<unsigned int> type_precs;
         std::vector<std::string> type_name_to_precs = SplitString(*type, ":");
         if(type_name_to_precs.size() != 2)
            THROW_ERROR("wrong XML syntax for supported_types attribute around \":\" \"" + *type + "\" [" + operation_name + "]");
         type_name = type_name_to_precs[0];
         if(type_name == "")
            THROW_ERROR("wrong XML syntax for supported_types attribute: missing the supported type - \"" + *type + "\" [" + operation_name + "]");

         if(type_name_to_precs[1] != "*")
         {
            std::vector<std::string> precs = SplitString(type_name_to_precs[1], ",");
            ;
            for(std::vector<std::string>::const_iterator single_prec = precs.begin(); single_prec != precs.end() && *single_prec != ""; ++single_prec)
            {
               auto type_uint = boost::lexical_cast<unsigned int>(*single_prec);
               type_precs.push_back(type_uint);
            }
         }
         supported_types.insert(make_pair(type_name, type_precs));
      }
   }
   if(CE_XVM(pipe_parameters, Enode))
      LOAD_XVM(pipe_parameters, Enode);
   if(CE_XVM(portsize_parameters, Enode))
      LOAD_XVM(portsize_parameters, Enode);

   /// time characterization
   time_m = time_model::create_model(device->get_type(), Param);
   double execution_time = time_model::execution_time_DEFAULT;
   auto initiation_time = from_strongtype_cast<unsigned int>(time_model::initiation_time_DEFAULT);
   unsigned int cycles = time_model::cycles_time_DEFAULT;
   double stage_period = time_model::stage_period_DEFAULT;
   if(CE_XVM(execution_time, Enode))
      LOAD_XVM(execution_time, Enode);
   if(CE_XVM(initiation_time, Enode))
      LOAD_XVM(initiation_time, Enode);
   if(CE_XVM(cycles, Enode))
      LOAD_XVM(cycles, Enode);
   if(CE_XVM(stage_period, Enode))
      LOAD_XVM(stage_period, Enode);
   bool synthesis_dependent = false;
   if(CE_XVM(synthesis_dependent, Enode))
      LOAD_XVM(synthesis_dependent, Enode);
   if(synthesis_dependent && cycles)
   {
      double clock_period = GetPointer<functional_unit>(fu)->get_clock_period();
      if(clock_period == 0.0)
         THROW_ERROR("Missing clock period for operation \"" + operation_name + "\" in unit " + fu->get_name());
      double clock_period_resource_fraction = GetPointer<functional_unit>(fu)->get_clock_period_resource_fraction();
      execution_time = cycles * clock_period * clock_period_resource_fraction;
   }
   time_m->set_execution_time(execution_time, cycles);
   const ControlStep ii(initiation_time);
   time_m->set_initiation_time(ii);
   time_m->set_synthesis_dependent(synthesis_dependent);
   time_m->set_stage_period(stage_period);
   /// timing path
   double max_delay = 0.0;
   const xml_node::node_list list_int = Enode->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == "timing_path")
      {
         std::string source, target;
         double delay;
         LOAD_XVM(source, EnodeC);
         LOAD_XVM(target, EnodeC);
         LOAD_XVM(delay, EnodeC);
         max_delay = std::max(delay, max_delay);
         time_m->set_execution_time(max_delay, time_model::cycles_time_DEFAULT);
         time_m->pin_to_pin_delay[source][target] = delay;
      }
   }
}

void operation::xwrite(xml_element* rootnode, const technology_nodeRef, const ParameterConstRef, const TargetDevice_Type /*type*/)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());

   /// name of the operation
   WRITE_XVM(operation_name, Enode);
   /// commutative property
   if(commutative)
      WRITE_XVM(commutative, Enode);
   if(!bounded)
      WRITE_XVM(bounded, Enode);

   /// supported types
   std::map<std::string, std::vector<unsigned int>>::const_iterator it_end = supported_types.end();
   std::map<std::string, std::vector<unsigned int>>::const_iterator it_begin = supported_types.begin();
   std::string supported_types_string;
   for(auto it = it_begin; it != it_end; ++it)
   {
      if(it != it_begin)
         supported_types_string += "|";
      supported_types_string += it->first + ":";
      if(it->second.size() == 0)
         supported_types_string += "*";
      else
      {
         auto prec_end = it->second.end();
         auto prec_begin = it->second.begin();
         for(auto prec_it = prec_begin; prec_it != prec_end; ++prec_it)
         {
            if(prec_it != prec_begin)
               supported_types_string += ",";
            supported_types_string += STR(*prec_it);
         }
      }
   }
   if(supported_types_string != "")
      WRITE_XNVM(supported_types, supported_types_string, Enode);
   if(pipe_parameters != "")
      WRITE_XVM(pipe_parameters, Enode);
   if(portsize_parameters != "")
      WRITE_XVM(portsize_parameters, Enode);

   /// timing characterization, if any
   if(time_m)
   {
      if(time_m->get_cycles() != time_model::cycles_time_DEFAULT)
      {
         unsigned int cycles = time_m->get_cycles();
         WRITE_XVM(cycles, Enode);
      }
      else if(time_m->get_execution_time() != time_model::execution_time_DEFAULT)
      {
         double execution_time = time_m->get_execution_time();
         WRITE_XVM(execution_time, Enode);
      }

      if(time_m->get_initiation_time() != time_model::initiation_time_DEFAULT)
      {
         auto initiation_time = from_strongtype_cast<unsigned int>(time_m->get_initiation_time());
         WRITE_XVM(initiation_time, Enode);
      }
      if(time_m->get_stage_period() != time_model::stage_period_DEFAULT)
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

void operation::lib_write(std::ofstream&, const simple_indentRef)
{
   THROW_UNREACHABLE("This function has not to be called");
}

void operation::lef_write(std::ofstream&, const simple_indentRef)
{
   THROW_UNREACHABLE("This function has not to be called");
}

void operation::print(std::ostream& os) const
{
   os << " [OP: " << operation_name << " " << (time_m ? STR(time_m->get_execution_time()) : "(n.a.)") << " " << (time_m ? STR(time_m->get_initiation_time()) : "(n.a.)") << " ";
   os << (commutative ? " commutative" : " non-commutative");
   os << (bounded ? " bounded" : " unbounded") << "]";
}

bool operation::is_type_supported(std::string type_name) const
{
   if(supported_types.begin() != supported_types.end())
   {
      /// if there is at least one supported type, the given type has to be in the list
      if(supported_types.find(type_name) == supported_types.end())
         return false;
   }
   return true;
}

bool operation::is_type_supported(std::string type_name, unsigned int type_prec) const
{
   if(supported_types.begin() != supported_types.end())
   {
      if(!is_type_supported(type_name))
         return false;
      /// check also for the precision
      auto supported_type = supported_types.find(type_name);
      if(supported_type->second.size() > 0 && std::find(supported_type->second.begin(), supported_type->second.end(), type_prec) == supported_type->second.end())
         return false;
   }
   return true;
}

bool operation::is_type_supported(const std::string& type_name, const std::vector<unsigned int>& type_prec, const std::vector<unsigned int>& /*type_n_element*/) const
{
   unsigned int max_prec = type_prec.begin() == type_prec.end() ? 0 : *max_element(type_prec.begin(), type_prec.end());

   return is_type_supported(type_name, max_prec);
}

std::string operation::get_type_supported_string() const
{
   std::string result;
   auto supported_type_it_end = supported_types.end();
   for(auto supported_type_it = supported_types.begin(); supported_type_it != supported_type_it_end; ++supported_type_it)
   {
      if(supported_type_it != supported_types.begin())
         result += "|";
      result += supported_type_it->first;
   }
   return result;
}

std::map<std::string, std::map<std::string, double>> operation::get_pin_to_pin_delay() const
{
   THROW_ASSERT(time_m, "timing information not yet specified for operation " + operation_name);
   return time_m->pin_to_pin_delay;
}

functional_unit::functional_unit() : logical_type(UNKNOWN), clock_period(0), clock_period_resource_fraction(1), characterization_timestamp()
{
}

functional_unit::functional_unit(const xml_nodeRef _XML_description) : logical_type(UNKNOWN), clock_period(0), clock_period_resource_fraction(1), XML_description(_XML_description), characterization_timestamp()
{
}

functional_unit::~functional_unit() = default;

void functional_unit::set_clock_period(double _clock_period)
{
   THROW_ASSERT(_clock_period > 0.0, "Clock period must be greater than zero");
   if(std::find(ordered_attributes.begin(), ordered_attributes.end(), "clock_period") == ordered_attributes.end())
      ordered_attributes.push_back("clock_period");
   std::vector<attributeRef> content;
   content.push_back(attributeRef(new attribute("float64", STR(_clock_period))));
   attributes["clock_period"] = attributeRef(new attribute(content));
   clock_period = _clock_period;
}

void functional_unit::set_clock_period_resource_fraction(double _clock_period_resource_fraction)
{
   if(std::find(ordered_attributes.begin(), ordered_attributes.end(), "clock_period_resource_fraction") == ordered_attributes.end())
      ordered_attributes.push_back("clock_period_resource_fraction");
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
      PP(os, "memory_type: " + memory_type + "\n");
   if(list_of_operation.size() > 0)
   {
      std::copy(list_of_operation.begin(), list_of_operation.end(), std::ostream_iterator<const technology_nodeRef>(os, ""));
      PP(os, "\n");
   }
   if(area_m)
   {
      PP(os, "A: " + boost::lexical_cast<std::string>(area_m->get_area_value()) + "\n");
   }
#if HAVE_CIRCUIT_BUILT
   if(CM)
      CM->print(os);
#endif
   PP(os, "]\n");
}

technology_nodeRef functional_unit::get_operation(const std::string& op_name) const
{
   auto i = op_name_to_op.find(op_name);
   if(i == op_name_to_op.end())
      return technology_nodeRef();
   return i->second;
}

#if HAVE_BOOLEAN_PARSER_BUILT
void functional_unit::gload(const std::string& definition, const std::string& fu_name,
                            technology_nodeRef
#if HAVE_CIRCUIT_BUILT
                                owner
#endif
                            ,
                            const ParameterConstRef
#if HAVE_CIRCUIT_BUILT
                                Param
#endif
)
{
   std::list<std::string> splitted = SplitString(definition, " ;\t");

#if HAVE_CIRCUIT_BUILT
   CustomOrderedSet<std::string> inputs;
   std::string output;
#endif
   // std::map<std::string, std::map<std::string, TimingModelRef> > pin_models;

   unsigned int n = 0;
   for(std::list<std::string>::iterator s = splitted.begin(); s != splitted.end(); ++s)
   {
      if(s->size() == 0)
         continue;
      switch(n)
      {
         case 0:
         {
            if(*s == "GATE")
            {
               logical_type = COMBINATIONAL;
               n++;
            }
            if(*s == "PIN")
            {
               n = 5;
            }
            break;
         }
         case 1:
         {
            functional_unit_name = fu_name;
            n++;
            break;
         }
         case 2:
         {
#if 0
            double area = boost::lexical_cast<double>(*s);
            double height = Param->getOption<double>("cell-height");
            double width = area / height;
#endif
            n++;
            break;
         }
         case 3:
         {
#if HAVE_CIRCUIT_BUILT
            int debug_level = Param->getOption<int>(OPT_debug_level);
            CM = structural_managerRef(new structural_manager(Param));
            structural_type_descriptorRef build_type = structural_type_descriptorRef(new structural_type_descriptor(functional_unit_name));
            CM->set_top_info(functional_unit_name + "IDLIB", build_type);

            structural_objectRef obj = CM->get_circ();
            module* mobj = GetPointer<module>(obj);
            structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));

            /// FIXME: the implementation of the following function does not exist
            //            boolean_parseY(*s, inputs, output, 0);

            // std::cerr << "Output= " << output << std::endl;
            structural_objectRef new_port = structural_objectRef(new port_o(debug_level, obj, port_o::OUT));
            new_port->set_id(output);
            new_port->set_type(bool_type);
            mobj->add_out_port(new_port);
            for(CustomOrderedSet<std::string>::iterator i = inputs.begin(); i != inputs.end(); ++i)
            {
               if(i->find("CONST") != std::string::npos)
                  continue;

               // std::cerr << "  Input= " << *n << std::endl;
               structural_objectRef p = structural_objectRef(new port_o(debug_level, obj, port_o::IN));
               p->set_id(*i);
               p->set_type(bool_type);
               mobj->add_in_port(p);
            }

            std::string eq = *s + ";";
            if(eq.find("CONST") != std::string::npos)
            {
               eq = output + "=" + eq.substr(eq.find("CONST") + 5, eq.size());
            }
            // std::cerr << "eq= " << eq << std::endl;

            CM->add_NP_functionality(CM->get_circ(), NP_functionality::EQUATION, eq);
#endif
            n++;
            break;
         }
         case 4: // PIN token
         {
            n++;
            break;
         }
         case 5: // pin name
         {
#if 0
            //analyzing the pin name or the wildcard *
            if (*s == "*")
            {
               for(CustomOrderedSet<std::string>::iterator i = inputs.begin(); i != inputs.end(); i++)
               {
                  pin_models[*i][output] = TimingModel::create_model(TimingModel::GENLIB_LIBRARY, Param);
               }
            }
            else
            {
               pin_models[*s][output] = TimingModel::create_model(TimingModel::GENLIB_LIBRARY, Param);
            }
#endif
            n++;
            break;
         }
         case 6: // phase
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  if (*s == "INV")
                     GetPointer<TimingGenlib>(o->second)->phase = TimingGenlib::INV;
                  else if (*s == "NONINV")
                     GetPointer<TimingGenlib>(o->second)->phase = TimingGenlib::NONINV;
                  else if (*s == "UNKNOWN")
                     GetPointer<TimingGenlib>(o->second)->phase = TimingGenlib::UNKNOWN;
                  else
                     THROW_ERROR("Not supported phase: " + *s);
               }
            }
#endif
            n++;
            break;
         }
         case 7: // input load
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  GetPointer<TimingGenlib>(o->second)->input_load = boost::lexical_cast<double>(*s);
               }
            }
#endif
            n++;
            break;
         }
         case 8: // maximum load
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  GetPointer<TimingGenlib>(o->second)->max_load = boost::lexical_cast<double>(*s);
               }
            }
#endif
            n++;
            break;
         }
         case 9: // rise-block
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  GetPointer<TimingGenlib>(o->second)->rise_block_delay = boost::lexical_cast<double>(*s);
               }
            }
#endif
            n++;
            break;
         }
         case 10: // rise-fanout
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  GetPointer<TimingGenlib>(o->second)->rise_fanout_delay = boost::lexical_cast<double>(*s);
               }
            }
#endif
            n++;
            break;
         }
         case 11: // fall-block
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  GetPointer<TimingGenlib>(o->second)->fall_block_delay = boost::lexical_cast<double>(*s);
               }
            }
#endif
            n++;
            break;
         }
         case 12: // fall-fanout
         {
#if 0
            for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
            {
               for(std::map<std::string, TimingModelRef>::iterator o = i->second.begin(); o != i->second.end(); o++)
               {
                  GetPointer<TimingGenlib>(o->second)->fall_fanout_delay = boost::lexical_cast<double>(*s);
               }
            }
#endif
            n = 0;
            break;
         }
         default:
         {
            THROW_ERROR("Malformed library: " + *s);
         }
      }
   }

#if 0
   for(std::map<std::string, std::map<std::string, TimingModelRef> >::iterator i = pin_models.begin(); i != pin_models.end(); i++)
   {
      for(std::map<std::string, TimingModelRef>::iterator k = i->second.begin(); k != i->second.end(); k++)
      {
         pin_timing_models[i->first][k->first] = k->second;
      }
   }
#endif

   technology_nodeRef op_curr = technology_nodeRef(new operation());
   GetPointer<operation>(op_curr)->operation_name = functional_unit_name;
   add(op_curr);
}
#endif

void functional_unit::xload(const xml_element* Enode, const technology_nodeRef fu, const ParameterConstRef Param, const target_deviceRef device)
{
   TargetDevice_Type dv_type = device->get_type();
#ifndef NDEBUG
   auto debug_level = Param->get_class_debug_level(GET_CLASS(*this));
#endif
#if HAVE_TECHNOLOGY_BUILT && HAVE_CMOS_BUILT
   int output_pin_counter = 0;
#endif
#if HAVE_CIRCUIT_BUILT
   structural_type_descriptorRef bool_type = structural_type_descriptorRef(new structural_type_descriptor("bool", 0));
#endif

#if HAVE_TECHNOLOGY_BUILT && HAVE_CMOS_BUILT
   std::map<std::string, std::vector<std::string>> attribute_list;
   std::map<std::string, std::map<std::string, attributeRef>> attribute_map;
   std::map<unsigned int, std::string> NP_functionalities;
#endif

   logical_type = UNKNOWN;
   const xml_node::node_list list_int = Enode->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == "name")
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("name is missing for " + EnodeC->get_name());
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
            THROW_ERROR("characterizing_constant_value is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("characterizing_constant_value is missing for " + EnodeC->get_name());
         characterizing_constant_value = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_type))
      {
         /*
          * Getting the memory type the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("memory_type is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("memory_type is missing for " + EnodeC->get_name());
         memory_type = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(channels_type))
      {
         /*
          * Getting the channels type the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("channels_type is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("channels_type is missing for " + EnodeC->get_name());
         channels_type = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_ctrl_type))
      {
         /*
          * Getting the memory controller type the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("memory_ctrl_type is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("memory_ctrl_type is missing for " + EnodeC->get_name());
         memory_ctrl_type = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(bram_load_latency))
      {
         /*
          * Getting the bram load latency the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("bram_load_latency is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("bram_load_latency is missing for " + EnodeC->get_name());
         bram_load_latency = text->get_content();
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(component_timing_alias))
      {
         /*
          * Getting the bram load latency the functional unit is compliant with.
          */
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("component_timing_alias is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("component_timing_alias is missing for " + EnodeC->get_name());
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
            THROW_ERROR("Timestamp is missing for " + EnodeC->get_name());
         if(text->get_content() == "")
            THROW_ERROR("Timestamp is missing for " + EnodeC->get_name());
         characterization_timestamp = TimeStamp(text->get_content());
      }
#if HAVE_CIRCUIT_BUILT
      else if(EnodeC->get_name() == "circuit")
      {
         if(!CM)
         {
            CM = structural_managerRef(new structural_manager(Param));
            structural_type_descriptorRef build_type = structural_type_descriptorRef(new structural_type_descriptor(functional_unit_name));
            CM->set_top_info(functional_unit_name, build_type);
         }
         // top must be a component_o
         const xml_node::node_list listC = EnodeC->get_children();
         for(const auto& iterC : listC)
         {
            const auto* EnodeCC = GetPointer<const xml_element>(iterC);
            if(!EnodeCC)
               continue;
            if(EnodeCC->get_name() == GET_CLASS_NAME(component_o))
               CM->get_circ()->xload(EnodeCC, CM->get_circ(), CM);
         }
      }
#endif
      else if(EnodeC->get_name() == "attribute")
      {
         attribute::xload(EnodeC, ordered_attributes, attributes);
#if HAVE_CMOS_BUILT
         /// check the attributes to determine if the cell is physical and, thus, it has not to be used for the logic synthesis
         if(dv_type == TargetDevice_Type::IC && attributes.find("dont_use") != attributes.end())
         {
            auto value = attributes["dont_use"]->get_content<bool>();
            if(value)
               logical_type = PHYSICAL;
         }
#endif
         if(attributes.find("clock_period") != attributes.end())
         {
            clock_period = attributes["clock_period"]->get_content<double>();
         }
         if(attributes.find("clock_period_resource_fraction") != attributes.end())
         {
            clock_period_resource_fraction = attributes["clock_period_resource_fraction"]->get_content<double>();
         }
      }
#if HAVE_TECHNOLOGY_BUILT && HAVE_CMOS_BUILT
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "pin")
      {
         std::string pin_name;

         const xml_node::node_list& pin_list = EnodeC->get_children();
         for(const auto& iter_int1 : pin_list)
         {
            const auto* EnodeP = GetPointer<const xml_element>(iter_int1);
            if(!EnodeP)
               continue;
            if(EnodeP->get_name() == "name")
            {
               const xml_text_node* text = EnodeP->get_child_text();
               pin_name = text->get_content();
            }
            else if(EnodeP->get_name() == "attribute")
            {
               THROW_ASSERT(pin_name.size(), "Pin name has to be specified before the attributes");
               attribute::xload(EnodeP, attribute_list[pin_name], attribute_map[pin_name]);
            }
         }

         if(attribute_map[pin_name].find("function") != attribute_map[pin_name].end())
         {
            const std::string this_function_attribute = functional_unit_name + "_" + boost::lexical_cast<std::string>(output_pin_counter);
            output_pin_counter++;
            ordered_attributes.push_back(this_function_attribute);
            attributes[this_function_attribute] = attribute_map[pin_name]["function"];
         }

         THROW_ASSERT(attribute_map[pin_name].find("direction") != attribute_map[pin_name].end(), "Direction not stored for pin: " + functional_unit_name + "/" + pin_name);

         std::string direction = attribute_map[pin_name]["direction"]->get_content_str();
         port_o::port_direction pdir = port_o::GEN;
         if(direction == "input")
            pdir = port_o::IN;
         else if(direction == "output")
            pdir = port_o::OUT;
         else if(direction != "internal")
            THROW_ERROR("not supported port type: " + direction);
#if HAVE_CIRCUIT_BUILT
         if(!CM)
         {
            CM = structural_managerRef(new structural_manager(Param));
            structural_type_descriptorRef build_type = structural_type_descriptorRef(new structural_type_descriptor(functional_unit_name));
            CM->set_top_info(functional_unit_name, build_type);
         }
         structural_objectRef port = CM->add_port(pin_name, pdir, CM->get_circ(), bool_type);
         for(unsigned int v = 0; v < attribute_list[pin_name].size(); v++)
         {
            port->add_attribute(attribute_list[pin_name][v], attribute_map[pin_name][attribute_list[pin_name][v]]);
         }
#endif

         if(pdir == port_o::OUT and attribute_map[pin_name].find("function") != attribute_map[pin_name].end())
         {
            std::string equation;
            if(NP_functionalities.find(NP_functionality::EQUATION) != NP_functionalities.end())
            {
               equation = NP_functionalities[NP_functionality::EQUATION];
            }

            std::string function = attribute_map[pin_name]["function"]->get_content_str();
            xml_node::convert_escaped(function);
            equation = equation + pin_name + "=" + function + ";";
            NP_functionalities[NP_functionality::EQUATION] = equation;
            if(dv_type == TargetDevice_Type::FPGA and NP_functionalities.find(NP_functionality::VERILOG_PROVIDED) == NP_functionalities.end())
            {
               std::string verilog = equation;
               boost::replace_all(verilog, "=", " = ");
               boost::replace_all(verilog, "+", "|");
               boost::replace_all(verilog, "*", "&");
               NP_functionalities[NP_functionality::VERILOG_PROVIDED] = "assign " + verilog;
            }
         }
      }
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "leakage_power")
      {
      }
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "pg_pin")
      {
      }
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "ff")
      {
         logical_type = FF;
      }
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "latch")
      {
         logical_type = LATCH;
      }
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "statetable")
      {
         logical_type = STATETABLE;
      }
      else if(dv_type == TargetDevice_Type::IC && EnodeC->get_name() == "layout")
         logical_type = COMBINATIONAL; // do nothing
#endif
      else
         THROW_ERROR("functional_unit - not yet supported: " + EnodeC->get_name());
   }

   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == "operation")
      {
         technology_nodeRef op_curr = technology_nodeRef(new operation);
         op_curr->xload(EnodeC, fu, Param, device);
         add(op_curr);
      }
   }

#if HAVE_CIRCUIT_BUILT
   for(auto& NP_functionalitie : NP_functionalities)
   {
      CM->add_NP_functionality(CM->get_circ(), static_cast<NP_functionality::NP_functionaly_type>(NP_functionalitie.first), NP_functionalitie.second);
   }
#endif

   if(get_operations_num() == 0)
   {
      technology_nodeRef op_curr = technology_nodeRef(new operation);
      GetPointer<operation>(op_curr)->operation_name = functional_unit_name;
      add(op_curr);
   }

   area_m = area_model::create_model(dv_type, Param);
#if HAVE_CMOS_BUILT
   /// CMOS technology
   if(dv_type == TargetDevice_Type::IC)
   {
      /// area stuff
      if(attributes.find("area") != attributes.end())
         area_m->set_area_value(attributes["area"]->get_content<double>());
      else
      {
         area_m->set_area_value(0.0);
      }
      /// time stuff
      if(!list_of_operation.size())
      {
         technology_nodeRef curr_op = technology_nodeRef(new operation);
         GetPointer<operation>(curr_op)->operation_name = functional_unit_name;
         add(curr_op);
      }
      for(auto& v : list_of_operation)
      {
         if(!GetPointer<operation>(v)->time_m)
         {
            GetPointer<operation>(v)->time_m = time_model::create_model(dv_type, Param);
         }
         if(attributes.find("drive_strength") != attributes.end())
            GetPointer<liberty_model>(GetPointer<operation>(v)->time_m)->set_drive_strength(attributes["drive_strength"]->get_content<double>());
#if HAVE_CIRCUIT_BUILT
         if(!GetPointer<liberty_model>(GetPointer<operation>(v)->time_m)->has_timing_groups() && CM && CM->get_circ())
         {
            for(unsigned int l = 0; l < GetPointer<module>(CM->get_circ())->get_out_port_size(); l++)
            {
               std::string output_name = GetPointer<module>(CM->get_circ())->get_out_port(l)->get_id();
               CustomOrderedSet<std::string> inputs;
               for(unsigned int m = 0; m < GetPointer<module>(CM->get_circ())->get_in_port_size(); m++)
               {
                  inputs.insert(GetPointer<module>(CM->get_circ())->get_in_port(m)->get_id());
               }
               GetPointer<liberty_model>(GetPointer<operation>(v)->time_m)->add_timing_group(output_name, inputs, timing_groupRef(new timing_group));
            }
         }
#endif
      }
   }
   else
#endif
   {
      /// FPGA technology
      if(dv_type == TargetDevice_Type::FPGA)
      {
         /// area stuff
         if(attributes.find("area") != attributes.end())
            area_m->set_area_value(attributes["area"]->get_content<double>());
         auto* clb = GetPointer<clb_model>(area_m);
         if(attributes.find("REGISTERS") != attributes.end())
            clb->set_resource_value(clb_model::REGISTERS, attributes["REGISTERS"]->get_content<double>());
         if(attributes.find("SLICE_LUTS") != attributes.end())
            clb->set_resource_value(clb_model::SLICE_LUTS, attributes["SLICE_LUTS"]->get_content<double>());
         if(attributes.find("SLICE") != attributes.end())
            clb->set_resource_value(clb_model::SLICE, attributes["SLICE"]->get_content<double>());
         if(attributes.find("LUT_FF_PAIRS") != attributes.end())
            clb->set_resource_value(clb_model::LUT_FF_PAIRS, attributes["LUT_FF_PAIRS"]->get_content<double>());
         if(attributes.find("DSP") != attributes.end())
            clb->set_resource_value(clb_model::DSP, attributes["DSP"]->get_content<double>());
         if(attributes.find("BRAM") != attributes.end())
            clb->set_resource_value(clb_model::BRAM, attributes["BRAM"]->get_content<double>());
      }
   }
#if 0

   std::string type = "UNKNOWN";
   if(CE_XVM(type, Enode)) LOAD_XVM(type, Enode);
   if (type == "COMBINATIONAL")
      logical_type = COMBINATIONAL;
   else if (type == "FF")
      logical_type = SEQUENTIAL;
   else if (type == "LATCH")
      logical_type = UNKNOWN;
   else
      THROW_ERROR("Logic type of cell " + functional_unit_name + " is not supported: " + type);

   //Recurse through child nodes:
   const xml_node::node_list list = Enode->get_children();
   for (xml_node::node_list::const_iterator iter = list.begin(); iter != list.end(); ++iter)
   {
      const xml_element* EnodeC = GetPointer<const xml_element>(*iter);
      if(!EnodeC) continue;
      if(EnodeC->get_name() == GET_CLASS_NAME(operation))
      {
         technology_nodeRef op_curr = technology_nodeRef(new operation);
         op_curr->xload(EnodeC, op_curr, Param);
         add(op_curr);
      }
      else if(EnodeC->get_name() == "circuit")
      {
         CM = structural_managerRef(new structural_manager);
         structural_type_descriptorRef build_type = structural_type_descriptorRef(new structural_type_descriptor("BUILD"));
         CM->set_top_info("BUILD", build_type);
         //top must be a component_o
         const xml_node::node_list listC = EnodeC->get_children();
         for (xml_node::node_list::const_iterator iterC = listC.begin(); iterC != listC.end(); ++iterC)
         {
            const xml_element* EnodeCC = GetPointer<const xml_element>(*iterC);
            if(!EnodeCC) continue;
            if(EnodeCC->get_name() == GET_CLASS_NAME(component_o))
               CM->get_circ()->xload(EnodeCC, CM->get_circ(), CM);
         }
      }
#if HAVE_TECHNOLOGY_MAPPING
      else if(EnodeC->get_name() == "timing_model") //TODO: move into TimingModel::xload
      {
         std::string type;
         LOAD_XVM(type, EnodeC);

         THROW_ASSERT(type == "genlib_library", "Only genlib timing model is currently supported");

         const xml_node::node_list listC = EnodeC->get_children();
         for (xml_node::node_list::const_iterator iterC = listC.begin(); iterC != listC.end(); ++iterC)
         {
            const xml_element* EnodeCC = GetPointer<const xml_element>(*iterC);
            if(!EnodeCC) continue;
            if(EnodeCC->get_name() == "pin")
            {
               std::string input;
               LOAD_XVM(input, EnodeCC);

               std::string output;
               LOAD_XVM(output, EnodeCC);

               std::string phase;
               LOAD_XVM(phase, EnodeCC);

               double input_load;
               LOAD_XVM(input_load, EnodeCC);

               double max_load;
               LOAD_XVM(max_load, EnodeCC);

               double rise_block_delay;
               LOAD_XVM(rise_block_delay, EnodeCC);

               double rise_fanout_delay;
               LOAD_XVM(rise_fanout_delay, EnodeCC);

               double fall_block_delay;
               LOAD_XVM(fall_block_delay, EnodeCC);

               double fall_fanout_delay;
               LOAD_XVM(fall_fanout_delay, EnodeCC);

               TimingModelRef model = TimingModel::create_model(TimingModel::GENLIB_LIBRARY, Param);
               TimingGenlib* tm = GetPointer<TimingGenlib>(model);

               if (phase == "INV")
                  tm->phase = TimingGenlib::INV;
               else if (phase == "NONINV")
                  tm->phase = TimingGenlib::NONINV;
               else if (phase == "UNKNOWN")
                  tm->phase = TimingGenlib::UNKNOWN;
               else
                  THROW_ERROR("malformed");

               tm->input_load = input_load;
               tm->max_load = max_load;

               tm->rise_block_delay = rise_block_delay;
               tm->rise_fanout_delay = rise_fanout_delay;

               tm->fall_block_delay = fall_block_delay;
               tm->fall_fanout_delay = fall_fanout_delay;

               pin_timing_models[input][output] = model;
            }
         }
      }
#endif
   }
#endif
}

void functional_unit::xwrite(xml_element* rootnode, const technology_nodeRef tn, const ParameterConstRef Param, const TargetDevice_Type dv_type)
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
         ordered_attributes.push_back("area");
      attributes["area"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(area_value)));
      /// FPGA
      auto* clb = GetPointer<clb_model>(area_m);
      if(clb)
      {
         if(clb->get_resource_value(clb_model::REGISTERS) != 0.0)
            attributes["REGISTERS"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(clb->get_resource_value(clb_model::REGISTERS))));
         if(clb->get_resource_value(clb_model::SLICE_LUTS) != 0.0)
            attributes["SLICE_LUTS"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(clb->get_resource_value(clb_model::SLICE_LUTS))));
         if(clb->get_resource_value(clb_model::LUT_FF_PAIRS) != 0.0)
            attributes["LUT_FF_PAIRS"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(clb->get_resource_value(clb_model::LUT_FF_PAIRS))));
         if(clb->get_resource_value(clb_model::DSP) != 0.0)
            attributes["DSP"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(clb->get_resource_value(clb_model::DSP))));
         if(clb->get_resource_value(clb_model::BRAM) != 0.0)
            attributes["BRAM"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(clb->get_resource_value(clb_model::BRAM))));
      }
   }

#if HAVE_CMOS_BUILT
   /// CMOS time attributes
   time_modelRef time_m = GetPointer<operation>(list_of_operation.front())->time_m;
   if(time_m && GetPointer<liberty_model>(time_m))
   {
      if(std::find(ordered_attributes.begin(), ordered_attributes.end(), "drive_strength") == ordered_attributes.end())
         ordered_attributes.push_back("drive_strength");
      attributes["drive_strength"] = attributeRef(new attribute(attribute::FLOAT64, boost::lexical_cast<std::string>(GetPointer<liberty_model>(time_m)->get_drive_strength())));
   }
#endif

   /// dumping of attributes
   for(const auto& ordered_attribute : ordered_attributes)
   {
      const attributeRef attr = attributes[ordered_attribute];
      attr->xwrite(rootnode, ordered_attribute);
   }

   /// writing logical type
   if(area_m)
   {
#if HAVE_CMOS_BUILT
      if(dv_type == TargetDevice_Type::IC)
      {
         if(logical_type == STATETABLE)
            rootnode->add_child_element("statetable");
         else if(logical_type == FF)
            rootnode->add_child_element("ff");
         else if(logical_type == LATCH)
            rootnode->add_child_element("latch");
      }
#endif
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
      GetPointer<operation>(*it)->xwrite(rootnode, tn, Param, dv_type);
   }

   /// circuit stuff
#if HAVE_CIRCUIT_BUILT
   if(CM && CM->get_circ())
   {
      CM->xwrite(rootnode, tn);
   }
#endif
}

void functional_unit::lib_write(std::ofstream& os, const simple_indentRef _PP)
{
   (*_PP)(os, "cell (" + functional_unit_name + ") {\n\n");

   (*_PP)(os, "#area characterization\n");
   (*_PP)(os, "area               	: " + boost::lexical_cast<std::string>(area_m->get_area_value()) + "\n");
   (*_PP)(os, "\n");

   (*_PP)(os, "#power characterization\n");
   (*_PP)(os, "\n");

   (*_PP)(os, "#pin-to-pin timing characterization\n");
   (*_PP)(os, "\n");

   (*_PP)(os, "}\n\n");
}

void functional_unit::lef_write(std::ofstream&, const simple_indentRef)
{
}

functional_unit_template::functional_unit_template() : FU(new functional_unit), no_constant_characterization(false)
{
}

functional_unit_template::functional_unit_template(const xml_nodeRef XML_description) : FU(new functional_unit(XML_description)), no_constant_characterization(false)
{
}

void functional_unit_template::xload(const xml_element* Enode, const technology_nodeRef tnd, const ParameterConstRef Param, const target_deviceRef device)
{
   const xml_node::node_list list_int = Enode->get_children();
   for(const auto& iter_int : list_int)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter_int);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == GET_CLASS_NAME(specialized))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         specialized = text->get_content();
         xml_node::convert_escaped(specialized);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(characterizing_constant_value))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         characterizing_constant_value = text->get_content();
         xml_node::convert_escaped(characterizing_constant_value);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_type))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         memory_type = text->get_content();
         xml_node::convert_escaped(memory_type);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(channels_type))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         channels_type = text->get_content();
         xml_node::convert_escaped(channels_type);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(memory_ctrl_type))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         memory_ctrl_type = text->get_content();
         xml_node::convert_escaped(memory_ctrl_type);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(bram_load_latency))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("specialization identifier is missing for " + EnodeC->get_name());
         bram_load_latency = text->get_content();
         xml_node::convert_escaped(bram_load_latency);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(no_constant_characterization))
      {
         no_constant_characterization = true;
      }
   }
   FU->xload(Enode, tnd, Param, device);
}

void functional_unit_template::xwrite(xml_element* rootnode, const technology_nodeRef tnd, const ParameterConstRef Param, TargetDevice_Type type)
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
   FU->xwrite(rootnode, tnd, Param, type);
}

void functional_unit_template::lib_write(std::ofstream&, const simple_indentRef)
{
   THROW_UNREACHABLE("This function has not to be called");
}

void functional_unit_template::lef_write(std::ofstream&, const simple_indentRef)
{
   THROW_UNREACHABLE("This function has not to be called");
}

void functional_unit_template::print(std::ostream& os) const
{
   FU->print(os);
}
