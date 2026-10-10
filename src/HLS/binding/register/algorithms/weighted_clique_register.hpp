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
