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
 * @file module_interface.cpp
 * @brief Base class for all module interfaces
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 * $Locker:  $
 * $State: Exp $
 *
 */
#include "module_interface.hpp"

#include "behavioral_helper.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "omp_functions.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"
#include "memory.hpp"
#include "memory_symbol.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

#include "polixml.hpp"
#include "xml_helper.hpp"

#include "Parameter.hpp"
#include "constant_strings.hpp"

/// HLS/module_allocation includes
#include "add_library.hpp"

/// STL includes
#include "custom_set.hpp"
#include <tuple>

/// utility include
#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_

module_interface::module_interface(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLSFunctionStep(_parameters, _HLSMgr, _funId, _design_flow_manager, _hls_flow_step_type)
{
   THROW_ASSERT(_parameters, "Parameter null");
}

module_interface::~module_interface() = default;

const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> module_interface::ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ret;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         const auto cg_man = HLSMgr->CGetCallGraphManager();
         if(HLSMgr->hasToBeInterfaced(funId) and (cg_man->ExistsAddressedFunction() or hls_flow_step_type == HLSFlowStep_Type::WB4_INTERFACE_GENERATION))
         {
            ret.insert(std::make_tuple(HLSFlowStep_Type::TOP_ENTITY_MEMORY_MAPPED_CREATION, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
         }
         else
         {
            ret.insert(std::make_tuple(parameters->getOption<HLSFlowStep_Type>(OPT_function_allocation_algorithm), HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION)); // add dependence to omp_function
            if(HLSMgr->Rfuns)
            {
               bool found = false;
               if(parameters->isOption(OPT_context_switch))
               {
                  auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
                  THROW_ASSERT(omp_functions, "OMP_functions must not be null");
                  if(omp_functions->kernel_functions.find(funId) != omp_functions->kernel_functions.end())
                     found = true;
                  if(omp_functions->parallelized_functions.find(funId) != omp_functions->parallelized_functions.end())
                     found = true;
                  if(omp_functions->atomic_functions.find(funId) != omp_functions->atomic_functions.end())
                     found = true;
                  if(found) // use new top_entity
                  {
                     const HLSFlowStep_Type top_entity_type = HLSFlowStep_Type::TOP_ENTITY_CS_CREATION;
                     ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
                  }
               }
               if(!found) // use standard
               {
                  const HLSFlowStep_Type top_entity_type = HLSFlowStep_Type::TOP_ENTITY_CREATION;
                  ret.insert(std::make_tuple(top_entity_type, HLSFlowStepSpecializationConstRef(), HLSFlowStep_Relationship::SAME_FUNCTION));
               }
            }
         }
         break;
      }
      case INVALIDATION_RELATIONSHIP:
      {
         break;
      }
      case PRECEDENCE_RELATIONSHIP:
      {
         ret.insert(std::make_tuple(HLSFlowStep_Type::ADD_LIBRARY, HLSFlowStepSpecializationConstRef(new AddLibrarySpecialization(false)), HLSFlowStep_Relationship::SAME_FUNCTION));
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return ret;
}

void module_interface::add_sign(const structural_managerRef SM, const structural_objectRef sig1, const structural_objectRef sig2, const std::string& sig_name)
{
   structural_objectRef sig = SM->add_sign(sig_name, SM->get_circ(), sig1->get_typeRef());
   SM->add_connection(sig, sig1);
   SM->add_connection(sig, sig2);
}

void module_interface::add_sign_vector(const structural_managerRef SM, const structural_objectRef sig1, const structural_objectRef sig2, const std::string& sig_name)
{
   THROW_ASSERT(GetPointer<port_o>(sig1)->get_ports_size(), "");
   structural_objectRef sig = SM->add_sign_vector(sig_name, GetPointer<port_o>(sig1)->get_ports_size(), SM->get_circ(), sig1->get_typeRef());
   SM->add_connection(sig, sig1);
   SM->add_connection(sig, sig2);
}

void module_interface::AddSignal(const structural_managerRef SM, const structural_objectRef component1, const std::string& port1_name, const structural_objectRef component2, const std::string& port2_name, const std::string& signal_name)
{
   structural_objectRef port1;
   unsigned int size1;
   if(port1_name.find("[") != std::string::npos)
   {
      const auto port1_base_name = port1_name.substr(0, port1_name.find("["));
      const auto port1_index = port1_name.substr(port1_name.find("[") + 1, port1_name.size() - port1_base_name.size() - 2);
      const auto port1_vector = component1->find_member(port1_base_name, port_o_K, component1);
      THROW_ASSERT(port1_vector, port1_name + " is not in " + component1->get_path());
      port1 = GetPointer<port_o>(port1_vector)->get_port(boost::lexical_cast<unsigned int>(port1_index));
      size1 = GetPointer<port_o>(port1_vector)->get_typeRef()->vector_size;
   }
   else
   {
      port1 = component1->find_member(port1_name, port_o_K, component1);
      THROW_ASSERT(port1, port1_name + " is not in " + component1->get_path());
      size1 = GET_TYPE_SIZE(port1);
   }
   structural_objectRef port2;
   unsigned int size2;
   if(port2_name.find("[") != std::string::npos)
   {
      const auto port2_base_name = port2_name.substr(0, port2_name.find("["));
      const auto port2_index = port2_name.substr(port2_name.find("[") + 1, port2_name.size() - port2_base_name.size() - 2);
      const auto port2_vector = component2->find_member(port2_base_name, port_o_K, component2);
      THROW_ASSERT(port2_vector, port2_base_name + " is not in " + component2->get_path());
      port2 = GetPointer<port_o>(port2_vector)->get_port(boost::lexical_cast<unsigned int>(port2_index));
      size2 = GetPointer<port_o>(port2_vector)->get_typeRef()->vector_size;
   }
   else
   {
      port2 = component2->find_member(port2_name, port_o_K, component2);
      THROW_ASSERT(port2, port2_name + " is not in " + component2->get_path());
      size2 = GET_TYPE_SIZE(port2);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connecting " + component1->get_id() + "::" + port1_name + " (Size " + STR(size1) + ") to " + component2->get_id() + "::" + port2_name + " (Size " + STR(size2) + ")");
   if(size1 > size2)
   {
      GetPointer<port_o>(port2)->type_resize(size1);
   }
   if(size1 < size2)
   {
      GetPointer<port_o>(port1)->type_resize(size2);
   }
   const auto bounded_object = GetPointer<port_o>(port1)->find_bounded_object();
   if(bounded_object)
   {
      SM->add_connection(bounded_object, port2);
   }
   else
   {
      structural_objectRef sig = SM->add_sign(signal_name, SM->get_circ(), port1->get_typeRef());
      SM->add_connection(sig, port1);
      SM->add_connection(sig, port2);
   }
}

void module_interface::AddConnection(const structural_managerRef SM, const structural_objectRef component1, const std::string& port1_name, const structural_objectRef component2, const std::string& port2_name)
{
   structural_objectRef port1;
   unsigned int size1;
   if(port1_name.find("[") != std::string::npos)
   {
      const auto port1_base_name = port1_name.substr(0, port1_name.find("["));
      const auto port1_index = port1_name.substr(port1_name.find("[") + 1, port1_name.size() - port1_base_name.size() - 2);
      const auto port1_vector = component1->find_member(port1_base_name, port_o_K, component1);
      THROW_ASSERT(port1_vector, port1_name + " is not in " + component1->get_path());
      port1 = GetPointer<port_o>(port1_vector)->get_port(boost::lexical_cast<unsigned int>(port1_index));
      size1 = GetPointer<port_o>(port1_vector)->get_typeRef()->vector_size;
   }
   else
   {
      port1 = component1->find_member(port1_name, port_o_K, component1);
      THROW_ASSERT(port1, port1_name + " is not in " + component1->get_path());
      size1 = GET_TYPE_SIZE(port1);
   }
   structural_objectRef port2;
   unsigned int size2;
   if(port2_name.find("[") != std::string::npos)
   {
      const auto port2_base_name = port2_name.substr(0, port2_name.find("["));
      const auto port2_index = port2_name.substr(port2_name.find("[") + 1, port2_name.size() - port2_base_name.size() - 2);
      const auto port2_vector = component2->find_member(port2_base_name, port_o_K, component2);
      THROW_ASSERT(port2_vector, port2_base_name + " is not in " + component2->get_path());
      port2 = GetPointer<port_o>(port2_vector)->get_port(boost::lexical_cast<unsigned int>(port2_index));
      size2 = GetPointer<port_o>(port2_vector)->get_typeRef()->vector_size;
   }
   else
   {
      port2 = component2->find_member(port2_name, port_o_K, component2);
      THROW_ASSERT(port2, port2_name + " is not in " + component2->get_path());
      size2 = GET_TYPE_SIZE(port2);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Connecting " + component1->get_id() + "::" + port1_name + " (Size " + STR(size1) + ") to " + component2->get_id() + "::" + port2_name + " (Size " + STR(size2) + ")");
   if(size1 > size2)
   {
      GetPointer<port_o>(port2)->type_resize(size1);
   }
   if(size1 < size2)
   {
      GetPointer<port_o>(port1)->type_resize(size2);
   }
   SM->add_connection(port1, port2);
}

void module_interface::AddConstant(const structural_managerRef SM, const structural_objectRef component, const std::string& port_name, const std::string& constant_value, const unsigned int constant_size)
{
   structural_objectRef port;
   unsigned int size;
   if(port_name.find("[") != std::string::npos)
   {
      const auto port_base_name = port_name.substr(0, port_name.find("["));
      const auto port_index = port_name.substr(port_name.find("[") + 1, port_name.size() - port_base_name.size() - 2);
      const auto port_vector = component->find_member(port_base_name, port_o_K, component);
      THROW_ASSERT(port_vector, port_name + " is not in " + component->get_path());
      port = GetPointer<port_o>(port_vector)->get_port(boost::lexical_cast<unsigned int>(port_index));
      size = GetPointer<port_o>(port_vector)->get_typeRef()->vector_size;
   }
   else
   {
      port = component->find_member(port_name, port_o_K, component);
      THROW_ASSERT(port, port_name + " is not in " + component->get_path());
      size = GetPointer<port_o>(port)->get_port_size();
      if(size < constant_size)
         GetPointer<port_o>(port)->type_resize(constant_size);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Binding " + constant_value + " to " + component->get_id() + "::" + port_name);
   structural_objectRef constant(new constant_o(parameters->getOption<int>(OPT_debug_level), SM->get_circ(), constant_value));
   structural_type_descriptorRef constant_type = structural_type_descriptorRef(new structural_type_descriptor("bool", std::max(size, constant_size)));
   constant->set_type(constant_type);
   GetPointer<module>(SM->get_circ())->add_internal_object(constant);
   SM->add_connection(constant, port);
}
