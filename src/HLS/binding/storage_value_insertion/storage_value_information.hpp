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
 * @brief Storage value information: variable are described by a pair: the variable id and the stage in which the
 * variable is used
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef STORAGE_VALUE_INFORMATION_HPP
#define STORAGE_VALUE_INFORMATION_HPP

#include "HLS/fsm/FSMInfo.hpp"
#include "custom_map.hpp"
#include "op_graph.hpp"
#include "refcount.hpp"

class fu_binding;
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(fu_binding);

struct VarInfo
{
   unsigned long long size;
   bool is_parameter;
   bool is_a_phi;
   bool isInt;
   bool isReal;
   OpGraph::vertex_descriptor op_vertex;
   unsigned fu_unit;
   unsigned fu_unit_index;
   VarInfo(unsigned long long _size, bool _is_parameter, bool _is_a_phi, bool _isInt, bool _isReal,
           OpGraph::vertex_descriptor _op_vertex, unsigned _fu_unit, unsigned _fu_unit_index)
       : size(_size),
         is_parameter(_is_parameter),
         is_a_phi(_is_a_phi),
         isInt(_isInt),
         isReal(_isReal),
         op_vertex(_op_vertex),
         fu_unit(_fu_unit),
         fu_unit_index(_fu_unit_index)
   {
   }
};

class StorageValueInformation
{
 protected:
   /// current number of storage values
   unsigned int number_of_storage_values;

   /// put into relation storage value index with variables
   CustomUnorderedMap<unsigned int, std::pair<unsigned int, unsigned int>> sv2variable;

   /// reverse map of sv2variable
   CustomUnorderedMap<std::pair<unsigned int, unsigned int>, unsigned int> variable2sv;

   /// store precomputed info associated with a variable
   CustomUnorderedMap<unsigned int, VarInfo> vw2info;

   /// The HLS manager
   const HLS_managerConstRef HLS_mgr;

   /// The index of the function
   const unsigned int function_id;

   /// operation graph used to compute the affinity between storage values
   OpGraph data;

   /// functional unit assignments
   const fu_binding* fu;

 public:
   StorageValueInformation(const HLS_managerConstRef HLS_mgr, const unsigned int function_id);

   ~StorageValueInformation() = default;

   /**
    * Initialize the step (i.e., like a constructor)
    */
   void Initialize();

   /**
    * Returns the number of storage values inserted
    */
   unsigned int get_number_of_storage_values() const;

   /**
    * return true in case a storage value exist for the pair vertex variable
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    * @param stage is the stage in case of pipelined state
    */
   bool is_a_storage_value(FSMInfo::state_descriptor curr_vertex, unsigned int var_index, unsigned int stage);

   /**
    * Returns the index of the storage value associated with the variable in a given vertex
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    * @param stage is the stage in case of pipelined state
    */
   unsigned int get_storage_value_index(FSMInfo::state_descriptor curr_vertex, unsigned int var_index,
                                        unsigned int stage);

   /**
    * Returns the index of the variable associated with the storage value in a given vertex
    */
   std::pair<unsigned int, unsigned int> get_variable(unsigned int storage_value_index) const;

   /**
    * return a weight that estimate how much two storage values are compatible.
    * An high value returned means an high compatibility between the two storage values.
    */
   int get_compatibility_weight(unsigned int storage_value_index1, unsigned int storage_value_index2) const;

   /**
    * return the maximum weight the get_compatibility_weight may return.
    * High values means high compatibility
    */
   int get_max_weight() const;

   /**
    * assign a storage value to a couple state-variable
    * @param curr_state is the current state
    * @param variable is the assigned variable
    * @param stage is the stage in case of pipelined state
    * @param sv is the assigned storage value*/
   void set_storage_value_index(FSMInfo::state_descriptor curr_state, unsigned int variable, unsigned int stage,
                                unsigned int sv);

   /**
    * return true in case the storage values are compatible
    * @param storage_value_index1 is the first storage value
    * @param storage_value_index2 is the second storage value
    */
   bool are_storage_value_compatible(unsigned int storage_value_index1, unsigned int storage_value_index2) const;
};
#endif
