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
 * @file vertex_coloring_register.hpp
 * @brief Class specification of a coloring based register allocation algorithm
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef VERTEX_COLORING_REGISTER_HPP
#define VERTEX_COLORING_REGISTER_HPP

#include "conflict_based_register.hpp"

class vertex_coloring_register : public conflict_based_register
{
 private:
   DesignFlowStep_Status RegisterBinding() final;

 public:
   /**
    * Constructor of the class.
    * @param Param is the parameter set
    * @param HLSMgr is the HLS manager
    * @param funId is the function identifier
    * @param design_flow_manager is the design flow manager
    */
   vertex_coloring_register(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                            const DesignFlowManager& design_flow_manager);
};

#endif
