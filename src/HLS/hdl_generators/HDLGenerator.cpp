/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2022-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file HDLGenerator.cpp
 * @brief Implementation of the common HDL generator utilities.
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */

#include "HDLGenerator.hpp"
#include "HDL_manager.hpp"

#include "structural_manager.hpp"
#include "structural_objects.hpp"

HDLGenerator::parameter::parameter(const structural_objectRef& port)
    : name(HDL_manager::convert_to_identifier(port->get_id())),
      type(port->get_typeRef()->get_name()),
      type_size(port->get_typeRef()->vector_size != 0 ? port->get_typeRef()->vector_size : port->get_typeRef()->size),
      alignment(GetPointerS<const port_o>(port)->get_port_alignment())
{
}

void HDLGenerator::Exec(std::ostream& out, structural_objectRef _mod, unsigned int function_id,
                        gc_vertex_descriptor op_v, const std::vector<parameter>& _p, const HDLWriter_Language language)
{
   const auto mod = GetPointer<module_o>(_mod);
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

std::string HDLGenerator::add_port(const structural_objectRef& circuit, const std::string& port_name, int dir,
                                   unsigned port_size, bool parametric) const
{
   structural_manager::add_port(port_name, static_cast<enum port_o::port_direction>(dir), circuit,
                                structural_type_descriptorRef(new structural_type_descriptor("bool", port_size)));
   if(parametric)
   {
      structural_manager::append_NP_functionality(circuit, NP_functionality::LIBRARY, port_name);
   }
   return HDL_manager::convert_to_identifier(port_name);
}
