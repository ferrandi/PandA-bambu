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
 * @file evaluation_base_step.hpp
 * @brief Base class to compute evaluations about high-level synthesis
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Superclass include
#include "hls_function_step.hpp"

#ifndef EVALUATION_BASE_STEP_HPP
#define EVALUATION_BASE_STEP_HPP

/**
 * This class is the base class of evaluation steps
 */
class EvaluationBaseStep : public HLSFunctionStep
{
 protected:
   /// List of steps performing evaluations
   std::vector<DesignFlowStepRef> cost_functions;

   /// List of objectives that have to be evaluated
   std::vector<HLSFlowStep_Type> cost_function_list;

   /// store the result of the evaluation
   std::vector<double> evaluations;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    * @HLSMgr is the HLS manager
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type the particular type of evaluation
    */
   EvaluationBaseStep(const ParameterConstRef parameters, const HLS_managerRef HLSMgr, const unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const HLSFlowStep_Type hls_flow_step_type);

   /**
    * Destructor
    */
   ~EvaluationBaseStep() override;

   /**
    * Returns the results of the evaluation
    */
   const std::vector<double>& GetEvaluations() const;
};
#endif
