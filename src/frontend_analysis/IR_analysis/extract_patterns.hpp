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
 * @file extract_patterns.hpp
 * @brief Class extracting patterns extending the GCC/CLANG IR. An example is the ternary_plus_expr.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef EXTRACT_PATTERNS_HPP
#define EXTRACT_PATTERNS_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(extract_patterns);
REF_FORWARD_DECL(tree_manager);
class statement_list;
//@}

/**
 * @brief Class extracting patterns extending the GCC/CLANG IR.
 */
class extract_patterns : public FunctionFrontendFlowStep
{
 private:
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * look for ternary plus expressions
    * @param sl is the statement list
    * @param TM is the tree manager
    */
   void ternary_plus_expr_extraction(statement_list* sl, tree_managerRef TM);

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   extract_patterns(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~extract_patterns() override;
   /**
    * Extract patterns from the GCC/CLANG IR.
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif /* EXTRACT_PATTERNS_HPP */
