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
 * @file virtual_phi_nodes_split.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef VIRTUAL_PHI_NODES_SPLIT_HPP
#define VIRTUAL_PHI_NODES_SPLIT_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

#include "refcount.hpp"
#include "utility.hpp"

#include "graph.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(virtual_phi_nodes_split);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_node);
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(tree_manager);
//@}

/**
 *
 */
class virtual_phi_nodes_split : public FunctionFrontendFlowStep
{
 private:
   /// flag to check if initial tree has been dumped
   static bool tree_dumped;

   /// Basic block introduced after entry
   blocRef next_entry;

   /**
    * virtually split a particular phi in two or more assignments
    * @param phi is the phi
    * @param bb_block is the current basic block
    * @param list_of_bloc is basic block of the current function
    * @param TM is the tree manager
    */
   void virtual_split_phi(tree_nodeRef phi, blocRef& bb_block, std::map<unsigned int, blocRef>& list_of_bloc, const tree_managerRef TM, std::map<std::pair<unsigned int, unsigned int>, unsigned int>& replace);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor
    * @param Param is the set of the parameters
    * @param AppM is the application manager
    * @param function_id is the index of the function
    * @param design_flow_manager is the design flow manager
    */
   virtual_phi_nodes_split(const ParameterConstRef Param, const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~virtual_phi_nodes_split() override;

   /**
    * Performs the virtual splitting of the phi-nodes.
    */
   DesignFlowStep_Status InternalExec() override;
};

#endif
