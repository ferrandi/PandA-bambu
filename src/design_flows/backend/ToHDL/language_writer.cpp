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
 * @file language_writer.cpp
 * @brief This classes starting from a structural representation write different HDL based descriptions (VHDL, Verilog,
 * SystemC).
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#include "language_writer.hpp"

#include "Parameter.hpp"
#include "VHDL_writer.hpp"
#include "copyrights_strings.hpp"
#include "exceptions.hpp"
#include "indented_output_stream.hpp"
#include "structural_objects.hpp"
#include "sv_writer.hpp"
#include "verilog_writer.hpp"

language_writer::language_writer(char open_char, char close_char, const ParameterConstRef _parameters)
    : indented_output_stream(new IndentedOutputStream(open_char, close_char, 2)),
      parameters(_parameters),
      debug_level(_parameters->getOption<int>(OPT_debug_level))
{
}

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

language_writerRef language_writer::create_writer(const HDLWriter_Language language,
                                                  const technology_managerConstRef _TM,
                                                  const ParameterConstRef _parameters)
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
      default:
         THROW_ERROR("HDL backend language not supported");
   }
   return language_writerRef();
}

void language_writer::write(const std::string& rawString)
{
   indented_output_stream->Append(rawString);
}

void language_writer::write_header(bool)
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

const CustomSet<std::string>& language_writer::GetHDLReservedNames()
{
   static CustomSet<std::string> ret = {RESET_PORT_NAME, CLOCK_PORT_NAME,   DONE_PORT_NAME,  IDLE_PORT_NAME,
                                        START_PORT_NAME, WENABLE_PORT_NAME, RETURN_PORT_NAME};
   return ret;
}

MIT_LICENSE_SHORT_MACRO

void language_writer::WriteLicense(bool is_library)
{
   if(is_library)
   {
      for(auto& row : MIT_LICENSE_SHORT)
      {
         write_comment(std::string(row));
      }
   }
}
