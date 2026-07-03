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
 * @file design_flow_aux_step.cpp
 * @brief Class for describing auxiliary steps in design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "design_flow_aux_step.hpp"

#include "exceptions.hpp"
#include "string_manipulation.hpp"

#include <ostream>
#include <utility>

static inline std::string GetTypeString(AuxDesignFlowStepType t)
{
   return t == DESIGN_FLOW_ENTRY ? "Entry" : "Exit";
}

AuxDesignFlowStep::AuxDesignFlowStep(AuxDesignFlowStepType _type, const DesignFlowManager& _design_flow_manager,
                                     const ParameterConstRef _parameters)
    : DesignFlowStep(ComputeSignature(_type), _design_flow_manager, _parameters), type(_type)
{
}

void AuxDesignFlowStep::ComputeRelationships(DesignFlowStepSet&, const DesignFlowStep::RelationshipType)
{
}

DesignFlowStep::signature_t AuxDesignFlowStep::ComputeSignature(const AuxDesignFlowStepType type)
{
   return DesignFlowStep::ComputeSignature(AUX, type, 0);
}

DesignFlowStep_Status AuxDesignFlowStep::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

std::string AuxDesignFlowStep::GetName() const
{
   return "AUX::" + GetTypeString(type);
}

void AuxDesignFlowStep::writeDot(std::ostream& out) const
{
   out << "shape=Msquare, label=\"" << GetTypeString(type) << "\"";
}

DesignFlowStepFactoryConstRef AuxDesignFlowStep::CGetDesignFlowStepFactory() const
{
   THROW_UNREACHABLE("This method should never be called");
   return DesignFlowStepFactoryConstRef();
}

bool AuxDesignFlowStep::HasToBeExecuted() const
{
   return true;
}
