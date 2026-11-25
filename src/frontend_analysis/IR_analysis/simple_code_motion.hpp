/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file simple_code_motion.hpp
 * @brief Analysis step that performs some simple code motions over the IR
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef SIMPLE_CODE_MOTION_HPP
#define SIMPLE_CODE_MOTION_HPP
#include "function_frontend_flow_step.hpp"

#include "custom_map.hpp"
#include "refcount.hpp"

// Keep synchronization barriers separate from operations with native hardware predicates.
#define SCM_OMP_CRITICAL "__kmp_bambu_critical"
#define SCM_OMP_END_CRITICAL "__kmp_bambu_end_critical"
#define SCM_OMP_BARRIER_REACHED "__kmp_bambu_barrier_reached"
#define SCM_OMP_WAIT_ALL_THREADS "__kmp_bambu_wait_all_threads"
#define SCM_OMP_CS_WAIT_ALL_THREADS "__kmp_bambu_cs_wait_all_threads"
#define SCM_OMP_FORK_CALL "__kmp_bambu_fork_call"
#define SCM_OMP_SET_NUM_THREADS "__kmp_bambu_th_set_nproc"
#define SCM_OMP_SET_REDUCE_DATA "__kmp_bambu_set_th_local_reduce_data"
#define SCM_OMP_GET_REDUCE_DATA "__kmp_bambu_get_th_local_reduce_data"

// Include team setup and reduction handoffs as well as locks and barriers.
#define SCM_OMP_SYNCHRONIZATION_FUNCTIONS \
   SCM_OMP_CRITICAL, SCM_OMP_END_CRITICAL, SCM_OMP_BARRIER_REACHED, SCM_OMP_WAIT_ALL_THREADS, \
       SCM_OMP_CS_WAIT_ALL_THREADS, SCM_OMP_FORK_CALL, SCM_OMP_SET_NUM_THREADS, SCM_OMP_SET_REDUCE_DATA, \
       SCM_OMP_GET_REDUCE_DATA

// These operations can also move onto conditional paths using native hardware predication.
#define SCM_OMP_PREDICABLE_FUNCTIONS \
   SCM_OMP_CRITICAL, SCM_OMP_END_CRITICAL, SCM_OMP_BARRIER_REACHED, SCM_OMP_WAIT_ALL_THREADS, \
       SCM_OMP_CS_WAIT_ALL_THREADS, SCM_OMP_SET_REDUCE_DATA, SCM_OMP_GET_REDUCE_DATA

// No operation, including pure computations, may cross a fork call.
#define SCM_OMP_FULL_BARRIER_FUNCTIONS SCM_OMP_FORK_CALL

REF_FORWARD_DECL(Schedule);
REF_FORWARD_DECL(ir_manager);
REF_FORWARD_DECL(ir_node);
REF_FORWARD_DECL(bloc);
class ssa_node;
class assign_stmt;

/**
 * Restructure the IR control flow graph
 */
class simple_code_motion : public FunctionFrontendFlowStep
{
 private:
   bool restart_ifmwi_opt;

   /// The scheduling solution
   ScheduleRef schedule;

   /// Flag to check if the initial IR has been dumped.
   static bool ir_dumped;

   /// True if only zero delay statement can be moved
   bool conservative;

   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>>
   ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   /**
    * Check if a statement can be moved in a basic block
    * @param dest_bb_index is the index of the destination basic block
    * @param tn is the statement to be moved
    * @param zero_delay returns if statement has zero delay
    * @return if the statement can be moved
    */
   FunctionFrontendFlowStep_Movable CheckMovable(const unsigned int dest_bb_index, ir_nodeRef tn, bool& zero_delay);

   void loop_pipelined(ir_nodeRef curr_stmt, const ir_managerRef TM, unsigned int curr_bb, unsigned int curr_loop_id,
                       std::list<ir_nodeRef>& to_be_removed, std::list<ir_nodeRef>& to_be_added_back,
                       std::list<ir_nodeRef>& to_be_added_front, std::map<unsigned int, blocRef>& list_of_bloc,
                       std::map<std::pair<unsigned int, blocRef>, std::pair<unsigned int, blocRef>>& dom_diff,
                       unsigned int curr_bb_dom);

 public:
   simple_code_motion(const ParameterConstRef parameters, const application_managerRef AppM, unsigned int function_id,
                      const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;

   bool IsScheduleBased() const;
};
#endif
