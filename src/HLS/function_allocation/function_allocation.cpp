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
 * @file function_allocation.cpp
 * @brief Base class to allocate functions in high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "function_allocation.hpp"
#include "fun_dominator_allocation.hpp"
#include "hls_manager.hpp"

#include "Parameter.hpp"
#include "polixml.hpp"
#include "xml_helper.hpp"

/// HLS/function_allocation include
#include "functions.hpp"

function_allocation::function_allocation(const ParameterConstRef _Param, const HLS_managerRef _HLSMgr, const DesignFlowManagerConstRef _design_flow_manager, const HLSFlowStep_Type _hls_flow_step_type)
    : HLS_step(_Param, _HLSMgr, _design_flow_manager, _hls_flow_step_type)
{
}

function_allocation::~function_allocation() = default;

void function_allocation::Initialize()
{
   HLS_step::Initialize();
   HLSMgr->Rfuns = functionsRef(new functions());
}
