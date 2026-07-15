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
