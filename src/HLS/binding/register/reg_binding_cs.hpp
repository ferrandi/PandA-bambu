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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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

    std::string CalculateRegisterName(unsigned int i);

    /**
    * Add to the resulting registers file the clock and selector
    */
    void add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port);

    /**
     * @brief function kernel need to instantiate scheduler before connect selector port of register file
     * @param selector_regFile_sign
     */
    void add_register_file_kernel(structural_objectRef selector_regFile_sign);

protected:
    /**
     * @brief function inside kernel need to connect selector
     */
    void add_register_file_function();
};

#endif // REG_BINDING_CS_H
