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
 *              Copyright (C) 2017-2020 Politecnico di Milano
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
 * @file dry_runevaluation.hpp
 * @brief Class to generate an empty evaluation report
 *
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef DRY_RUN_EVALUATION_HPP
#define DRY_RUN_EVALUATION_HPP

/// base class include
#include "evaluation_base_step.hpp"

class DryRunEvaluation : public EvaluationBaseStep
{
 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of evaluation
    */
   DryRunEvaluation(const ParameterConstRef Param, const HLS_managerRef HLSMgr, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~DryRunEvaluation() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
