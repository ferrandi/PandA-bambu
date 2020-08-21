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
 * @file var_decl_fix.hpp
 * @brief Pre-analysis step fixing var_decl duplication.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VAR_DECL_FIX_HPP
#define VAR_DECL_FIX_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// Utility include
#include "custom_set.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
//@}

/**
 * Pre-analysis step. It transforms the raw intermediate representation removing
 * var_decl duplication: two var_decls with the same variable name in the same function.
 */
class VarDeclFix : public FunctionFrontendFlowStep
{
 protected:
   /**
    * Return the normalized identifier; in this class it is the identifier itself. Subclasses can specialize it
    * @param identifier is the identifier to be normalized
    * @return the normalized identifier
    */
   virtual const std::string Normalize(const std::string& identifier) const;

   /**
    * Recursive examinate tree node
    */
   void recursive_examinate(const tree_nodeRef& tn, CustomUnorderedSet<unsigned int>& already_examinated_decls, CustomUnorderedSet<std::string>& already_examinated_names, CustomUnorderedSet<std::string>& already_examinated_type_names,
                            CustomUnorderedSet<unsigned int>& already_visited_ae);

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
    * @param parameters is the set of input parameters
    * @param frontend_flow_step_type is the type of step; it is different for subclasses
    */
   VarDeclFix(const application_managerRef AppM, unsigned int fun_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters, const FrontendFlowStepType frontend_flow_step_type = VAR_DECL_FIX);

   /**
    * Destructor
    */
   ~VarDeclFix() override;

   /**
    * Fixes the var_decl duplication.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};
#endif
