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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file weighted_clique_register.hpp
 * @brief Weighted clique covering register allocation procedure
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef WEIGHTED_CLIQUE_REGISTER_HPP
#define WEIGHTED_CLIQUE_REGISTER_HPP
#include "reg_binding_creator.hpp"

enum class CliqueCovering_Algorithm;

class WeightedCliqueRegisterBindingSpecialization : public HLSFlowStepSpecialization
{
 public:
   /// The algorithm to be used
   const CliqueCovering_Algorithm clique_covering_algorithm;

   explicit WeightedCliqueRegisterBindingSpecialization(const CliqueCovering_Algorithm clique_covering_algorithm);

   std::string GetName() const override;

   context_t GetSignatureContext() const override;
};

class weighted_clique_register : public reg_binding_creator
{
 private:
   DesignFlowStep_Status RegisterBinding() final;

 public:
   /**
    * Constructor of the class.
    * @param _Param is the parameter set
    * @param _HLSMgr is the HLS manager
    * @param _funId is the function identifier
    * @param design_flow_manager is the design flow manager
    * @param hls_flow_step_specialization is the specialization applied to this step
    */
   weighted_clique_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                            const DesignFlowManager& design_flow_manager,
                            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   void Initialize() override;
};

#endif // WEIGHTED_CLIQUE_HPP
