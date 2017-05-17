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
 * @file reg_binding.cpp
 * @brief add register file and add selector to their input
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
*/
#include "reg_binding_cs.hpp"
#include "omp_functions.hpp"
#include "hls.hpp"
#include "hls_manager.hpp"
#include "structural_objects.hpp"
#include "hls_target.hpp"
#include "structural_manager.hpp"
#include "technology_manager.hpp"
#include "Parameter.hpp"

reg_binding_cs::reg_binding_cs(const hlsRef& HLS_, const HLS_managerRef HLSMgr_) :
    reg_binding(HLS_, HLSMgr_)
{
}

reg_binding_cs::~reg_binding_cs()
{
}

std::string reg_binding_cs::CalculateRegisterName(unsigned int )
{
   return "rams_dist";
}

void reg_binding_cs::add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port)
{
   reg_binding::add_to_SM(clock_port, reset_port);
   auto omp_functions = GetPointer<OmpFunctions>(HLSMgr->Rfuns);
   if(omp_functions->kernel_functions.find(HLS->functionId) != omp_functions->kernel_functions.end())
   {
      //selector connected when scheduler instantiated
   }
   else
   {
      add_register_file_function();
   }
}

void reg_binding_cs::specialise_reg(structural_objectRef & reg, unsigned int r)
{
   reg_binding_cs::specialise_reg(reg, r);
   unsigned int dimension=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
   for(unsigned int i=0;i<GetPointer<module>(reg)->get_in_port_size();i++)
   {
      if (GetPointer<module>(reg)->get_in_port(i)->get_id() == SELECTOR_REGISTER_FILE)
      {
         GetPointer<module>(reg)->get_in_port(i)->type_resize(dimension); // selector
      }
   }
}
