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
 * @file hls.cpp
 * @brief Data structure implementation for high-level synthesis flow.
 *
 * This file contains all the implementations used by hls class to manage the
 * high level synthesis flow
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
/// Header include
#include "hls.hpp"

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

#include "hls_target.hpp"
#include "technology_manager.hpp"

#include "hls_constraints.hpp"

#include "conn_binding.hpp"
#include "fu_binding.hpp"
#include "reg_binding.hpp"
#include "schedule.hpp"

#include "standard_hls.hpp"
#include "virtual_hls.hpp"

#include "structural_manager.hpp"

#include "BambuParameter.hpp"

#include "exceptions.hpp"
#include "polixml.hpp"
#include "utility.hpp"
#include "xml_helper.hpp"

#include <boost/lexical_cast.hpp>
#include <utility>

#include "op_graph.hpp"

/// HLS/binding/storage_value_insertion includes
#include "storage_value_information.hpp"

/// HLS/chaining_information
#include "chaining_information.hpp"

/// HLS/module_allocation
#include "allocation_information.hpp"

#include "dbgPrintHelper.hpp"

static void computeResources(const structural_objectRef circ, const technology_managerRef TM, std::map<std::string, unsigned int>& resources);

/*************************************************************************************************
 *                                                                                               *
 *                                           HLS methods                                         *
 *                                                                                               *
 *************************************************************************************************/

hls::hls(const ParameterConstRef _Param, unsigned int _function_id, OpVertexSet _operations, const HLS_targetRef _HLS_T, const HLS_constraintsRef _HLS_C)
    : functionId(_function_id),
      operations(std::move(_operations)),
      HLS_T(_HLS_T),
      HLS_C(_HLS_C),
      allocation_information(),
      registered_inputs(false),
      registered_done_port(false),
      call_sites_number(0),
      Param(_Param),
      debug_level(_Param->getOption<int>(OPT_debug_level)),
      output_level(_Param->getOption<int>(OPT_output_level)),
      HLS_execution_time(0)
{
   THROW_ASSERT(HLS_T, "HLS initialization: HLS_target not available");
   THROW_ASSERT(HLS_C, "HLS initialization: HLS_constraints not available");
   THROW_ASSERT(Param, "HLS initialization: Parameter not available");
}

hls::~hls() = default;

void hls::xload(const xml_element* node, const OpGraphConstRef data)
{
   ScheduleRef sch = this->Rsch;
   fu_binding& fu = *(this->Rfu);
   unsigned int tot_cstep = 0;

   std::map<std::string, vertex> String2Vertex;
   std::map<std::pair<std::string, std::string>, std::list<unsigned int>> String2Id;

   for(auto operation : operations)
   {
      String2Vertex[GET_NAME(data, operation)] = operation;
   }

   for(unsigned int id = 0; id < allocation_information->get_number_fu_types(); id++)
   {
      String2Id[allocation_information->get_fu_name(id)].push_back(id);
   }
   // Recurse through child nodes:
   const xml_node::node_list list = node->get_children();
   for(const auto& iter : list)
   {
      const auto* Enode = GetPointer<const xml_element>(iter);
      if(!Enode || Enode->get_name() != "scheduling")
         continue;
      const xml_node::node_list list1 = Enode->get_children();
      for(const auto& iter1 : list1)
      {
         const auto* EnodeC = GetPointer<const xml_element>(iter1);
         if(!EnodeC)
            continue;
         if(EnodeC->get_name() == "scheduling_constraints")
         {
            std::string vertex_name;
            unsigned int cstep = 0u;
            LOAD_XVM(vertex_name, EnodeC);
            THROW_ASSERT(vertex_name != "", "bad formed xml file: vertex_name expected in a hls specification");
            if(CE_XVM(cstep, EnodeC))
               LOAD_XVM(cstep, EnodeC);
            else
               THROW_ERROR("bad formed xml file: cstep expected in a hls specification for operation " + vertex_name);
            if(cstep > tot_cstep)
               tot_cstep = cstep;

            unsigned int fu_index;
            LOAD_XVM(fu_index, EnodeC);

            std::string fu_name;
            std::string library = LIBRARY_STD;
            LOAD_XVM(fu_name, EnodeC);
            if(CE_XVM(library, EnodeC))
               LOAD_XVM(library, EnodeC);
            unsigned int fu_type;
            if(allocation_information->is_artificial_fu(String2Id[std::make_pair(fu_name, library)].front()) || allocation_information->is_assign(String2Id[std::make_pair(fu_name, library)].front()))
            {
               fu_type = String2Id[std::make_pair(fu_name, library)].front();
               String2Id[std::make_pair(fu_name, library)].pop_front();
            }
            else
            {
               fu_type = String2Id[std::make_pair(fu_name, library)].front();
            }

            sch->set_execution(String2Vertex[vertex_name], ControlStep(cstep));
            fu.bind(String2Vertex[vertex_name], fu_type, fu_index);
         }
      }
   }
   sch->set_csteps(ControlStep(tot_cstep + 1u));
}

void hls::xwrite(xml_element* rootnode, const OpGraphConstRef data)
{
   const ScheduleRef sch = this->Rsch;
   fu_binding& fu = *(this->Rfu);

   xml_element* Enode = rootnode->add_child_element("scheduling");

   for(auto operation : operations)
   {
      xml_element* EnodeC = Enode->add_child_element("scheduling_constraints");
      std::string vertex_name = GET_NAME(data, operation);
      const auto cstep = sch->get_cstep(operation).second;
      WRITE_XVM(vertex_name, EnodeC);
      WRITE_XVM(cstep, EnodeC);

      unsigned int fu_type = fu.get_assign(operation);
      unsigned int fu_index = fu.get_index(operation);
      std::string fu_name, library;
      boost::tie(fu_name, library) = allocation_information->get_fu_name(fu_type);

      WRITE_XVM(fu_name, EnodeC);
      WRITE_XVM(fu_index, EnodeC);
      if(library != LIBRARY_STD)
         WRITE_XVM(library, EnodeC);
   }

   if(datapath)
   {
      Enode = rootnode->add_child_element("resource_allocation");
      std::map<std::string, unsigned int> resources;
      const technology_managerRef TM = HLS_T->get_technology_manager();
      computeResources(datapath->get_circ(), TM, resources);
      for(auto& resource : resources)
      {
         xml_element* EnodeC = Enode->add_child_element("resource");
         std::string name = resource.first;
         unsigned int number = resource.second;
         WRITE_XVM(name, EnodeC);
         WRITE_XVM(number, EnodeC);
      }
   }
}

static void computeResources(const structural_objectRef circ, const technology_managerRef TM, std::map<std::string, unsigned int>& resources)
{
   const module* mod = GetPointer<module>(circ);
   for(unsigned int l = 0; l < mod->get_internal_objects_size(); l++)
   {
      const structural_objectRef obj = mod->get_internal_object(l);
      const structural_type_descriptorRef id_type = obj->get_typeRef();
      if(obj->get_kind() != component_o_K)
         continue;
      computeResources(obj, TM, resources);
      if(obj->get_id() == "Controller_i" || obj->get_id() == "Datapath_i")
         continue;
      std::string library = TM->get_library(id_type->id_type);
      if(library == WORK_LIBRARY || library == PROXY_LIBRARY)
         continue;
      resources[id_type->id_type]++;
   }
}

void hls::PrintResources() const
{
   THROW_ASSERT(datapath, "datapath not yet created!");
   std::map<std::string, unsigned int> resources;
   const technology_managerRef TM = HLS_T->get_technology_manager();
   computeResources(datapath->get_circ(), TM, resources);
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Summary of resources:");
   for(auto r = resources.begin(); r != resources.end(); ++r)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "--- - " + r->first + ": " + STR(r->second));
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
}
