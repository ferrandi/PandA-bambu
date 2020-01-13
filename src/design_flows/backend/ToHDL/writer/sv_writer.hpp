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
 * @file verilog_writer.hpp
 * @brief Class for system verilog writing. Currently only system verilog provided descriptions are managed.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef SYSTEM_VERILOG_WRITER_HPP
#define SYSTEM_VERILOG_WRITER_HPP

#include "verilog_writer.hpp"

class system_verilog_writer : public verilog_writer
{
 public:
   /**
    * Return the name of the language writer.
    */
   std::string get_name() const override
   {
      return "system_verilog";
   }
   /**
    * Return the filename extension associted with the verilog_writer.
    */
   std::string get_extension() const override
   {
      return ".sv";
   }

   /**
    * Write in the proper language the behavioral description of the module described in "Not Parsed" form.
    * @param cir is the component.
    */
   void write_NP_functionalities(const structural_objectRef& cir) override;

   /**
    * Constructor
    */
   explicit system_verilog_writer(const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~system_verilog_writer() override;
};

#endif
