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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file un_comparison_lowering.hpp
 * @brief Step that replace uneq_expr, ltgt_expr, unge_expr, ungt_expr, unle_expr and unlt_expr
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef UN_COMPARISON_LOWERING_HPP
#define UN_COMPARISON_LOWERING_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/**
 * Replace uneq_expr, ltgt_expr, unge_expr, ungt_expr, unle_expr and unlt_expr
 */
class UnComparisonLowering : public FunctionFrontendFlowStep
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
    * @param fun_id is the function index
    * @param design_flow_manager is the design flow manager
    * @param Param is the set of the parameters
    */
   UnComparisonLowering(const application_managerRef AppM, unsigned int fun_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~UnComparisonLowering() override;

   /**
    * Fixes the var_decl duplication.
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
