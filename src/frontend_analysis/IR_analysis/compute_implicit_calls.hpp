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
 * @file compute_implicit_calls.hpp
 * @brief Determine variables to be stored in memory
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$ $
 * Last modified by $Author$
 *
 */

#ifndef COMPUTE_IMPLICIT_CALLS_HPP
#define COMPUTE_IMPLICIT_CALLS_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// STL include
#include "custom_map.hpp"
#include <list>

/// Utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(BehavioralHelper);
REF_FORWARD_DECL(tree_manager);

class compute_implicit_calls : public FunctionFrontendFlowStep
{
 private:
   /// The tree manager
   tree_managerRef TM;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param parameters is the set of input parameters
    * @param AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param design_flow_manager is the design flow manager
    */
   compute_implicit_calls(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_idi, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~compute_implicit_calls() override;

   /**
    * Determines the variables that require a memory access
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
