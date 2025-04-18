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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file storage_value_information_fsm.cpp
 * @brief This package is used to define the storage value scheme adopted by the register allocation algorithms.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "storage_value_information_fsm.hpp"

StorageValueInformationFsm::StorageValueInformationFsm(const HLS_managerConstRef _HLS_mgr,
                                                       const unsigned int _function_id)
    : StorageValueInformation(_HLS_mgr, _function_id)
{
}

StorageValueInformationFsm::~StorageValueInformationFsm() = default;

bool StorageValueInformationFsm::is_a_storage_value(vertex, unsigned int var_index)
{
   return storage_index_map.find(var_index) != storage_index_map.end();
}

unsigned int StorageValueInformationFsm::get_storage_value_index(vertex, unsigned int var_index)
{
   THROW_ASSERT(storage_index_map.find(var_index) != storage_index_map.end(), "the storage value is missing");
   return storage_index_map.find(var_index)->second;
}

void StorageValueInformationFsm::set_storage_value_index(vertex, unsigned int variable, unsigned int sv)
{
   storage_index_map[variable] = sv;
}
