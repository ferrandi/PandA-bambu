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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file design_flow_aux_step.cpp
 * @brief Class for describing auxiliary steps in design flow
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "design_flow_aux_step.hpp"

#include "exceptions.hpp"         // for THROW_UNREACHABLE
#include <boost/lexical_cast.hpp> // for lexical_cast
#include <ostream>                // for operator<<, basic_ostream
#include <utility>

AuxDesignFlowStep::AuxDesignFlowStep(std::string _name, const AuxDesignFlowStepType _type, const DesignFlowManagerConstRef _design_flow_manager, const ParameterConstRef _parameters)
    : DesignFlowStep(_design_flow_manager, _parameters), type(_type), name(std::move(_name))
{
}

AuxDesignFlowStep::~AuxDesignFlowStep() = default;

void AuxDesignFlowStep::ComputeRelationships(DesignFlowStepSet&, const DesignFlowStep::RelationshipType)
{
}

const std::string AuxDesignFlowStep::ComputeSignature(const std::string& name, const AuxDesignFlowStepType type)
{
   return "AUX::" + boost::lexical_cast<std::string>(type) + "::" + name;
}

DesignFlowStep_Status AuxDesignFlowStep::Exec()
{
   return DesignFlowStep_Status::EMPTY;
}

const std::string AuxDesignFlowStep::GetName() const
{
   return name;
}

const std::string AuxDesignFlowStep::GetSignature() const
{
   return ComputeSignature(name, type);
}

void AuxDesignFlowStep::WriteDot(std::ostream& out) const
{
   out << "shape=Msquare, label=\"" << name << "\"";
}

const DesignFlowStepFactoryConstRef AuxDesignFlowStep::CGetDesignFlowStepFactory() const
{
   THROW_UNREACHABLE("This method should never be called");
   return DesignFlowStepFactoryConstRef();
}

bool AuxDesignFlowStep::HasToBeExecuted() const
{
   return true;
}
