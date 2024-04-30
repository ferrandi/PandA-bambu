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
 * @file string_cst_fix.hpp
 * @brief Pre-analysis step fixing readonly initializations and string_cst references.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef STRING_CST_FIX_HPP
#define STRING_CST_FIX_HPP

/// Superclass include
#include "application_frontend_flow_step.hpp"

/// STL include
#include "custom_map.hpp"
#include "custom_set.hpp"

/// Utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
//@}

/**
 * Pre-analysis step. It transforms the raw intermediate representation by removing
 * direct references to string_cst.
 */
class string_cst_fix : public ApplicationFrontendFlowStep
{
 protected:
   /// Already visited address expression (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited_ae;

   /// relation between constant string and read-only variable initialized with the string_cst.
   CustomUnorderedMap<unsigned int, tree_nodeRef> string_cst_map;

   /**
    * Recursive tree node analysis
    */
   void recursive_analysis(tree_nodeRef& tn, const std::string& srcp);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   string_cst_fix(const application_managerRef AppM, const DesignFlowManagerConstRef design_flow_manager,
                  const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~string_cst_fix() override;

   /**
    * Fixes the var_decl duplication.
    * @return the exit status of this step
    */
   DesignFlowStep_Status Exec() override;
};
#endif
