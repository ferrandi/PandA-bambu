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
 * @file LUT_model.hpp
 * @brief Class specification for the timing model based on LUTs
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _LUT_MODEL_HPP_
#define _LUT_MODEL_HPP_

#include "time_model.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(technology_manager);

class LUT_model : public time_model
{
 public:
   typedef enum
   {
      COMBINATIONAL_DELAY,
      MINIMUM_PERIOD_POST_MAP,
      MINIMUM_PERIOD_POST_PAR
   } value_t;

 protected:
   /// timing characterization
   std::map<value_t, double> timing_results;

 public:
   /**
    * @name Constructors and destructors.
    */
   //@{
   /**
    * Constructor
    */
   explicit LUT_model(const ParameterConstRef Param);

   /**
    * Destructor
    */
   ~LUT_model() override;
   //@}

   void xwrite(xml_element* pin_node, const std::string& output_pin) override;

   /**
    * Sets the timing information for the specified type
    */
   void set_timing_value(value_t val, double num);

   /**
    * Returns true if the specified timing value has been set, false otherwise
    */
   bool is_timing_value(value_t val) const;

   /**
    * Returns the timing information for the given type
    */
   double get_timing_value(value_t val) const;
};

typedef refcount<LUT_model> LUT_modelRef;

#endif
