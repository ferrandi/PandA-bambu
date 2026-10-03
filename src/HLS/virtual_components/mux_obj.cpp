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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file mux_obj.hpp
 * @brief Base class for multiplexer into datapath
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "mux_obj.hpp"

mux_obj::mux_obj(const generic_objRef _first, const generic_objRef _second, unsigned int _level,
                 const std::string& _name, const generic_objRef overall_target)
    : generic_obj(CONNECTION_ELEMENT, _name),
      bitsize(0),
      first(_first),
      second(_second),
      tree_target(overall_target),
      level(_level)
{
}

void mux_obj::set_target(const generic_objRef tgt)
{
   target = tgt;
}

generic_objRef mux_obj::get_final_target()
{
   return tree_target;
}

generic_objRef mux_obj::GetSelector() const
{
   return selector;
}

void mux_obj::set_selector(const generic_objRef sel)
{
   selector = sel;
}

unsigned int mux_obj::get_level() const
{
   return level;
}
unsigned int mux_obj::get_bitsize() const
{
   return bitsize;
}
