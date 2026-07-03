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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file chordal_coloring_register.hpp
 * @brief Class specification of the register allocation algorithms based on chordal algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
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
 private:
   /// compare lexically two vectors
   bool lex_compare_gt(const std::vector<unsigned int>& v1, const std::vector<unsigned int>& v2) const;

   /**
    * Chordal coloring algorithm algorithm.
    * Stores the output registers in result_regs and the input storage values in regs.
    * Stores in result_map the relations between them.
    * Then it updates high-level synthesis results
    * All previous result are erased.
    * @return the exit status of this step
    */
   DesignFlowStep_Status RegisterBinding() final;

 public:
   /**
    * Constructor of the class.
    * @param Param is the set of input parameters
    * @param HLSMgr is the HLS manager
    * @param funId is the identifier of the function being processed
    * @param design_flow_manager is the design flow manager
    */
   chordal_coloring_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                             const DesignFlowManager& design_flow_manager);

   /**
    * Destructor of the class.
    */
};

#endif
