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
 * @file storage_value_information_fsm.cpp
 * @brief This package is used to define the storage value scheme adopted by the register allocation algorithms.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// Header include
#include "storage_value_information_fsm.hpp"

#include "hls_manager.hpp"
#include "tree_helper.hpp"

StorageValueInformationFsm::StorageValueInformationFsm(const HLS_managerConstRef _HLS_mgr,
                                                       const unsigned int _function_id)
    : StorageValueInformation(_HLS_mgr, _function_id)
{
}

StorageValueInformationFsm::~StorageValueInformationFsm() = default;

bool StorageValueInformationFsm::is_a_storage_value(vertex, unsigned int var_index, unsigned int stage)
{
   return storage_index_map.find(std::make_pair(var_index, stage)) != storage_index_map.end();
}

unsigned int StorageValueInformationFsm::get_storage_value_index(vertex, unsigned int var_index, unsigned int stage)
{
   THROW_ASSERT(storage_index_map.find(std::make_pair(var_index, stage)) != storage_index_map.end(),
                "the storage value is missing");
   return storage_index_map.find(std::make_pair(var_index, stage))->second;
}

void StorageValueInformationFsm::set_storage_value_index(vertex, unsigned int variable, unsigned int stage,
                                                         unsigned int sv)
{
   storage_index_map[std::make_pair(variable, stage)] = sv;
}

bool StorageValueInformationFsm::are_storage_value_compatible(unsigned int storage_value_index1,
                                                              unsigned int storage_value_index2) const
{
   THROW_ASSERT(storage_value_index1 != storage_value_index2, "unexpected condition");
   if(!StorageValueInformation::are_value_bitsize_compatible(storage_value_index1, storage_value_index2))
   {
      return false;
   }
   const auto var_stage1 = get_variable_index(storage_value_index1);
   const auto var_stage2 = get_variable_index(storage_value_index2);
   const tree_managerRef TreeM = HLS_mgr->get_tree_manager();
   const auto is_par1 = tree_helper::is_parameter(TreeM, var_stage1.first);
   const auto is_par2 = tree_helper::is_parameter(TreeM, var_stage2.first);
   if((is_par1 && var_stage1.second == 0) || (is_par2 && var_stage2.second == 0))
   {
      return false;
   }
   else
   {
      return true;
   }
}
