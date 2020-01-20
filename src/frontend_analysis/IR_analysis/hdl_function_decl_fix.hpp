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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file hdl_function_decl_fix.hpp
 * @brief Pre-analysis step fixing names of functions which clash with signal names
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef HDL_FUNCTION_DECL_FIX_HPP
#define HDL_FUNCTION_DECL_FIX_HPP

/// Superclass include
#include "application_frontend_flow_step.hpp"

/**
 * Frontend flow step which changes names of functions with clash with names of auxiliary signals
 */
class HDLFunctionDeclFix : public ApplicationFrontendFlowStep
{
 protected:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   HDLFunctionDeclFix(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~HDLFunctionDeclFix() override;

   /**
    * Fixes the var_decl duplication.
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
