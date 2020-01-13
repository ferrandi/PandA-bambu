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
 * @file constant_flop_wrapper.cpp
 * @brief Step that recognizes when there's a floating point operation
 *        with a constant and optimize it.
 *
 * @author Nicolas Tagliabue
 * @author Lorenzo Porro
 */

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// Utility include
#include "custom_set.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(soft_float_cg_ext);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
CONSTREF_FORWARD_DECL(tree_node);
//@}

class constant_flop_wrapper : public FunctionFrontendFlowStep
{
 protected:
   /// The set of already created functions
   static CustomOrderedSet<std::string> operations;

   /// Tree manager
   const tree_managerRef TreeM;

   /// tree manipulation
   const tree_manipulationRef tree_man;

   /**
    * Integrate the tree manager with the soft float functions with a constant parameter
    * @param functions_to_be_created is the set of functions to be generated
    */
   void SoftFloatWriter(CustomSet<std::pair<std::string, tree_nodeConstRef>> functions_to_be_created);

   /**
    * Compute the name of a function with implicit constant operand
    * @param function_name is the name of the function
    * @param constant is the tree node storing the constant
    */
   std::string GenerateFunctionName(const std::string& function_name, const tree_nodeConstRef constant);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param fun_id is the function index
    * @param design_flow_manager is the design flow manager
    */
   constant_flop_wrapper(const ParameterConstRef Param, const application_managerRef AppM, unsigned int fun_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~constant_flop_wrapper() override;

   /**
    * Fixes the var_decl duplication.
    */
   DesignFlowStep_Status InternalExec() override;
};
