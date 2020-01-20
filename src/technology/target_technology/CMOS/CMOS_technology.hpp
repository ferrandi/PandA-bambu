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
 * @file CMOS_technology.hpp
 * @brief Class used to represent a target die for ASIC implementation
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 */

#ifndef _CMOS_TECHNOLOGY_HPP_
#define _CMOS_TECHNOLOGY_HPP_

#include "target_technology.hpp"
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(technology_manager);

class CMOS_technology : public target_technology
{
   /**
    * Initialize the target device based on the given parameters
    */
   void initialize() override;

 public:
   /**
    * Constructor of the class
    */
   explicit CMOS_technology(const ParameterConstRef& param);

   /**
    * Destructor of the class
    */
   ~CMOS_technology() override;

   /**
    * Returns the type of the technology currently implemented in a string format.
    * @return a string representing the type of the technology
    */
   std::string get_string_type() const override;
};

#endif
