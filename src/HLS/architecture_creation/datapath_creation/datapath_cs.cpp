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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
 * @file classic_datapath.cpp
 * @brief Base class for usual datapath creation.
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/

#include "omp_functions.hpp"
#include "hls.hpp"
#include "hls_target.hpp"
#include "hls_manager.hpp"
#include "function_behavior.hpp"
#include "behavioral_helper.hpp"

#include "generic_obj.hpp"
#include "mux_conn.hpp"
#include "mux_obj.hpp"
#include "commandport_obj.hpp"
#include "dataport_obj.hpp"

#include "technology_manager.hpp"

#include "BambuParameter.hpp"
#include "schedule.hpp"
#include "fu_binding.hpp"
#include "reg_binding.hpp"
#include "conn_binding.hpp"
#include "memory.hpp"

#include "exceptions.hpp"

#include "structural_objects.hpp"
#include "structural_manager.hpp"

#include "tree_reindex.hpp"
#include "tree_manager.hpp"
#include "tree_node.hpp"

#include "dbgPrintHelper.hpp"
#include "Parameter.hpp"

#include <iosfwd>
#include <boost/lexical_cast.hpp>

///technology/physical_library include
#include "technology_node.hpp"

#include "copyrights_strings.hpp"
#include "datapath_cs.hpp"

datapath_CS::datapath_CS(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager) :
   classic_datapath(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::DATAPATH_CS_CREATOR)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this));
}

datapath_CS::~datapath_CS()
{
}

DesignFlowStep_Status datapath_CS::InternalExec()
{
    classic_datapath::Exec();    //exec of hierarchical class
    for (auto const functional_unit : HLS->Rfu)
    {

    }
}

void datapath_CS::add_ports()
{
}
