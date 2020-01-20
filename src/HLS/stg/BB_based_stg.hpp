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
 * @file BB_based_stg.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BB_BASED_STG_HPP
#define BB_BASED_STG_HPP

#include "STG_creator.hpp"

CONSTREF_FORWARD_DECL(OpGraph);

class BB_based_stg : public STG_creator
{
 protected:
   /**
    * Compute the relationship of this step
    * @param relationship_type is the type of relationship to be considered
    * @return the steps in relationship with this
    */
   const CustomUnorderedSet<std::tuple<HLSFlowStep_Type, HLSFlowStepSpecializationConstRef, HLSFlowStep_Relationship>> ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

 public:
   /**
    * Constructor.
    * @param design_flow_manager is  the design flow manager
    */
   BB_based_stg(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~BB_based_stg() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

   /**
    * Initialize the step (i.e., like a constructor, but executed just before exec
    */
   void Initialize() override;

 private:
   /**
    * Given two bb linked by a forwarding edge, this method tries to move
    * overlap the execution of the last state of the bb ending the cycle
    * with the execution of the first state of the bb that begins the cycle.
    */
   void optimize_cycles(vertex bbEndingCycle, CustomUnorderedMap<vertex, vertex>& first_state, CustomUnorderedMap<vertex, vertex>& last_state, std::map<vertex, std::list<vertex>>& global_starting_ops, std::map<vertex, std::list<vertex>>& global_ending_ops,
                        std::map<vertex, std::list<vertex>>& global_executing_ops, std::map<vertex, std::list<vertex>>& global_onfly_ops);

   /**
    * Returns true if all the operations in the list can be moved to the state
    * specified.
    * Operations contained in the vertx list lastStateConditionalOpList
    * will be ignored in the analysis.
    * This method works fine only if the list of operation contains all the
    * operations executed in a state preceding the one passed as a parameter.
    */
   bool can_be_moved(std::list<vertex>& lastStateEndingOp, std::list<vertex>& lastStateConditionalOpList, vertex firstStateNextBb, std::map<vertex, std::list<vertex>>& global_starting_ops, std::map<vertex, std::list<vertex>>& global_executing_ops);

   /**
    * This method takes as parameters an operation and a state
    * of the STG graph, and returns the operation that needs
    * the output of the given operation in the given state, NULL otherwise.
    * If the result of the given operation is read by a phi operation chained
    * with a second operation, a pointer to that second operation is returned.
    * In case the result of the given operation is read by a phi which is not
    * chained to any other operation, and no other operation needs the output
    * of the given operation, a pointer to the phi itself is returned.
    *
    * NOTICE that a phi is treated as a weaker dependance, because it can be
    * solved via chaining. A phi is returned only if no stronger data
    * dependency is found.
    */
   vertex check_data_dependency(vertex operation, vertex state, std::map<vertex, std::list<vertex>>& global_starting_ops, std::map<vertex, std::list<vertex>>& global_executing_ops);

   /**
    * returns true if the operation takes no time
    */
   bool is_instantaneous_operation(vertex operation);

   /**
    * returns true if the number of fu available
    * prevents us from moving that operation in the next state
    */
   bool res_const_operation(vertex& operation, std::list<vertex>& lastStateExecutingOpList, vertex lastSt);

   /**
    * computes the variables used and defined in the
    * by the given list of operations, and saves them in the two sets.
    *
    * operations included in the ignoreList will not be considered in
    * this analysis.
    */
   void compute_use_def(const std::list<vertex>& opEndingList, const std::list<vertex>& opRuningList, const std::list<vertex>& ignoreList, CustomOrderedSet<unsigned int>& useSet, CustomOrderedSet<unsigned int>& defSet, const OpGraphConstRef data);

   /**
    * Copies all the operations of the state to move in the following.
    * An edge is created from the second last state to the destination state.
    * The state to move, and all the edges to/from it are not modified by this method.
    */
   void move_without_duplication(const vertex stateToMove, const vertex secondLastState, const vertex dest_first_state, const std::map<vertex, std::list<vertex>>& global_starting_ops, const std::map<vertex, std::list<vertex>>& global_executing_ops,
                                 const std::map<vertex, std::list<vertex>>& global_ending_ops, const CustomOrderedSet<unsigned int>& defSet, const CustomOrderedSet<unsigned int>& useSet);

   /**
    * Duplicates the first state of the destination bb and copies all the operations
    * of the state to move in the new state. An edge is created from the second last
    * state to the new state, and outgoing edges are created from the new state to all
    * the states that follows the first state of the destination bb.
    * The state to move, and all the edges to/from it are not modified by this method.
    */
   void move_with_duplication(const vertex stateToMove, const vertex secondLastState, const vertex dest_first_state, const std::map<vertex, std::list<vertex>>& global_starting_ops, const std::map<vertex, std::list<vertex>>& global_executing_ops,
                              const std::map<vertex, std::list<vertex>>& global_ending_ops, const CustomOrderedSet<unsigned int>& defSet, const CustomOrderedSet<unsigned int>& useSet);
   /**
    * If hardware discrepancy analysis is activated, use this function to
    * compute the edge increments for the Efficient Path Profiling (EPP) on the STG.
    *
    * Details on EPP and on the algorithm to compute edge increments are here:
    * Thomas Ball and James R. Larus. 1996. Efficient path profiling. In
    * Proceedings of the 29th annual ACM/IEEE international symposium on
    * Microarchitecture (MICRO 29). IEEE Computer Society, Washington, DC, USA,
    * 46-57.
    *
    * @param starting_ops: maps every state to a list of operations starting in
    * that state. They are necessary to compute a set of states when the HW
    * discrepancy analysis has to check the EPP trace, in addition to states
    * before taking feedback edges.
    */
   void compute_EPP_edge_increments(const std::map<vertex, std::list<vertex>>& starting_ops) const;
};
#endif
