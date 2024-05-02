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
 * @file design_flow_step.cpp
 * @brief Base class for step of design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "design_flow_step.hpp"

#include "Parameter.hpp"
#include "design_flow_manager.hpp"

#include <ostream>
#include <string>

DesignFlowStep::DesignFlowStep(signature_t _signature, const DesignFlowManagerConstRef _design_flow_manager,
                               const ParameterConstRef _parameters)
    : composed(false),
      design_flow_manager(_design_flow_manager),
      parameters(_parameters),
      debug_level(parameters->getOption<int>(OPT_debug_level)),
      output_level(parameters->getOption<int>(OPT_output_level)),
      signature(_signature)
{
}

DesignFlowStep::~DesignFlowStep() = default;

void DesignFlowStep::WriteDot(std::ostream& out) const
{
   out << "label=\"" << GetName() << "\\n"
       << "Signature: " << GetSignature() << "\"";
}

bool DesignFlowStep::IsComposed() const
{
   return composed;
}

void DesignFlowStep::Initialize()
{
}

DesignFlowStep::signature_t DesignFlowStep::GetSignature() const
{
   return signature;
}

std::string DesignFlowStep::GetName() const
{
   return "DesignFlowStep::" + STR(signature);
}

int DesignFlowStep::CGetDebugLevel() const
{
   return debug_level;
}

DesignFlowStep_Status DesignFlowStep::GetStatus() const
{
   return design_flow_manager.lock()->GetStatus(GetSignature());
}

void DesignFlowStep::PrintInitialIR() const
{
}

void DesignFlowStep::PrintFinalIR() const
{
}

DesignFlowStep::signature_t DesignFlowStep::ComputeSignature(StepClass step_class, unsigned short step_type,
                                                             unsigned long long context)
{
   THROW_ASSERT(context < (1ULL << 40U), "Only 40-bits context is allowed.");
   return static_cast<signature_t>(step_class) << 56U | static_cast<signature_t>(step_type) << 40U |
          (context & 0xFFFFFFFFFFULL);
}

DesignFlowStep::StepClass DesignFlowStep::GetStepClass(signature_t signature)
{
   return static_cast<StepClass>(signature >> 56U);
}

unsigned short DesignFlowStep::GetStepType(signature_t signature)
{
   return static_cast<unsigned short>(signature >> 40U);
}

unsigned long long DesignFlowStep::GetSignatureContext(signature_t signature)
{
   return signature & 0xFFFFFFFFFFULL;
}

size_t DesignFlowStepHash::operator()(const DesignFlowStepRef& step) const
{
   return step->GetSignature();
}

bool DesignFlowStepEqual::operator()(const DesignFlowStepRef& x, const DesignFlowStepRef& y) const
{
   return x->GetSignature() == y->GetSignature();
}
