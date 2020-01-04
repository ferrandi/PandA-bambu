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
 * @file storage_value_information.hpp
 * @brief This package is used to define the storage value scheme adopted by the register allocation algorithms.
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef STORAGE_VALUE_INFORMATION_HPP
#define STORAGE_VALUE_INFORMATION_HPP

/// graph include
#include "graph.hpp"

/// STD include
#include "custom_map.hpp"

/// utility include
#include "refcount.hpp"

class fu_binding;
CONSTREF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(OpGraph);

class StorageValueInformation
{
 protected:
   friend class values_scheme;

   /// current number of storage values
   unsigned int number_of_storage_values;

   /// put into relation variables/values with storage values
   CustomUnorderedMapUnstable<unsigned int, unsigned int> storage_index_map;

   /// put into relation storage value index with variables
   std::vector<unsigned int> variable_index_vect;

   /// relation between var written and operations
   CustomUnorderedMap<unsigned int, vertex> vw2vertex;

   /// The HLS manager
   const HLS_managerConstRef HLS_mgr;

   /// The index of the function
   const unsigned int function_id;

   /// operation graph used to compute the affinity between storage values
   OpGraphConstRef data;

   /// functional unit assignments
   Wrefcount<const fu_binding> fu;

 public:
   /**
    * Constructor
    */
   StorageValueInformation(const HLS_managerConstRef HLS_mgr, const unsigned int function_id);

   /**
    * Destructor
    */
   ~StorageValueInformation();

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
    */
   bool is_a_storage_value(vertex curr_vertex, unsigned int var_index) const;

   /**
    * Returns the index of the storage value associated with the variable in a given vertex
    * @param curr_vertex is the vertex
    * @param var_index is the variable
    */
   unsigned int get_storage_value_index(vertex curr_vertex, unsigned int var_index) const;

   /**
    * Returns the index of the variable associated with the storage value in a given vertex
    */
   unsigned int get_variable_index(unsigned int storage_value_index) const;

   /**
    * return a weight that estimate how much two storage values are compatible.
    * An high value returned means an high compatibility between the two storage values.
    */
   int get_compatibility_weight(unsigned int storage_value_index1, unsigned int storage_value_index2) const;

   /**
    * return the in case the storage values have compatible size
    * @param storage_value_index1 is the first storage value
    * @param storage_value_index2 is the second storage value
    */
   bool are_value_bitsize_compatible(unsigned int storage_value_index1, unsigned int storage_value_index2) const;
};
typedef refcount<StorageValueInformation> StorageValueInformationRef;
#endif
