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
 * @file NI_SSA_liveness.hpp
 * @brief Non-Iterative liveness analysis for SSA based gimple descriptions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef NI_SSA_LIVENESS_HPP
#define NI_SSA_LIVENESS_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(NI_SSA_liveness);
REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(Parameter);
class statement_list;
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(tree_node);
//@}

/**
 *
 */
class NI_SSA_liveness : public FunctionFrontendFlowStep
{
 private:
   /// Algorithm 5: Explore all paths from a variable’s use to its definition.
   void Up_and_Mark(blocRef B, tree_nodeRef v, statement_list* sl);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the index of the function
    * @param design_flow_manager is the design flow manager
    */
   NI_SSA_liveness(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~NI_SSA_liveness() override;

   /**
    * Performes the liveness analysis.
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
