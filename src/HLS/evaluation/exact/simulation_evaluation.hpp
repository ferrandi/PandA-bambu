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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file simulation_evaluation.hpp
 * @brief .
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef SIMULATION_EVALUATION_HPP
#define SIMULATION_EVALUATION_HPP

/// superclass include
#include "evaluation_base_step.hpp"

/// utility include
#include "refcount.hpp"

/**
 * @class SynthesisEvaluation
 * Class performing the actual simulation
 */
class SimulationEvaluation : public EvaluationBaseStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   bool already_executed;

 public:
   /**
    * Constructor of the class
    */
   SimulationEvaluation(const ParameterConstRef Param, const HLS_managerRef hls_mgr,
                        const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor of the class
    */
   ~SimulationEvaluation() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
