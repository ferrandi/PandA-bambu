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
 * @file FixStructsPassedByValue.hpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

#ifndef FIX_STRUCTS_PASSED_BY_VALUE_HPP
#define FIX_STRUCTS_PASSED_BY_VALUE_HPP

#include "function_frontend_flow_step.hpp"

class FixStructsPassedByValue : public FunctionFrontendFlowStep
{
 protected:
   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    */
   FixStructsPassedByValue(const ParameterConstRef params, const application_managerRef AM, unsigned int fun_id,
                           const DesignFlowManagerConstRef dfm);

   /**
    * Destructor
    */
   ~FixStructsPassedByValue() override;
};

#endif
