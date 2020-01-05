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
 * @file build_virtual_phi.hpp
 * @brief Analysis step building phi of vops
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef BUILD_VIRTUAL_PHI_HPP
#define BUILD_VIRTUAL_PHI_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// tree include
#include "tree_node.hpp"

/// utility includes
#include "refcount.hpp"

REF_FORWARD_DECL(BBGraph);
REF_FORWARD_DECL(tree_manager);
REF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);

class BuildVirtualPhi : public FunctionFrontendFlowStep
{
 private:
   /// The tree manager
   const tree_managerRef TM;

   /// the tree manipulation
   tree_manipulationRef tree_man;

   /// The graph of basic blocks
   BBGraphRef basic_block_graph;

   /// Cache of created phi - first key is the used ssa - second key is the basic block where is created
   TreeNodeMap<CustomUnorderedMapStable<vertex, tree_nodeRef>> added_phis;

   /// Cache of reaching defs - first key is the used ssa - second key is the basic block to be considered
   TreeNodeMap<CustomUnorderedMapStable<vertex, tree_nodeRef>> reaching_defs;

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param function_id is the node id of the function analyzed.
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   BuildVirtualPhi(const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~BuildVirtualPhi() override;

   /**
    * Performs the loops analysis
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

   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;
};
#endif
