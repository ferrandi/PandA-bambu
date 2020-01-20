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
 * @file liberty_model.hpp
 * @brief Class specification for the timing model based on liberty information
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _LIBERTY_MODEL_HPP_
#define _LIBERTY_MODEL_HPP_

#include "time_model.hpp"
REF_FORWARD_DECL(timing_group);

#include "custom_map.hpp"
#include "custom_set.hpp"

class liberty_model : public time_model
{
   double drive_strength;

   double skew;

   std::map<std::string, std::map<CustomOrderedSet<std::string>, timing_groupRef>> timing_groups;

 public:
   /**
    * @name Constructors and destructors.
    */
   //@{
   /**
    * Constructor
    */
   explicit liberty_model(const ParameterConstRef Param);

   /**
    * Destructor
    */
   ~liberty_model() override;
   //@}

   void xwrite(xml_element* pin_node, const std::string& output_pin) override;

   void add_timing_group(const std::string& output, const CustomOrderedSet<std::string>& inputs, const timing_groupRef& tg);

   std::map<std::string, std::map<CustomOrderedSet<std::string>, timing_groupRef>> get_timing_groups() const
   {
      return timing_groups;
   }

   void set_timing_groups(const std::map<std::string, std::map<CustomOrderedSet<std::string>, timing_groupRef>>& timing_groups_);

   bool has_timing_groups() const
   {
      return (timing_groups.size() > 0);
   }

   void set_drive_strength(double value);

   double get_drive_strength() const;

   void set_skew_value(double value);

   double get_skew_value() const;
};

typedef refcount<liberty_model> liberty_modelRef;

#endif
