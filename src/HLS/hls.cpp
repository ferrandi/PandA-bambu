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
 * @file hls.cpp
 * @brief Data structure implementation for high-level synthesis flow.
 *
 * This file contains all the implementations used by hls class to manage the
 * high level synthesis flow
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "hls.hpp"
#include "fsm/FSMInfo.hpp"

#include "BambuParameter.hpp"
#include "allocation_information.hpp"
#include "chaining_information.hpp"
#include "conn_binding.hpp"
#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "fu_binding.hpp"
#include "hls_constraints.hpp"
#include "hls_device.hpp"
#include "op_graph.hpp"
#include "polixml.hpp"
#include "reg_binding.hpp"
#include "schedule.hpp"
#include "standard_hls.hpp"
#include "storage_value_information.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"
#include "utility.hpp"
#include "virtual_hls.hpp"
#include "xml_helper.hpp"

#include <utility>

static void computeResources(const structural_objectRef circ, const technology_managerRef TM,
                             std::map<std::string, unsigned int>& resources);

/*************************************************************************************************
 *                                                                                               *
 *                                           HLS methods                                         *
 *                                                                                               *
 *************************************************************************************************/

hls::hls(const ParameterConstRef _Param, unsigned int _function_id, OpVertexSet _operations, const HLS_deviceRef _HLS_T,
         const HLS_constraintsRef _HLS_C)
    : functionId(_function_id),
      module_binding_algorithm(HLSFlowStep_Type::UNKNOWN),
      chaining_algorithm(HLSFlowStep_Type::UNKNOWN),
      liveVariableAlgorithm(HLSFlowStep_Type::UNKNOWN),
      operations(std::move(_operations)),
      HLS_D(_HLS_T),
      HLS_C(_HLS_C),
      allocation_information(nullptr),
      Rsch(nullptr),
      Rfu(nullptr),
      fsm_info(nullptr),
      RregGroup(nullptr),
      Rconn(nullptr),
      chaining_information(nullptr),
      registered_inputs(false),
      registered_done_port(false),
      call_sites_number(0),
      datapath(nullptr),
      controller(nullptr),
      top(nullptr),
      Param(_Param),
      debug_level(_Param->getOption<int>(OPT_debug_level)),
      output_level(_Param->getOption<int>(OPT_output_level)),
      HLS_execution_time(0)
{
   THROW_ASSERT(HLS_D, "HLS initialization: HLS_device not available");
   THROW_ASSERT(HLS_C, "HLS initialization: HLS_constraints not available");
   THROW_ASSERT(Param, "HLS initialization: Parameter not available");
}

void hls::xload(const xml_element* node, const OpGraph& data)
{
   ScheduleRef sch = this->Rsch;
   fu_binding& fu = *(this->Rfu);
   unsigned int tot_cstep = 0;

   std::map<std::string, OpGraph::vertex_descriptor> String2Vertex;
   std::map<std::pair<std::string, std::string>, std::list<unsigned int>> String2Id;

   for(auto operation : operations)
   {
      String2Vertex[data.CGetNodeInfo(operation).vertex_name] = operation;
   }

   for(unsigned int id = 0; id < allocation_information->get_number_fu_types(); id++)
   {
      String2Id[allocation_information->get_fu_name(id)].push_back(id);
   }
   // Recurse through child nodes:
   const auto list = node->get_children();
   for(const auto& iter : list)
   {
      const auto* Enode = GetPointer<const xml_element>(iter);
      if(!Enode || Enode->get_name() != "scheduling")
      {
         continue;
      }
      const auto list1 = Enode->get_children();
      for(const auto& iter1 : list1)
      {
         const auto* EnodeC = GetPointer<const xml_element>(iter1);
         if(!EnodeC)
         {
            continue;
         }
         if(EnodeC->get_name() == "scheduling_constraints")
         {
            std::string vertex_name;
            unsigned int cstep = 0u;
            LOAD_XVM(vertex_name, EnodeC);
            THROW_ASSERT(vertex_name != "", "bad formed xml file: vertex_name expected in a hls specification");
            if(CE_XVM(cstep, EnodeC))
            {
               LOAD_XVM(cstep, EnodeC);
            }
            else
            {
               THROW_ERROR("bad formed xml file: cstep expected in a hls specification for operation " + vertex_name);
            }
            if(cstep > tot_cstep)
            {
               tot_cstep = cstep;
            }

            unsigned int fu_index;
            LOAD_XVM(fu_index, EnodeC);

            std::string fu_name;
            std::string library = LIBRARY_STD;
            LOAD_XVM(fu_name, EnodeC);
            if(CE_XVM(library, EnodeC))
            {
               LOAD_XVM(library, EnodeC);
            }
            unsigned int fu_type;
            if(allocation_information->is_artificial_fu(String2Id[std::make_pair(fu_name, library)].front()) ||
               allocation_information->is_assign(String2Id[std::make_pair(fu_name, library)].front()))
            {
               fu_type = String2Id[std::make_pair(fu_name, library)].front();
               String2Id[std::make_pair(fu_name, library)].pop_front();
            }
            else
            {
               fu_type = String2Id[std::make_pair(fu_name, library)].front();
            }

            sch->set_execution(String2Vertex[vertex_name], cstep);
            fu.bind(String2Vertex[vertex_name], fu_type, fu_index);
         }
      }
   }
   sch->set_csteps(tot_cstep + 1u);
}

void hls::xwrite(xml_element* rootnode, const OpGraph& data)
{
   const ScheduleRef sch = this->Rsch;
   fu_binding& fu = *(this->Rfu);

   xml_element* Enode = rootnode->add_child_element("scheduling");

   for(auto operation : operations)
   {
      xml_element* EnodeC = Enode->add_child_element("scheduling_constraints");
      std::string vertex_name = data.CGetNodeInfo(operation).vertex_name;
      const auto cstep = sch->get_cstep(operation).second;
      WRITE_XVM(vertex_name, EnodeC);
      WRITE_XVM(cstep, EnodeC);

      unsigned int fu_type = fu.get_assign(operation);
      unsigned int fu_index = fu.get_index(operation);
      const auto [fu_name, library] = allocation_information->get_fu_name(fu_type);

      WRITE_XVM(fu_name, EnodeC);
      WRITE_XVM(fu_index, EnodeC);
      if(library != LIBRARY_STD)
      {
         WRITE_XVM(library, EnodeC);
      }
   }

   if(datapath)
   {
      Enode = rootnode->add_child_element("resource_allocation");
      std::map<std::string, unsigned int> resources;
      const technology_managerRef TM = HLS_D->get_technology_manager();
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

static void computeResources(const structural_objectRef circ, const technology_managerRef TM,
                             std::map<std::string, unsigned int>& resources)
{
   const module_o* mod = GetPointer<module_o>(circ);
   auto processResources = [&](const structural_objectRef c) {
      THROW_ASSERT(c, "unexpected condition");
      const structural_type_descriptorRef id_type = c->get_typeRef();
      if(c->get_kind() != module_o_K)
      {
         return;
      }
      computeResources(c, TM, resources);
      if(c->get_id() == "Controller_i" || c->get_id() == "Datapath_i")
      {
         return;
      }
      std::string library = TM->get_library(id_type->id_type);
      if(library == WORK_LIBRARY || library == PROXY_LIBRARY)
      {
         return;
      }
      resources[id_type->id_type]++;
   };
   for(unsigned int l = 0; l < mod->get_internal_objects_size(); l++)
   {
      const structural_objectRef obj = mod->get_internal_object(l);
      processResources(obj);
   }
   NP_functionalityRef NPF = mod->get_NP_functionality();
   if(NPF and NPF->exist_NP_functionality(NP_functionality::IP_COMPONENT))
   {
      std::string ip_cores = NPF->get_NP_functionality(NP_functionality::IP_COMPONENT);
      std::vector<std::string> ip_cores_list = string_to_container<std::vector<std::string>>(ip_cores, ",");
      for(const auto& ip_core : ip_cores_list)
      {
         std::vector<std::string> ip_core_vec = string_to_container<std::vector<std::string>>(ip_core, ":");
         if(ip_core_vec.size() < 1 or ip_core_vec.size() > 2)
         {
            THROW_ERROR("Malformed IP component definition \"" + ip_core + "\"");
         }
         std::string library, component_name;
         if(ip_core_vec.size() == 2)
         {
            library = ip_core_vec[0];
            component_name = ip_core_vec[1];
         }
         else
         {
            component_name = ip_core_vec[0];
            library = TM->get_library(component_name);
         }
         technology_nodeRef tn = TM->get_fu(component_name, library);
         structural_objectRef core_cir;
         if(tn->get_kind() == functional_unit_K)
         {
            core_cir = GetPointer<functional_unit>(tn)->CM->get_circ();
         }
         else if(tn->get_kind() == functional_unit_template_K &&
                 GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
         {
            core_cir = GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
         }
         else
         {
            THROW_ERROR("Unexpected pattern");
         }
         processResources(core_cir);
      }
   }
}

void hls::PrintResources() const
{
   THROW_ASSERT(datapath, "datapath not yet created!");
   std::map<std::string, unsigned int> resources;
   const technology_managerRef TM = HLS_D->get_technology_manager();
   computeResources(datapath->get_circ(), TM, resources);
   if(output_level <= OUTPUT_LEVEL_PEDANTIC)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "");
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "-->Summary of resources:");
   for(auto r = resources.begin(); r != resources.end(); ++r)
   {
      INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "--- - " + r->first + ": " + STR(r->second));
   }
   INDENT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "<--");
}
