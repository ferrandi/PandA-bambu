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
 * @file soft_int_cg_ext.hpp
 * @brief Step that extends the call graph with software implementation of integer operators.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SOFT_INT_CG_EXT_HPP
#define SOFT_INT_CG_EXT_HPP

/// Superclass include
#include "application_frontend_flow_step.hpp"

/// Utility include
#include "custom_set.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(soft_int_cg_ext);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
//@}

/**
 * Add to the call graph the function calls associated with the integer division and modulus operations.
 */
class soft_int_cg_ext : public ApplicationFrontendFlowStep
{
 private:
   /// Already visited tree node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   const tree_managerRef TreeM;

   bool doSoftDiv;

   CustomOrderedSet<unsigned int> fun_id_to_restart;

   /**
    * Recursive examine tree node
    */
   bool recursive_transform(unsigned int function_id, const tree_nodeRef& current_tree_node,
                            const tree_nodeRef& current_statement, const tree_manipulationRef tree_man);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void ComputeRelationships(DesignFlowStepSet& relationships,
                             const DesignFlowStep::RelationshipType relationship_type) override;

 public:
   /**
    * Constructor.
    * @param AM is the application manager
    * @param dfm is the design flow manager
    * @param par is the set of the parameters
    */
   soft_int_cg_ext(const application_managerRef AM, const DesignFlowManagerConstRef dfm, const ParameterConstRef par);

   /**
    * Destructor
    */
   ~soft_int_cg_ext() override;

   /**
    * transform soft int operation into function call
    */
   DesignFlowStep_Status Exec() override;
};
#endif
