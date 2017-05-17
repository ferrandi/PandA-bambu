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

void reg_binding_cs::add_register_file_function()
{
   const structural_managerRef& SM = HLS->datapath;
   const structural_objectRef& circuit = SM->get_circ();
   structural_objectRef selector_register_file_datapath = circuit->find_member(SELECTOR_REGISTER_FILE,port_o_K,circuit);
   for (unsigned int i = 0; i < get_used_regs(); i++)
   {
      generic_objRef regis = get(i);
      std::string name = regis->get_string();
      structural_objectRef registerFile = circuit->find_member(name, component_o_K, circuit);
      structural_objectRef port_selector = registerFile->find_member(SELECTOR_REGISTER_FILE, port_o_K, registerFile);
      SM->add_connection(selector_register_file_datapath, port_selector);
   }
}

void reg_binding_cs::add_register_file_kernel(structural_objectRef selector_regFile_sign)
{
   const structural_managerRef& SM = HLS->datapath;
   const structural_objectRef& circuit = SM->get_circ();
   for (unsigned int i = 0; i < get_used_regs(); i++)
   {
      generic_objRef regis = get(i);
      std::string name = regis->get_string();
      structural_objectRef registerFile = circuit->find_member(name, component_o_K, circuit);
      structural_objectRef port_selector = registerFile->find_member(SELECTOR_REGISTER_FILE, port_o_K, registerFile);
      SM->add_connection(selector_regFile_sign, port_selector);
   }
}

void reg_binding_cs::specialise_reg(structural_objectRef & reg, unsigned int r)
{
   reg_binding_cs::specialise_reg(reg, r);
   unsigned int offset=2;
   if (GetPointer<module>(reg)->get_in_port(0)->get_id() == CLOCK_PORT_NAME)
   {
      if(GetPointer<module>(reg)->get_in_port(1)->get_id() == RESET_PORT_NAME)
         offset = 3;
      else
         offset = 2;
   }
   unsigned int dimension=static_cast<unsigned int>(log2(HLS->Param->getOption<unsigned int>(OPT_context_switch)));
   GetPointer<module>(reg)->get_in_port(offset)->type_resize(dimension); // selector
}
