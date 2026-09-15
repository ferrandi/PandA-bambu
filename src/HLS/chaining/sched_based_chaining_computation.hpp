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
 * @file sched_based_chaining_computation.hpp
 * @brief chaining computation starting from the results of the scheduling step
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#ifndef SCHED_BASED_CHAINING_COMPUTATION_HPP
#define SCHED_BASED_CHAINING_COMPUTATION_HPP

#include "chaining.hpp"

REF_FORWARD_DECL(ChainingInformation);

class sched_based_chaining_computation : public chaining
{
 public:
   sched_based_chaining_computation(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                                    const DesignFlowManager& design_flow_manager);

   void Initialize() override;

   DesignFlowStep_Status InternalExec() override;
};

#endif
