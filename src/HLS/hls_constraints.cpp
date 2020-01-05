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
 * @file hls_constraints.cpp
 * @brief Data structure implementation for HLS constraints.
 *
 * This file contains the implementation of all the methods used to constrain the synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "hls_constraints.hpp"
#include "exceptions.hpp"
#include "technology_manager.hpp"
#include "utility.hpp"

#include "custom_map.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"
#include "xml_helper.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "fileIO.hpp"
#include <boost/filesystem.hpp>
#include <iosfwd>
#include <string>

/// STL includes
#include <utility>
#include <vector>

/// utility include
#include "string_manipulation.hpp"

const double HLS_constraints::clock_period_resource_fraction_DEFAULT = 1.0;

/// function used to extract the functional unit name and its library from a string.
void DECODE_FU_LIB(std::string& fu_name, std::string& fu_library, const std::string& combined)
{
   std::vector<std::string> splitted = SplitString(combined, ":");
   fu_name = splitted[0];
   fu_library = splitted[1];
}

HLS_constraints::HLS_constraints(const ParameterConstRef& _Param, std::string _fun_name)
    : clock_period(_Param->getOption<double>(OPT_clock_period)), clock_period_resource_fraction(clock_period_resource_fraction_DEFAULT), registers(INFINITE_UINT), fun_name(std::move(_fun_name)), parameters(_Param)
{
   /// add the general builtin constraints
   add_builtin_constraints();

   auto debug_level = parameters->getOption<unsigned int>(OPT_debug_level);
   if(parameters->isOption(OPT_xml_input_configuration))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "parsing the configuration file for constraints...");
      auto fn = parameters->getOption<std::string>(OPT_xml_input_configuration);
      try
      {
         xml_element* constraint_node = nullptr;
         XMLDomParser parser(fn);
         parser.Exec();
         if(parser)
         {
            xml_element* node = parser.get_document()->get_root_node();
            xml_node::node_list list = node->get_children();
            for(auto& iter : list)
            {
               auto* Enode = GetPointer<xml_element>(iter);
               if(!Enode || Enode->get_name() != GET_CLASS_NAME(HLS_constraints))
               {
                  continue;
               }
               std::string function_name;
               if(!fun_name.empty()) /* constraints related to a specific function */
               {
                  if(CE_XVM(function_name, Enode))
                  {
                     LOAD_XVM(function_name, Enode);
                  }
                  if(function_name == fun_name)
                  {
                     constraint_node = Enode;
                  }
               }
               else /* general constraints */
               {
                  if(!CE_XVM(function_name, Enode))
                  {
                     constraint_node = Enode;
                  }
               }
            }
         }
         if(constraint_node)
         {
            xload(constraint_node);
         }
      }
      catch(const char* msg)
      {
         THROW_ERROR("Error during constraints file parsing: " + std::string(msg));
      }
      catch(const std::string& msg)
      {
         THROW_ERROR("Error during constraints file parsing: " + msg);
      }
      catch(const std::exception& ex)
      {
         THROW_ERROR("Error during constraints file parsing: " + std::string(ex.what()));
      }
      catch(...)
      {
         THROW_ERROR("Error during constraints file parsing");
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, " ==== XML configuration file parsed for constraints information ====");
      if(!fun_name.empty())
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, " Constraints of function: " + fun_name);
      }
      else
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, " Global constraints: ");
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, *this);
   }

   /// add user defined constraints
   if(parameters->isOption(OPT_constraints_file))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "parsing the constraint file...");
      read_HLS_constraints_File(parameters->getOption<std::string>(OPT_constraints_file));
      PRINT_DBG_MEX(DEBUG_LEVEL_MINIMUM, debug_level, "Constraint file parsed");
   }

   /// the command-line values overwrite the ones written into the XML file
   if(parameters->isOption(OPT_clock_period))
   {
      clock_period = parameters->getOption<double>(OPT_clock_period);
   }
   if(parameters->isOption(OPT_clock_period_resource_fraction))
   {
      clock_period_resource_fraction = parameters->getOption<double>(OPT_clock_period_resource_fraction);
   }

   // ---------- Save XML file ------------ //
   if(debug_level >= DEBUG_LEVEL_PEDANTIC)
   {
      write_HLS_constraints_File("HLS_constraints.out.XML");
   }
}

std::string HLS_constraints::get_function_name() const
{
   return fun_name;
}

void HLS_constraints::set_number_fu(const std::string& name, const std::string& library, unsigned int n_resources)
{
   tech_constraints[ENCODE_FU_LIB(name, library)] = n_resources;
}

unsigned int HLS_constraints::get_number_fu(const std::string& name, const std::string& library) const
{
   if(tech_constraints.find(ENCODE_FU_LIB(name, library)) == tech_constraints.end())
   {
      return INFINITE_UINT;
   }
   else
   {
      return tech_constraints.find(ENCODE_FU_LIB(name, library))->second;
   }
}

unsigned int HLS_constraints::get_number_fu(const std::string& combined) const
{
   std::string fu_name;
   std::string lib_name;
   DECODE_FU_LIB(fu_name, lib_name, combined);
   return get_number_fu(fu_name, lib_name);
}

void HLS_constraints::bind_vertex_to_fu(const std::string& vertex_name, const std::string& fu_name, const std::string& fu_library, const unsigned int fu_index)
{
   binding_constraints[vertex_name] = std::make_pair(fu_name, std::make_pair(fu_library, fu_index));
}

bool HLS_constraints::has_binding_to_fu(const std::string& vertex_name) const
{
   return binding_constraints.find(vertex_name) != binding_constraints.end();
}

void HLS_constraints::set_clock_period(double period)
{
   clock_period = period;
}

void HLS_constraints::set_max_registers(unsigned int n_resources)
{
   registers = n_resources;
}

#if 0
/**
 * Check if HLS contraints imposed on high-level synthesis have been respected. It throws an exection if:
 *    - Does not exist a functional unit for an operation to be executed
 *    - Functional units consumption is too high (functional unit power is greater than maximum power per cycle)
 *    - Does not exist enough functional unit for an operation.
 * @param GM is the reference to behavioral_manager class, where all information about graphs are stored
 * @param TM is the rerefence to technology_manager class, where all information about architecture are stored
 */
void HLS_constraints::check_HLS_constraints(const behavioral_managerRef & GM, const technology_managerRef & TM) const
{
  VertexIterator v, v_end;
  std::string fu_name;
  bool flag;
  double fu_max_power;
  const graph * cfg = GM->CGetOpGraph(behavioral_manager::CFG);
  HLS_constraints_functor<base_constraints_functor> CF(*this);

  for (boost::tie(v, v_end) = boost::vertices(*cfg); v != v_end; v++)
  {
    (void) TM->get_attribute_of_fu_per_op(GET_OP(cfg, *v), technology_manager::min, technology_manager::execution_time, fu_name, flag);
    if (!flag)
      THROW_ERROR(std::string("Does not exist a functional unit for operation ") + GET_OP(cfg, *v));
    fu_max_power = TM->get_attribute_of_fu_per_op(GET_OP(cfg, *v), technology_manager::max, technology_manager::power_consumption, fu_name, flag);
    if (max_power_per_cycle < fu_max_power)
      THROW_ERROR(std::string("Functional units consumption is too high ") + GET_OP(cfg, *v));
    if (!TM->check_technology_constraint(GET_OP(cfg, *v), CF))
      THROW_ERROR(std::string("Does not exist enough functional unit for operation \"") + GET_OP(cfg, *v) + std::string("\". Check the number of resources that can implement that operation"));
  }
}
#endif

void HLS_constraints::print(std::ostream& os) const
{
   if(clock_period != 0)
   {
      os << "- Clock period: " << clock_period << "\n";
   }
   if(registers == INFINITE_UINT)
   {
      os << "- Registers: " << registers << "\n";
   }
   if(!tech_constraints.empty())
   {
      os << "- Resource constraints:\n";
      auto i_end = tech_constraints.end();
      for(auto i = tech_constraints.begin(); i != i_end; ++i)
      {
         os << i->first << " " << i->second << std::endl;
      }
   }
   if(!binding_constraints.empty())
   {
      os << "- Binding constraints:\n";
      auto i_end = binding_constraints.end();
      for(auto i = binding_constraints.begin(); i != i_end; ++i)
      {
         os << i->first << " " << i->second.first << " " << i->second.second.first << " " << i->second.second.second << std::endl;
      }
   }
   if(!scheduling_constraints.empty())
   {
      os << "- Scheduling constraints:\n";
      auto i_end = scheduling_constraints.end();
      for(auto i = scheduling_constraints.begin(); i != i_end; ++i)
      {
         os << i->first << " " << i->second << std::endl;
      }
   }
}

void HLS_constraints::xload(const xml_element* Enode)
{
   // Recurse through child nodes:
   const xml_node::node_list& list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
      {
         continue;
      }
      if(EnodeC->get_name() == "tech_constraints")
      {
         std::string fu_name;
         std::string fu_library = LIBRARY_STD;
         unsigned int n = 0;
         LOAD_XVM(fu_name, EnodeC);
         THROW_ASSERT(!fu_name.empty(), "bad formed xml file: fu_name expected in a tech_constraints");
         if(CE_XVM(fu_library, EnodeC))
         {
            LOAD_XVM(fu_library, EnodeC);
         }
         LOAD_XVM(n, EnodeC);
         tech_constraints[ENCODE_FU_LIB(fu_name, fu_library)] = n;
      }
      else if(EnodeC->get_name() == "binding_constraints")
      {
         std::string vertex_name;
         std::string fu_name;
         std::string fu_library = LIBRARY_STD;
         unsigned int fu_index = 0;
         LOAD_XVM(vertex_name, EnodeC);
         THROW_ASSERT(!vertex_name.empty(), "bad formed xml file: vertex_name expected in a binding_constraints");
         LOAD_XVM(fu_name, EnodeC);
         THROW_ASSERT(!fu_name.empty(), "bad formed xml file: fu_name expected in a binding_constraints");
         if(CE_XVM(fu_library, EnodeC))
         {
            LOAD_XVM(fu_library, EnodeC);
         }
         LOAD_XVM(fu_index, EnodeC);
         binding_constraints[vertex_name] = std::make_pair(fu_name, std::make_pair(fu_library, fu_index));
      }
      else if(EnodeC->get_name() == "scheduling_constraints")
      {
         std::string vertex_name;
         int priority = 0;
         LOAD_XVM(vertex_name, EnodeC);
         THROW_ASSERT(!vertex_name.empty(), "bad formed xml file: vertex_name expected in a scheduling_constraints");
         LOAD_XVM(priority, EnodeC);
         scheduling_constraints[vertex_name] = priority;
      }
   }
   if(CE_XVM(clock_period, Enode))
   {
      LOAD_XVM(clock_period, Enode);
   }
   if(CE_XVM(registers, Enode))
   {
      LOAD_XVM(registers, Enode);
   }
}

void HLS_constraints::xwriteFUConstraints(xml_element* Enode, const std::string& fu_name, const std::string& fu_library, unsigned int n, bool forDump)
{
   if(n == INFINITE_UINT and !forDump)
   {
      return;
   }
   xml_element* EnodeC = Enode->add_child_element("tech_constraints");
   WRITE_XVM(fu_name, EnodeC);
   if(fu_library != LIBRARY_STD)
   {
      WRITE_XVM(fu_library, EnodeC);
   }
   WRITE_XVM(n, EnodeC);
}

void HLS_constraints::xwriteHLSConstraints(xml_element* Enode, bool forDump)
{
   WRITE_XVM(clock_period, Enode);
   if(registers != INFINITE_UINT or forDump)
   {
      WRITE_XVM(registers, Enode);
   }
}

void HLS_constraints::xwriteBindingConstraints(xml_element* Enode, const std::string& vertex_name, const std::string& fu_name, const std::string& fu_library, unsigned int fu_index)
{
   xml_element* EnodeC = Enode->add_child_element("binding_constraints");
   WRITE_XVM(vertex_name, EnodeC);
   WRITE_XVM(fu_name, EnodeC);
   if(fu_library != LIBRARY_STD)
   {
      WRITE_XVM(fu_library, EnodeC);
   }
   WRITE_XVM(fu_index, EnodeC);
}

void HLS_constraints::xwriteSchedulingConstraints(xml_element* Enode, const std::string& vertex_name, int priority)
{
   xml_element* EnodeC = Enode->add_child_element("scheduling_constraints");
   WRITE_XVM(vertex_name, EnodeC);
   WRITE_XVM(priority, EnodeC);
}

void HLS_constraints::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   auto i_end = tech_constraints.end();
   for(auto i = tech_constraints.begin(); i != i_end; ++i)
   {
      std::string fu_name;
      std::string fu_library;
      unsigned int n = i->second;
      DECODE_FU_LIB(fu_name, fu_library, i->first);
      xwriteFUConstraints(Enode, fu_name, fu_library, n);
   }

   auto bi_end = binding_constraints.end();
   for(auto bi = binding_constraints.begin(); bi != bi_end; ++bi)
   {
      std::string vertex_name = bi->first;
      std::string fu_name = bi->second.first;
      std::string fu_library = bi->second.second.first;
      unsigned int fu_index = bi->second.second.second;
      xwriteBindingConstraints(Enode, vertex_name, fu_name, fu_library, fu_index);
   }

   auto sch_end = scheduling_constraints.end();
   for(auto sch = scheduling_constraints.begin(); sch != sch_end; ++sch)
   {
      std::string vertex_name = sch->first;
      int priority = sch->second;
      xwriteSchedulingConstraints(Enode, vertex_name, priority);
   }
   xwriteHLSConstraints(Enode);
}

void HLS_constraints::set_scheduling_priority(const std::string& Ver, int Priority)
{
   scheduling_constraints[Ver] = Priority;
}

CustomMap<std::string, int> HLS_constraints::get_scheduling_priority() const
{
   return scheduling_constraints;
}

void HLS_constraints::add_builtin_constraints()
{
   /// Load default constraints on default resources
   const char* builtin_constraints_data = {
#include "constraints_STD.data"
   };
   const char* builtin_constraints_data_ALUs = {
#include "constraints_STD_ALUs.data"
   };
   try
   {
      XMLDomParser parser_noALUs("builtin_constraints_data", builtin_constraints_data);
      XMLDomParser parser_ALUs("builtin_constraints_data_ALUs", builtin_constraints_data_ALUs);
      XMLDomParser& parser = parameters->isOption(OPT_use_ALUs) && parameters->getOption<bool>(OPT_use_ALUs) ? parser_ALUs : parser_noALUs;
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.

         // Recurse through child nodes:
         const auto& list = node->get_children();
         for(const auto& iter : list)
         {
            const auto* Enode = GetPointer<const xml_element>(iter);
            if(!Enode || Enode->get_name() != GET_CLASS_NAME(HLS_constraints))
            {
               continue;
            }
            xload(Enode);
         }
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error during parsing of constraint file");
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error during parsing of constraint file");
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
      THROW_ERROR("Error during parsing of constraint file");
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
      THROW_ERROR("Error during parsing of constraint file");
   }
}

void HLS_constraints::read_HLS_constraints_File(const std::string& fn)
{
   try
   {
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.

         // Recurse through child nodes:
         const auto& list = node->get_children();
         for(const auto& iter : list)
         {
            const auto* Enode = GetPointer<const xml_element>(iter);
            if(!Enode || Enode->get_name() != GET_CLASS_NAME(HLS_constraints))
            {
               continue;
            }
            xload(Enode);
         }
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}

void HLS_constraints::write_HLS_constraints_File(const std::string& f)
{
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node(GET_CLASS_NAME(HLS_constraints));
      xwrite(nodeRoot);
      document.write_to_file_formatted(f);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}
