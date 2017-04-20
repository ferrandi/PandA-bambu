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
      std::string register_type_name=CalculateRegisterName(i);
      std::string library = HLS->HLS_T->get_technology_manager()->get_library(register_type_name);
      structural_objectRef reg_mod = SM->add_module_from_technology_library(name, register_type_name, library, circuit, HLS->HLS_T->get_technology_manager());
      this->specialise_reg(reg_mod, i);
      structural_objectRef port_selector = reg_mod->find_member(SELECTOR_REGISTER_FILE, port_o_K, reg_mod);
      SM->add_connection(selector_register_file_datapath, port_selector);
      regis->set_structural_obj(reg_mod);
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
      std::string register_type_name=CalculateRegisterName(i);
      std::string library = HLS->HLS_T->get_technology_manager()->get_library(register_type_name);
      structural_objectRef reg_mod = SM->add_module_from_technology_library(name, register_type_name, library, circuit, HLS->HLS_T->get_technology_manager());
      this->specialise_reg(reg_mod, i);
      structural_objectRef port_selector = reg_mod->find_member(SELECTOR_REGISTER_FILE, port_o_K, reg_mod);
      SM->add_connection(selector_regFile_sign, port_selector);
      regis->set_structural_obj(reg_mod);
   }
}
