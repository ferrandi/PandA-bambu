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
 * @file short_circuit_taf.hpp
 * @brief Analysis step rebuilding a short circuit in a single gimple_cond with the condition in three address form.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SHORT_CIRCUIT_TAF_HPP
#define SHORT_CIRCUIT_TAF_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
class statement_list;
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
class short_circuit_taf : public FunctionFrontendFlowStep
{
 private:
   /**
    * Check if a basic block is a merging BB for a short circuit.
    */
   bool check_merging_candidate(unsigned int& bb1, unsigned int& bb2, unsigned int merging_candidate, bool& bb1_true, bool& bb2_true, std::map<unsigned int, blocRef>& list_of_bloc);

   /**
    * create the or/and expression required by short circuit collapsing
    */
   bool create_gimple_cond(unsigned int bb1, unsigned int bb2, bool bb1_true, std::map<unsigned int, blocRef>& list_of_bloc, bool or_type, unsigned int merging_candidate);

   /**
    * restructure the CFG eliminating all BBs not needed after short circuit collapsing
    */
   void restructure_CFG(unsigned int bb1, unsigned int bb2, unsigned int merging_candidate, std::map<unsigned int, blocRef>& list_of_bloc);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * @brief check if phi could create problem to the short circuit collapsing
    * @param curr_bb is the basic block that merge the two or more flows
    * @param list_of_bloc is the list of basic blocks
    * @return true in case the short circuit merging can be performed
    */
   bool check_phis(unsigned int curr_bb, std::map<unsigned int, blocRef>& list_of_bloc);

   void fix_multi_way_if(unsigned int curr_bb, std::map<unsigned int, blocRef>& list_of_bloc, unsigned int succ);

 public:
   /**
    * Constructor.
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param DesignFlowManagerConstRef is the design flow manager
    */
   short_circuit_taf(const ParameterConstRef _Param, const application_managerRef _AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    *  Destructor
    */
   ~short_circuit_taf() override;

   /**
    * Restructures the unstructured code
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;
};
#endif
