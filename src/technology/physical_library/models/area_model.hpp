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
 * @file area_model.hpp
 * @brief Abstract class representing a generic area/resource model
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _AREA_MODEL_HPP_
#define _AREA_MODEL_HPP_

#include "refcount.hpp"
/// refcount definition for the class
REF_FORWARD_DECL(area_model);
CONSTREF_FORWARD_DECL(Parameter);
class xml_element;
enum class TargetDevice_Type;

#include <string>

class area_model
{
 public:
   typedef enum
   {
      PRE_TECHNOLOGY_MAPPING,
      POST_TECHNOLOGY_MAPPING
   } chacterization_t;

   typedef enum
   {
      COMBINATIONAL_AREA,
      NONCOMBINATIONAL_AREA,
      CELL_AREA,
      INTERCONNECT_AREA,
      TOTAL_AREA
   } element_t;

 protected:
   /// class containing all the parameters
   const ParameterConstRef Param;

 public:
   /**
    * @name Constructors and destructors.
    */
   //@{
   /// Constructor.
   explicit area_model(const ParameterConstRef& _Param_);

   /// Destructor.
   virtual ~area_model();
   //@}

   /**
    * Print method.
    */
   virtual void print(std::ostream& os) const = 0;

   /**
    * Factory method.
    */
   static area_modelRef create_model(const TargetDevice_Type type, const ParameterConstRef& Param);

   /**
    * Set the nominal value for the area of the component
    */
   virtual void set_area_value(const double& _area_) = 0;

   /**
    * Return the nominal value for the area of the component
    */
   virtual double get_area_value() const = 0;

   /**
    * Checks if there is a characterization value
    */
   virtual bool is_characterization(unsigned int characterization_type) const = 0;

   /// default area value
   static const double area_DEFAULT;
};
/// refcount definition of the class
typedef refcount<area_model> area_modelRef;

#endif
