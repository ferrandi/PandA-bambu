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
 * @file clb_model.hpp
 * @brief Specification of the class for representing FPGA area/resource models
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _CLB_MODEL_HPP_
#define _CLB_MODEL_HPP_

#include "area_model.hpp"

#include "custom_map.hpp"
#include <vector>

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

class clb_model : public area_model
{
 public:
   /// type of resources
   typedef enum
   {
      REGISTERS,
      SLICE,
      SLICE_LUTS,
      LUT_FF_PAIRS,
      ALMS,
      LOGIC_ELEMENTS,
      FUNCTIONAL_ELEMENTS,
      DSP,
      BRAM
   } value_t;

 protected:
   /// a double value representing the area of the component
   double area;

   /// resources required for the component
   std::map<value_t, double> used_resources;

 public:
   /**
    * Constructor
    */
   explicit clb_model(const ParameterConstRef& Param);

   /**
    * Constructor with specified area value
    */
   clb_model(const ParameterConstRef& Param, const double& _area_);

   /**
    * Destructor
    */
   ~clb_model() override;

   /**
    * Prints the used resources into an output stream
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
    * Sets the number of resources used for the specified type
    */
   void set_resource_value(value_t val, double num);

   /**
    * Returns true if the specified resource is used, false otherwise
    */
   bool is_used_resource(value_t val) const;

   /**
    * Returns the number of resources used for the given type
    */
   double get_resource_value(value_t val) const;
};

typedef refcount<clb_model> clb_modelRef;

#endif
