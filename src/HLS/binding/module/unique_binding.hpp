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
 * @file unique_binding.hpp
 * @brief Class to create a unique binding
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 */
#ifndef UNIQUE_BINDING_HPP
#define UNIQUE_BINDING_HPP
#include "fu_binding_creator.hpp"

/**
 * Class managing the module allocation.
 */
class unique_binding : public fu_binding_creator
{
 public:
   unique_binding(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId,
                  const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;
};

#endif
