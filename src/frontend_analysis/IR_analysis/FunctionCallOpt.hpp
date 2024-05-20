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
 *              Copyright (C) 2021-2024 Politecnico di Milano
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
 * @file FunctionCallOpt.hpp
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef FUNCTION_CALL_OPT_HPP
#define FUNCTION_CALL_OPT_HPP

#include "function_frontend_flow_step.hpp"

#include <utility>

#include "custom_map.hpp"
#include "custom_set.hpp"

class statement_list;
REF_FORWARD_DECL(application_manager);
CONSTREF_FORWARD_DECL(DesignFlowManager);
CONSTREF_FORWARD_DECL(Parameter);
CONSTREF_FORWARD_DECL(tree_node);

enum FunctionOptType
{
   INLINE,
   VERSION
};

class FunctionCallOpt : public FunctionFrontendFlowStep
{
 private:
   static CustomMap<unsigned int, CustomSet<std::tuple<unsigned int, FunctionOptType>>> opt_call;

   static size_t inline_max_cost;

   CustomMap<unsigned int, unsigned int> caller_bb;

   CustomUnorderedSet<unsigned int> already_visited;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Check if given call statement performs a call with all constant arguments
    * @param call_stmt considered call statement
    * @return true If all arguments of the call are constants
    * @return false If any argument is not constant
    */
   static bool HasConstantArgs(const tree_nodeConstRef& call_stmt);

   /**
    * Compute function body cost based on statements' types
    * @param body function body to be considered
    * @return size_t Cost value
    */
   size_t compute_cost(const statement_list* body, bool& has_simd);

   /**
    * Check if given function body has loops
    * @param body function body to be considered
    * @return true If body has loops between its basic blocks
    * @return false If no loops where detected in the cfg
    */
   size_t detect_loops(const statement_list* body) const;

 public:
   /// Set of always inlined functions
   static CustomSet<unsigned int> always_inline;

   /// Set of never inlined functions
   static CustomSet<unsigned int> never_inline;

   /**
    * @brief Request optimization for given call statement
    *
    * @param call_stmt the call statement optimize
    * @param caller_id id of the function where the call_stmt is present
    * @param opt type of optimization to apply
    */
   static void RequestCallOpt(const tree_nodeConstRef& call_stmt, unsigned int caller_id, FunctionOptType opt);

   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   FunctionCallOpt(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id,
                   const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~FunctionCallOpt() override;

   void Initialize() override;

   /**
    * Computes the operations CFG graph data structure.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   bool HasToBeExecuted() const override;
};
#endif
