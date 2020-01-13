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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file reg_binding.cpp
 * @brief add register file and add selector to their input
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */
#include "reg_binding_cs.hpp"
#include "Parameter.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "hls_target.hpp"
#include "math_function.hpp"
#include "omp_functions.hpp"
#include "structural_manager.hpp"
#include "structural_objects.hpp"
#include "technology_manager.hpp"

/// STD include
#include <string>

reg_binding_cs::reg_binding_cs(const hlsRef& HLS_, const HLS_managerRef HLSMgr_) : reg_binding(HLS_, HLSMgr_)
{
}

reg_binding_cs::~reg_binding_cs()
{
}

std::string reg_binding_cs::CalculateRegisterName(unsigned int)
{
   return "register_file";
}

void reg_binding_cs::specialise_reg(structural_objectRef& reg, unsigned int r)
{
   reg_binding::specialise_reg(reg, r);
   unsigned int mem_dimension = HLS->Param->getOption<unsigned int>(OPT_context_switch);
   int dimension = ceil_log2(HLS->Param->getOption<unsigned long long int>(OPT_context_switch));
   if(!dimension)
      dimension = 1;
   structural_objectRef selector_port = reg->find_member(SELECTOR_REGISTER_FILE, port_o_K, reg);
   if(selector_port != nullptr)
   {
      selector_port->type_resize(static_cast<unsigned>(dimension)); // selector
   }
   GetPointer<module>(reg)->SetParameter("n_elements", STR(mem_dimension));
   for(unsigned int j = 0; j < GetPointer<module>(reg)->get_in_port_size(); j++) // connect input scheduler with datapath input
   {
      structural_objectRef port_i = GetPointer<module>(reg)->get_in_port(j);
      std::string port_name = GetPointer<port_o>(port_i)->get_id();
   }
}
