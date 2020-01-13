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
 * @file language_writer.cpp
 * @brief This classes starting from a structural representation write different HDL based descriptions (VHDL, Verilog, SystemC).
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// constants include
#include "copyrights_strings.hpp"

#include "language_writer.hpp"

#include "VHDL_writer.hpp"
#include "sv_writer.hpp"
#include "verilog_writer.hpp"
#if HAVE_EXPERIMENTAL
#include "SystemC_writer.hpp"
#include "blif_writer.hpp"
#include "edif_writer.hpp"
#endif
#include "structural_objects.hpp"

#include "exceptions.hpp"

///. include
#include "Parameter.hpp"

/// utility include
#include "indented_output_stream.hpp"

language_writer::language_writer(char open_char, char close_char, const ParameterConstRef _parameters)
    : indented_output_stream(new IndentedOutputStream(open_char, close_char, 2)), parameters(_parameters), debug_level(_parameters->getOption<int>(OPT_debug_level))
{
}

language_writer::~language_writer() = default;

unsigned int language_writer::bitnumber(long long unsigned int n)
{
   unsigned int count = 0;
   while(n)
   {
      count++;
      n >>= 1;
   }
   if(count == 0)
   {
      return 1;
   }
   return count;
}

language_writerRef language_writer::create_writer(const HDLWriter_Language language, const technology_managerConstRef _TM, const ParameterConstRef _parameters)
{
   THROW_ASSERT(_parameters, "");
   switch(language)
   {
      case HDLWriter_Language::VERILOG:
         return language_writerRef(new verilog_writer(_parameters));
         break;
      case HDLWriter_Language::SYSTEM_VERILOG:
         return language_writerRef(new system_verilog_writer(_parameters));
         break;
      case HDLWriter_Language::VHDL:
         return language_writerRef(new VHDL_writer(_TM, _parameters));
         break;
#if HAVE_EXPERIMENTAL
      case HDLWriter_Language::SYSTEMC:
         return language_writerRef(new SystemC_writer(_parameters));
         break;
      case HDLWriter_Language::BLIF:
         return language_writerRef(new blif_writer(_parameters));
         break;
      case HDLWriter_Language::EDIF:
         return language_writerRef(new edif_writer(_parameters));
         break;
#endif
      default:
         THROW_ERROR("HDL backend language not supported");
   }
   return language_writerRef();
}

void language_writer::write(const std::string& rawString)
{
   indented_output_stream->Append(rawString);
}

void language_writer::write_header()
{
}

void language_writer::write_timing_specification(const technology_managerConstRef, const structural_objectRef&)
{
}

const std::string language_writer::WriteString() const
{
   return indented_output_stream->WriteString();
}

void language_writer::WriteFile(const std::string& filename) const
{
   indented_output_stream->WriteFile(filename);
}

CustomSet<std::string> language_writer::GetHDLReservedNames() const
{
   CustomSet<std::string> ret;
   ret.insert(RESET_PORT_NAME);
   ret.insert(CLOCK_PORT_NAME);
   ret.insert(DONE_PORT_NAME);
   ret.insert(START_PORT_NAME);
   ret.insert(WENABLE_PORT_NAME);
   ret.insert(RETURN_PORT_NAME);
   return ret;
}

COPYING3_SHORT_MACRO

void language_writer::WriteLicense()
{
   for(auto& row : COPYING3_SHORT)
   {
      write_comment(std::string(row));
   }
}
