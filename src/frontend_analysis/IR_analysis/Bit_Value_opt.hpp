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
 * @file Bit_Value_opt.hpp
 * @brief Class performing some optimizations on the IR exploiting Bit Value analysis.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BIT_VALUE_OPT_HPP
#define BIT_VALUE_OPT_HPP

#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(Bit_Value_opt);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
struct function_decl;
struct ssa_name;
//@}

/**
 * @brief Class performing some optimizations on the IR exploiting Bit Value analysis.
 */
class Bit_Value_opt : public FunctionFrontendFlowStep
{
 private:
   /// when true IR has been modified
   bool modified;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * do bit value based optimization such as:
    * - constant propagation
    * @param fd is the function declaration
    * @param TM is the tree manager
    */
   void optimize(const function_decl* fd, const tree_managerRef& TM, const tree_manipulationRef& IRman);

   void propagateValue(const tree_managerRef& TM, const tree_nodeRef& old_val, const tree_nodeRef& new_val,
                       const std::string& callSiteString);

 public:
   /**
    * Constructor.
    * @param _Param is the set of the parameters
    * @param _AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    */
   Bit_Value_opt(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id,
                 const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~Bit_Value_opt() override;

   /**
    * Optimize IR after the BitValue/BitValueIPA has been executed
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;

   static void constrainSSA(ssa_name* op_ssa, const tree_managerRef& TM);
};
#endif /* Bit_Value_opt_HPP */
