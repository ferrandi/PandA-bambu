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
 * @file phi_opt.hpp
 * @brief Analysis step that optimize the phis starting from the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef PHI_OPT_HPP
#define PHI_OPT_HPP

/// Superclass include
#include "function_frontend_flow_step.hpp"

/// Utility include
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(bloc);
REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_manipulation);
REF_FORWARD_DECL(tree_node);
class statement_list;
class gimple_cond;
class gimple_phi;
//@}

/**
 * Identifier of patterns to be transformed by phi_opt
 */
enum class PhiOpt_PatternType
{
   UNKNOWN,        /**< Unknown pattern */
   DIFF_NOTHING,   /**< Empty basic block with multiple input */
   GIMPLE_NOTHING, /**< Empty basic block dominated by assign can be removed without further changes */
   IF_MERGE,       /**< Edges coming from if to be merged */
   IF_NOTHING,     /**< Basic block dominated by if can be removed without further changes */
   IF_REMOVE,      /**< Phi dominated by gimple_cond to be removed */
   MULTI_MERGE,    /**< Phi dominated by gimple_multi_way_if to be merged */
   MULTI_NOTHING,  /**< Basic block dominated by multi way if can be removed without further changes */
   MULTI_REMOVE,   /**< Phi dominated by gimple_multi_way_if to be removed */
   UNCHANGED       /**< Transformation blocked by timing */
};

/**
 * Restructure the tree control flow graph
 */
class PhiOpt : public FunctionFrontendFlowStep
{
   friend class short_circuit_taf;

 private:
   /// The tree manager
   tree_managerRef TM;

   /// The tree manipulation
   tree_manipulationConstRef tree_man;

   /// The basic block graph of the function
   statement_list* sl{nullptr};

   /// flag to check if initial tree has been dumped
   static bool tree_dumped;

   /// flag used to restart code motion step
   bool bb_modified;

   /// The scheduling solution
   ScheduleRef schedule;

   /**
    * Identify to which pattern an empty basic block belongs
    * @param bb_index is the index of the empty basic block
    * @return the identified pattern
    */
   PhiOpt_PatternType IdentifyPattern(const unsigned int bb_index) const;

   /**
    * Remove an empty basic block with multiple input edges
    */
   void ApplyDiffNothing(const unsigned int bb_index);

   /**
    * Remove an empty basic block dominated by gimple assign
    * @param bb_index is the index of the empty basic block
    */
   void ApplyGimpleNothing(const unsigned int bb_index);

   /**
    * Transform the control flow graph by merging a gimple_phi dominated by a gimple_cond
    * @param bb_index is the index of the empty basic block
    */
   void ApplyIfMerge(const unsigned int bb_index);

   /**
    * Transform the control flow graph by eliminating an empty basic block dominated by gimple_cond without modifying phi
    * @param bb_index is the index of the empty basic block
    */
   void ApplyIfNothing(const unsigned int bb_index);

   /**
    * Transform the control flow graph by removing a gimple_phi dominated by a gimple_cond
    * @param bb_index is the index of the empty basic block
    */
   void ApplyIfRemove(const unsigned int bb_index);

   /**
    * Transform the control flow graph by merging a gimple_phi dominated by a gimple_multi_way_if
    * @param bb_index is the index of the empty basic block
    */
   void ApplyMultiMerge(const unsigned int bb_index);

   /**
    * Transform the control flow graph by eliminating an empty basic block dominated by gimple_multi_way_if without modifying phi
    * @param bb_index is the index of the empty basic block
    */
   void ApplyMultiNothing(const unsigned int bb_index);

   /**
    * Transform the control flow graph by removing a gimple_phi dominated by a gimple_multi_way_if
    * @param bb_index is the index of the empty basic block
    */
   void ApplyMultiRemove(const unsigned int bb_index);

   /**
    * Transform single input phi in assignment
    */
   void SinglePhiOptimization(const unsigned int bb_index);

   /**
    * Remove chains of basic blocks
    * @param bb_index is the starting basic block index
    */
   void ChainOptimization(const unsigned int bb_index);

   /**
    * Remove a basic block composed only of phis my merging with the successor
    * @param block is the index of the basic block
    */
   void MergePhi(const unsigned int bb_index);

   /**
    * Remove a redundant cond expr
    * @param statement is the statement containing
    */
   void RemoveCondExpr(const tree_nodeRef statement);

   /**
    * Return the set of analyses in relationship with this design step
    * @param relationship_type is the type of relationship to be considered
    */
   const CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param AppM is the application manager
    * @param function_id is the identifier of the function
    * @param design_flow_manager is the design flow manager
    * @param parameters is the set of input parameters
    */
   PhiOpt(const application_managerRef AppM, unsigned int function_id, const DesignFlowManagerConstRef design_flow_manager, const ParameterConstRef parameters);

   /**
    *  Destructor
    */
   ~PhiOpt() override;

   /**
    * Updates the tree to have a more compliant CFG
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

   /**
    * Compute the relationships of a step with other steps
    * @param dependencies is where relationships will be stored
    * @param relationship_type is the type of relationship to be computed
    */
   void ComputeRelationships(DesignFlowStepSet& relationship, const DesignFlowStep::RelationshipType relationship_type) override;
};
#endif
