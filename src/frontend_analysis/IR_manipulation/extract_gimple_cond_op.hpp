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
 * @file extract_gimple_cond_op.hpp
 * @brief Analysis step that extract condition from gimple_cond
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef EXTRACT_GIMPLE_COND_OP_HPP
#define EXTRACT_GIMPLE_COND_OP_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// Utility include
#include "refcount.hpp"

/**
 * Extract cond from gimple_cond
 */
class ExtractGimpleCondOp : public FunctionFrontendFlowStep
{
 private:
   /// flag used to restart code motion step
   bool bb_modified;

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
    * @param function_id is the identifier of the function
    * @param parameters is the set of input parameters
    */
   ExtractGimpleCondOp(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager, const unsigned int function_id, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~ExtractGimpleCondOp() override;

   /**
    * Updates the tree to have a more compliant CFG
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
