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
 *              Copyright (C) 2023 Politecnico di Milano
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
 * @file area_info.hpp
 * @brief Collect information about resource area
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef AREA_INFO_HPP
#define AREA_INFO_HPP

#include "refcount.hpp"
REF_FORWARD_DECL(area_info);
CONSTREF_FORWARD_DECL(Parameter);
class xml_element;

#include <map>
#include <string>

class area_info
{
 public:
   /// type of resources
   using value_t = enum {
      REGISTERS,
      SLICE,
      SLICE_LUTS,
      LUT_FF_PAIRS,
      ALMS,
      LOGIC_ELEMENTS,
      FUNCTIONAL_ELEMENTS,
      LOGIC_AREA,
      DSP,
      BRAM,
      DRAM,
      POWER,
      URAM
   };

 private:
   /// a double value representing the area of the component
   double area;

   /// resources required for the component
   std::map<value_t, double> used_resources;

   /// class containing all the parameters
   const ParameterConstRef Param;

 public:
   /**
    * @name Constructors and Destructors.
    */
   //@{
   /// Constructor.
   explicit area_info(const ParameterConstRef& _Param);

   /// Destructor.
   ~area_info();
   //@}

   /**
    * Print method.
    */
   void print(std::ostream& os) const;

   /**
    * Factory method.
    */
   static area_infoRef factory(const ParameterConstRef& Param);

   /**
    * Set the nominal value for the area of the component
    */
   void set_area_value(const double& _area_);

   /**
    * Return the nominal value for the area of the component
    */
   double get_area_value() const;

   void set_resource_value(value_t val, double num);
   bool is_used_resource(value_t val) const;
   double get_resource_value(value_t val) const;

   /// default area value
   static const double area_DEFAULT;
};
/// refcount definition of the class
using area_infoRef = refcount<area_info>;

#endif
