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
 * @file remove_enfing_if.hpp
 * @brief Analysis step removing ending if, with only phi or return operation.
 *
 * @author Marco Arnaboldi <marco1.arnaboldi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef REMOVE_ENDING_IF_HPP
#define REMOVE_ENDING_IF_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(Schedule);
class statement_list;
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
//@}

/// STL include
#include <deque>
#include <list>
#include <string>

#include "custom_map.hpp"
#include "custom_set.hpp"

/**
 * Structure the original short circuit
 */
class RemoveEndingIf : public FunctionFrontendFlowStep
{
 private:
   /// The statement list of the analyzed function
   statement_list* sl;

   /// The tree manager
   tree_managerRef TM;

   /// The tree manipulation
   tree_manipulationRef tree_man;

   /// The scheduling solution
   ScheduleRef schedule;

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
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   RemoveEndingIf(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~RemoveEndingIf() override;

   /**
    * Restructures the unstructured code
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Check if this step has actually to be executed
    * @return true if the step has to be executed
    */
   bool HasToBeExecuted() const override;
};
#endif
