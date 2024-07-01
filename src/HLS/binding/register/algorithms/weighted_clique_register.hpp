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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file weighted_clique_register.hpp
 * @brief Weighted clique covering register allocation procedure
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef WEIGHTED_CLIQUE_REGISTER_HPP
#define WEIGHTED_CLIQUE_REGISTER_HPP

#include "compatibility_based_register.hpp"

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

class weighted_clique_register : public compatibility_based_register
{
 private:
   DesignFlowStep_Status RegisterBinding() final;

 public:
   /**
    * Constructor of the class.
    * @param design_flow_manager is the design flow manager
    */
   weighted_clique_register(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, unsigned int _funId,
                            const DesignFlowManagerConstRef design_flow_manager,
                            const HLSFlowStepSpecializationConstRef hls_flow_step_specialization);

   ~weighted_clique_register() override;

   void Initialize() override;
};

#endif // WEIGHTED_CLIQUE_HPP
