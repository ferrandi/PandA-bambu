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
 * @file cell_model.hpp
 * @brief Specification of the class to represent area/resource models for integrated circuits
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _CELL_MODEL_HPP_
#define _CELL_MODEL_HPP_

#include "area_model.hpp"

#include "custom_map.hpp"
#include <vector>

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(target_device);
CONSTREF_FORWARD_DECL(technology_manager);

class cell_model : public area_model
{
 protected:
   /// a double value representing the area of the component
   double area;

   /// values of the available characterizations
   std::map<unsigned int, std::map<unsigned int, double>> characterization;

   /**
    * Determines the shape of the component, given the available information
    */
   void determine_shape();

 public:
   /**
    * Constructor
    */
   explicit cell_model(const ParameterConstRef& Param);

   /**
    * Constructor with initial area value
    */
   cell_model(const ParameterConstRef& Param, const double& _area_);

   /**
    * Destructor
    */
   ~cell_model() override;

   /**
    * Print method
    */
   void print(std::ostream& os) const override;

   /**
    * Sets the nominal value for the area of the component
    */
   void set_area_value(const double& _area_) override;

   /**
    * Returns the nominal value for the area of the component
    */
   double get_area_value() const override;

   /**
    * Checks if there is a characterization for the given type
    */
   bool is_characterization(unsigned int characterization_type) const override;

   /**
    * Checks if the given element has a characterization
    */
   bool has_element_characterization(unsigned int characterization_type, unsigned int element_type) const;

   /**
    * Returns the characterization for the given element
    */
   double get_characterization(unsigned int characterization_type, unsigned int element_type) const;

   /**
    * Sets the value of the characterization for the given element
    */
   void set_characterization(unsigned int characterization_type, unsigned int element_type, double value);
};
/// refcount definition of the class
typedef refcount<cell_model> cell_modelRef;

#endif
