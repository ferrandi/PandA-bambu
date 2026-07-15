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
 *   Copyright (C) 2016-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file top_entity_omp_cs.hpp
 *
 * @author Giovanni Gozzi <giovanni.gozzi@polimi.it>
 *
 */
#ifndef TOP_ENTITY_OMP_CS_H
#define TOP_ENTITY_OMP_CS_H
#include "top_entity.hpp"

#include <list>
#include <string>

class top_entity_omp_cs : public top_entity
{
 public:
   top_entity_omp_cs(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                     const DesignFlowManager& design_flow_manager,
                     const HLSFlowStep_Type _hls_flow_step_type = HLSFlowStep_Type::TOP_ENTITY_OMP_CS_CREATION);

   /**
    * Add selector and suspension
    * @return the exit status of this step
    */
   virtual DesignFlowStep_Status InternalExec() override;
};

#endif // TOP_ENTITY_OMP_CS_H
