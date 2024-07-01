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
 * @file FunctionCallTypeCleanup.hpp
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef FUNCTION_CALL_TYPE_CLEANUP_HPP
#define FUNCTION_CALL_TYPE_CLEANUP_HPP

#include "function_frontend_flow_step.hpp"

#include <string>
#include <vector>

REF_FORWARD_DECL(application_manager);
REF_FORWARD_DECL(bloc);
CONSTREF_FORWARD_DECL(DesignFlowManager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);

class FunctionCallTypeCleanup : public FunctionFrontendFlowStep
{
 private:
   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * @brief
    *
    * @param TM is the tree manager
    * @param tree_man is the tree manipulation
    * @param block is the call statement basic block
    * @param stmt is the call statement
    * @param args is the reference of the args vector from the call statement
    * @param srcp is the default srcp
    * @return true when parameters have been modified
    * @return false when no modification is applied
    */
   bool ParametersTypeCleanup(const tree_managerRef& TM, const tree_manipulationRef& tree_man, const blocRef& block,
                              const tree_nodeRef& stmt, std::vector<tree_nodeRef>& args, const std::string& srcp) const;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   FunctionCallTypeCleanup(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id,
                           const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~FunctionCallTypeCleanup() override;

   /**
    * Computes the operations CFG graph data structure.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
