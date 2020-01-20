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
 * @file parallel_memory_fu_binding.hpp
 * @brief Data structure used to store the functional-unit binding of the vertices.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef PARALLEL_MEMORY_FU_BINDING_HPP
#define PARALLEL_MEMORY_FU_BINDING_HPP

/// Superclass include
#include "fu_binding.hpp"

/// utility include
#include "custom_map.hpp"
#include "custom_set.hpp"

class ParallelMemoryFuBinding : public fu_binding
{
 protected:
   /// Internal objects for which access_allowed was killed
   CustomSet<structural_objectRef> access_allowed_killeds;

   virtual bool manage_module_ports(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM, const structural_objectRef curr_gate, unsigned int num);

 public:
   /**
    * For each component, the corresponding ALLOW_MEM_ACCESS_FU
    */
   CustomMap<structural_objectRef, structural_objectRef> component_to_allow_mem_access;

   /**
    * Constructor.
    * @param HLS_mgr is the HLS manager
    * @param function_id is the index of the function
    * @param parameters is the set of input parameters
    */
   ParallelMemoryFuBinding(const HLS_managerConstRef HLS_mgr, const unsigned int function_id, const ParameterConstRef parameters);

   /**
    * Instance the functional unit inside the structural representation of the datapath
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port);
};
#endif
