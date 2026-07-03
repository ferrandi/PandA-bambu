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
   StorageValueInformationPipeline(const HLS_managerConstRef HLS_mgr, const unsigned int function_id);

   /**
    * return true in case a storage value exist for the pair vertex variable
    * @param state is the vertex
    * @param var_index is the variable
    */
   bool is_a_storage_value(vertex state, unsigned int var_index) override;

   /**
    * Returns the index of the storage value associated with the variable in a given vertex
    * @param state is the vertex
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
