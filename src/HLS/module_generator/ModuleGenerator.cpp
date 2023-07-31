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
 *              Copyright (C) 2022-2023 Politecnico di Milano
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
 * @file ModuleGenerator.cpp
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

#include "ModuleGenerator.hpp"

#include "structural_objects.hpp"

ModuleGenerator::parameter::parameter(const structural_objectRef& port)
    : name(port->get_id()),
      type(port->get_typeRef()->get_name()),
      type_size(port->get_typeRef()->vector_size != 0 ? port->get_typeRef()->vector_size : port->get_typeRef()->size),
      alignment(GetPointerS<const port_o>(port)->get_port_alignment())
{
}

void ModuleGenerator::Exec(std::ostream& out, structural_objectRef _mod, unsigned int function_id, vertex op_v,
                           const std::vector<parameter>& _p, const HDLWriter_Language language)
{
   const auto mod = GetPointer<module>(_mod);
   std::vector<parameter> _ports_in(mod->get_in_port_size(), parameter());
   for(auto i = 0U; i < _ports_in.size(); ++i)
   {
      _ports_in[i] = parameter(mod->get_in_port(i));
   }
   std::vector<parameter> _ports_out(mod->get_out_port_size(), parameter());
   for(auto i = 0U; i < _ports_out.size(); ++i)
   {
      _ports_out[i] = parameter(mod->get_out_port(i));
   }
   std::vector<parameter> _ports_inout(mod->get_in_out_port_size(), parameter());
   for(auto i = 0U; i < _ports_inout.size(); ++i)
   {
      _ports_inout[i] = parameter(mod->get_in_out_port(i));
   }
   InternalExec(out, _mod, function_id, op_v, language, _p, _ports_in, _ports_out, _ports_inout);
}
