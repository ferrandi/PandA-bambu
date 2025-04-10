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
 * @file storage_value_information.hpp
 * @brief This package is used to define the storage value scheme adopted when register replication for pipelining is
 * required.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef STORAGE_VALUE_INFORMATION_PIPELINE_HPP
#define STORAGE_VALUE_INFORMATION_PIPELINE_HPP

#include "storage_value_information.hpp"

class StorageValueInformationPipeline : public StorageValueInformation
{
 private:
   /// put into relation variables/values with storage values
   CustomUnorderedMap<std::pair<vertex, unsigned int>, unsigned int> storage_index_double_map;

 public:
   /**
    * Constructor
    */
   StorageValueInformationPipeline(const HLS_managerConstRef HLS_mgr, const unsigned int function_id);
   /**
    * Destructor
    */
   ~StorageValueInformationPipeline() override;

   /**
    * return true in case a storage value exist for the pair vertex variable
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    */
   bool is_a_storage_value(vertex state, unsigned int var_index) override;

   /**
    * Returns the index of the storage value associated with the variable in a given vertex
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    */
   unsigned int get_storage_value_index(vertex state, unsigned int var_index) override;

   /**
    * assign a storage value to a couple state-variable
    * @param curr_state is the current state
    * @param variable is the assigned variable
    * @param sv is the assigned storage value*/
   void set_storage_value_index(vertex curr_state, unsigned int variable, unsigned int sv) override;
};
using StorageValueInformationPipelineRef = refcount<StorageValueInformationPipeline>;
#endif
