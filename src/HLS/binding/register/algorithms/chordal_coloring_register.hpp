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
 * @file chordal_coloring_register.hpp
 * @brief Class specification of the register allocation algorithms based on chordal algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CHORDAL_COLORING_REGISTER_HPP
#define CHORDAL_COLORING_REGISTER_HPP

#include "conflict_based_register.hpp"
#include <vector>
/**
 * Class containing the chordal coloring algorithm implementation
 */
class chordal_coloring_register : public conflict_based_register
{
 public:
   /**
    * Constructor of the class.
    * @param design_flow_manager is the design flow manager
    */
   chordal_coloring_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor of the class.
    */
   ~chordal_coloring_register() override;

   /**
    * Chordal coloring algorithm algorithm.
    * Stores the output registers in result_regs and the input storage values in regs.
    * Stores in result_map the relations between them.
    * Then it updates high-level synthesis results
    * All previous result are erased.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

 private:
   /// compare lexically two vectors
   bool lex_compare_gt(const std::vector<unsigned int>& v1, const std::vector<unsigned int>& v2) const;
};

#endif
