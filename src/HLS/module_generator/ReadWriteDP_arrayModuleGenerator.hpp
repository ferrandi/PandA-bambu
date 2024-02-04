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
 *              Copyright (C) 2022-2024 Politecnico di Milano
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
 * @file ReadWriteDP_arrayModuleGenerator.hpp
 * @brief
 *
 *
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _READ_WRITE_DPARRAY_MODULE_GENERATOR_HPP_
#define _READ_WRITE_DPARRAY_MODULE_GENERATOR_HPP_

#include "ModuleGenerator.hpp"

class ReadWriteDP_arrayModuleGenerator : public ModuleGenerator::Registrar<ReadWriteDP_arrayModuleGenerator>
{
 public:
   ReadWriteDP_arrayModuleGenerator(const HLS_managerRef& HLSMgr);

   void InternalExec(std::ostream& out, structural_objectRef mod, unsigned int function_id, vertex op_v,
                     const HDLWriter_Language language, const std::vector<ModuleGenerator::parameter>& _p,
                     const std::vector<ModuleGenerator::parameter>& _ports_in,
                     const std::vector<ModuleGenerator::parameter>& _ports_out,
                     const std::vector<ModuleGenerator::parameter>& _ports_inout) final;
};

#endif