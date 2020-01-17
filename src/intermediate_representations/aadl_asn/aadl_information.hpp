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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file  aadl_information.hpp
 * @brief The information collected from aadl file
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef AADL_INFORMATION_HPP
#define AADL_INFORMATION_HPP

/// STD include
#include <string>

/// STL include
#include <list>

/// utility includes
#include "custom_map.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(AsnType);

class AadlInformation
{
 protected:
   /// The asn types
   CustomMap<std::string, AsnTypeRef> asn_types;

   /**
    * Normalize asn_type name
    * @param name is the name
    * @return the normalized name
    */
   std::string Normalize(const std::string& name) const;

 public:
   /// Strucutre containing the characteristics of a parameter
   struct AadlParameter
   {
      typedef enum
      {
         BIG_ENDIANESS,
         LITTLE_ENDIANESS,
         NATIVE_ENDIANESS
      } EndianessType;

      /// The name of the parameter;
      std::string name;

      /// The type of the parameter
      std::string asn_type;

      /// The endianess
      EndianessType endianess;

      typedef enum
      {
         IN,
         OUT,
         INOUT
      } Direction;

      /// The direction of the parameter
      Direction direction;

      /// The number of bambu registers to be allocated
      unsigned int num_registers;

      /// The bambu address
      unsigned int bambu_address;

      /// True if the parameter is a pointer
      bool pointer;

      /**
       * Constructor
       */
      AadlParameter();

      /**
       * Return the endianess
       */
      static EndianessType Endianess(const std::string& endianess_string);

      /**
       * Return the direction
       */
      static Direction GetDirection(const std::string& direction_string);
   };

   /// For each function the list of parameters
   std::map<std::string, std::list<AadlParameter>> function_parameters;

   /// The top functions in the order in which they are specified in the aadl file
   std::list<std::string> top_functions_names;

   /// For each function the size of internal memory
   CustomMap<std::string, unsigned int> internal_memory_sizes;

   /// For each function the exposed size of internal memory
   CustomMap<std::string, unsigned int> exposed_memory_sizes;

   /**
    * Add an asn_type
    * @param name is the name of the type
    * @param type is the type
    */
   void AddAsnType(const std::string& name, const AsnTypeRef& asn_type);

   /**
    * Get an asn_type
    * @param name is the name of the type
    */
   AsnTypeRef CGetAsnType(const std::string& name) const;
};
typedef refcount<AadlInformation> AadlInformationRef;
typedef refcount<const AadlInformation> AadlInformationConstRef;
#endif
