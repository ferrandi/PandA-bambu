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
 *              Copyright (c) 2015-2020 Politecnico di Milano
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
 * @file parallel_memory_conn_binding.hpp
 * @brief Data structure used to store the interconnection binding of datapath elements when parallel memory controller is adopted
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "parallel_memory_conn_binding.hpp"

///. include
#include "Parameter.hpp"

/// circuit includes
#include "structural_manager.hpp"
#include "structural_objects.hpp"

/// HLS includes
#include "hls.hpp"
#include "hls_manager.hpp"

/// HLS/binding/module
#include "parallel_memory_fu_binding.hpp"

/// HLS/functions
#include "omp_functions.hpp"

ParallelMemoryConnBinding::ParallelMemoryConnBinding(const BehavioralHelperConstRef _behavioral_helper, const ParameterConstRef _parameters) : conn_binding(_behavioral_helper, _parameters)
{
}

ParallelMemoryConnBinding::~ParallelMemoryConnBinding()
{
}

void ParallelMemoryConnBinding::add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM)
{
   conn_binding::add_to_SM(HLSMgr, HLS, SM);
   const auto memory_banks_number = parameters->getOption<unsigned int>(OPT_memory_banks_number);
   const structural_objectRef circuit = SM->get_circ();
   for(const auto& component : GetPointer<ParallelMemoryFuBinding>(HLS->Rfu)->component_to_allow_mem_access)
   {
      const auto done = component.second->find_member("done", port_o_K, component.second);
      THROW_ASSERT(done, "");
      GetPointer<port_o>(done)->add_n_ports(memory_banks_number, done);
      const auto component_done = component.first->find_member(DONE_PORT_NAME, port_o_K, component.first);
      THROW_ASSERT(component_done, "");
      const auto component_done_signal = GetPointer<port_o>(component_done)->find_bounded_object(component.first->get_owner());
      THROW_ASSERT(component_done_signal, component_done->get_path());
      for(unsigned int memory_bank_index = 0; memory_bank_index < memory_banks_number; memory_bank_index++)
      {
         SM->add_connection(component_done_signal, GetPointer<port_o>(done)->get_port(memory_bank_index));
      }

      const auto op = component.second->find_member("op", port_o_K, component.second);
      GetPointer<port_o>(op)->add_n_ports(memory_banks_number, op);
      const auto access_allowed_component = component.first->find_member("access_allowed", port_o_K, component.first);
      THROW_ASSERT(access_allowed_component, "");
      const auto access_allowed_component_sign = SM->add_sign_vector("access_allowed_" + component.first->get_id(), memory_banks_number, circuit, op->get_typeRef());
      SM->add_connection(op, access_allowed_component_sign);
      SM->add_connection(access_allowed_component_sign, access_allowed_component);

      const auto start = component.first->find_member("start_port", port_o_K, component.first);
      THROW_ASSERT(start, "");
      const auto start_signal = GetPointer<port_o>(start)->find_bounded_object(component.first->get_owner());
      THROW_ASSERT(start_signal, "");
      const auto start_ama = component.second->find_member("start_port", port_o_K, component.second);
      THROW_ASSERT(start_ama, "");
      GetPointer<port_o>(start_ama)->add_n_ports(memory_banks_number, start_ama);
      for(unsigned int memory_bank_index = 0; memory_bank_index < memory_banks_number; memory_bank_index++)
      {
         SM->add_connection(GetPointer<port_o>(start_ama)->get_port(memory_bank_index), start_signal);
      }
   }
}
