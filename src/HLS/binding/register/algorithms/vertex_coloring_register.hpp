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
 * @file vertex_coloring_register.hpp
 * @brief Class specification of a coloring based register allocation algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef VERTEX_COLORING_REGISTER_HPP
#define VERTEX_COLORING_REGISTER_HPP

#include "conflict_based_register.hpp"

class vertex_coloring_register : public conflict_based_register
{
 private:
   DesignFlowStep_Status RegisterBinding() final;

 public:
   /**
    * Constructor of the class.
    * @param Param is the parameter set
    * @param HLSMgr is the HLS manager
    * @param funId is the function identifier
    * @param design_flow_manager is the design flow manager
    */
   vertex_coloring_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                            const DesignFlowManager& design_flow_manager);
};

#endif
