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
 * @file create_tree_manager.hpp
 * @brief Class that creates the tree_manager starting from the source code files
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _CREATE_TREE_MANAGER_HPP_
#define _CREATE_TREE_MANAGER_HPP_

/// Superclass include
#include "application_frontend_flow_step.hpp"

/// Utility include
#include "refcount.hpp"
REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(GccWrapper);

/**
 * Class that creates the tree_manager starting from the source code files
 */
class create_tree_manager : public ApplicationFrontendFlowStep
{
 private:
   /// The gcc wrapper
   const GccWrapperRef gcc_wrapper;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor
    * @param Param is the set of the parameters
    * @param AppM is the reference to the application manager
    * @param design_flow_manager is the design flow manager
    */
   create_tree_manager(const ParameterConstRef Param, const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~create_tree_manager() override;

   /**
    * Creates the tree_manager data structure by invoking the GCC/CLANG wrapper
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
