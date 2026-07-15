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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file buildFSM.hpp
 * @brief create the FSM. Loop and functional pipelining supported.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef BUILDFSM_HPP
#define BUILDFSM_HPP

#include "hls_function_step.hpp"

#include "op_graph.hpp"

class buildFSM : public HLSFunctionStep
{
   using operation_descriptor = OpGraph::vertex_descriptor;

   HLSRelationships ComputeHLSRelationships(const DesignFlowStep::RelationshipType relationship_type) const override;

   void print_statistics() const;

   /**
    * Compute minimum and maximum number of cycles for bounded scheduling
    * @param latency Latency for pipelined functions
    */
   void ComputeCyclesCount(unsigned latency);

 public:
   buildFSM(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
            const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

   void Initialize() override;
};
#endif
