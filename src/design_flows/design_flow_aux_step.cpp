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
