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
 * @file evaluation.hpp
 * @brief Class to compute evaluations about high-level synthesis
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef EVALUATION_HPP
#define EVALUATION_HPP

#include "hls_step.hpp"

#include <vector>

enum class Evaluation_Mode
{
   NONE,
   DRY_RUN,
   EXACT,
};

/**
 * @class evaluation
 * @brief Class definition for high level synthesis result evaluation
 * @ingroup Estimations
 *
 * This class is the abstract class for all kind of result evaluations. Different kinds of evaluation can be performed.
 * Real evaluations or lower-bound/upper-bound evaluations can be provided.
 */
class Evaluation
#if(__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
    final
#endif
    : public HLS_step
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_type is the type of evaluation
    */
   Evaluation(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr,
              const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~Evaluation() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override
#if(__GNUC__ == 4 && __GNUC_MINOR__ >= 7)
       final
#endif
       ;
};
#endif
