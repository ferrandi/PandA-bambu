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
 * @file storage_value_information_fsm.hpp
 * @brief This package is used to define the storage value scheme adopted by the register allocation algorithms.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef STORAGE_VALUE_INFORMATION_FSM_HPP
#define STORAGE_VALUE_INFORMATION_FSM_HPP

#include "storage_value_information.hpp"
CONSTREF_FORWARD_DECL(StateTransitionGraph);

class StorageValueInformationFsm : public StorageValueInformation
{
 protected:
   /// put into relation variables/values plus stage with storage values
   CustomUnorderedMap<std::pair<unsigned int, unsigned int>, unsigned int> storage_index_map;

 public:
   /**
    * Constructor
    */
   StorageValueInformationFsm(const HLS_managerConstRef HLS_mgr, const unsigned int function_id);

   /**
    * Destructor
    */
   ~StorageValueInformationFsm() override;

   /**
    * return true in case a storage value exist for the pair vertex variable
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    * @param stage is the stage in case of pipelined state
    */
   bool is_a_storage_value(vertex curr_vertex, unsigned int var_index, unsigned int stage) override;

   /**
    * Returns the index of the storage value associated with the variable in a given vertex
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    * @param stage is the stage in case of pipelined state
    */
   unsigned int get_storage_value_index(vertex curr_vertex, unsigned int var_index, unsigned int stage) override;

   /**
    * assign a strage value to a couple state-variable
    * @param curr_state is the current state
    * @param variable is the assigned variable
    * @param stage is the stage in case of pipelined state
    * @param sv is the assigned storage value*/
   void set_storage_value_index(vertex curr_state, unsigned int variable, unsigned int stage, unsigned int sv) override;

   bool are_storage_value_compatible(unsigned int storage_value_index1,
                                     unsigned int storage_value_index2) const override;
};

using StorageValueInformationFsmRef = refcount<StorageValueInformationFsm>;
#endif
