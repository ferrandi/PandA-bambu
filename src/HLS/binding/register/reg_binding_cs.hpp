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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file reg_binding.hpp
 * @brief add register file and add selector to their input
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 *
 */

#ifndef REG_BINDING_CS_H
#define REG_BINDING_CS_H

#include "reg_binding.hpp"

class reg_binding_cs : public reg_binding
{
 public:
   /**
    * Constructor.
    */
   reg_binding_cs(const hlsRef& HLS, const HLS_managerRef HLSMgr_);

   /**
    * Destructor.
    */
   virtual ~reg_binding_cs();

 protected:
   /**
    * @brief CalculateRegisterName
    * @param i
    * @return
    */
   std::string CalculateRegisterName(unsigned int i);

   /**
    * @brief specialise_reg add dimension selector
    * @param reg
    * @param r
    */
   void specialise_reg(structural_objectRef& reg, unsigned int r);
};

#endif // REG_BINDING_CS_H
