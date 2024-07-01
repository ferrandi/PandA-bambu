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
 * @file tree2fun.hpp
 * @brief Step that replace some tree node expression with function call
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef TREE2FUN_HPP
#define TREE2FUN_HPP
#include "custom_set.hpp"
#include "function_frontend_flow_step.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree2fun);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
//@}

/**
 * Add to the call graph the function calls associated with the integer division and modulus operations.
 */
class tree2fun : public FunctionFrontendFlowStep
{
 private:
   /// Already visited tree node (used to avoid infinite recursion)
   CustomUnorderedSet<unsigned int> already_visited;

   const tree_managerRef TreeM;

   /**
    * Recursively examine tree node
    */
   bool recursive_transform(const tree_nodeRef& current_tree_node, const tree_nodeRef& current_statement,
                            const tree_manipulationRef tree_man);

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param fun_id is the function index
    * @param design_flow_manager is the design flow manager
    */
   tree2fun(const ParameterConstRef _parameters, const application_managerRef AppM, unsigned int _function_id,
            const DesignFlowManagerConstRef design_flow_manager);

   ~tree2fun() override;

   DesignFlowStep_Status InternalExec() override;
};
#endif
