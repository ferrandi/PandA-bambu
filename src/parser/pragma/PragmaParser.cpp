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
 * @file PragmaParser.cpp
 * @brief Parsing pragma from C sources.
 *
 * A object for retrieving information about pragma directives in a C/C++ program.
 *
 * @author Matteo Fioroni <matteofioroni@yahoo.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_HAVE_EXPERIMENTAL.hpp"

/// Header include
#include "PragmaParser.hpp"

/// Constants include
#include "pragma_constants.hpp"

/// Parameter include
#include "Parameter.hpp"

/// Pragma include
#include "pragma_manager.hpp"

/// Utility include
#include "boost/lexical_cast.hpp"
#include "boost/tokenizer.hpp"
#include "exceptions.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>
#include <fstream>

unsigned int PragmaParser::number = 0;

unsigned int PragmaParser::file_counter = 0;

// constructor
PragmaParser::PragmaParser(const pragma_managerRef _PM, const ParameterConstRef _Param) : PM(_PM), debug_level(_Param->get_class_debug_level(GET_CLASS(*this))), Param(_Param), level(0), search_function(false)
{
   THROW_ASSERT(PM, "Pragma manager not initialized");
}

// destructor
PragmaParser::~PragmaParser() = default;

std::string PragmaParser::substitutePragmas(const std::string& OldFile)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Substituting pragma in " + OldFile);
   THROW_ASSERT(boost::filesystem::exists(boost::filesystem::path(OldFile)), "Input file \"" + OldFile + "\" does not exist");

   boost::filesystem::path old_path(OldFile);
   std::string FileName = Param->getOption<std::string>(OPT_output_temporary_directory) + STR_CST_pragma_prefix + boost::lexical_cast<std::string>(file_counter) + "_" + GetLeafFileName(old_path);
   std::ofstream fileOutput(FileName.c_str(), std::ios::out);

   file_counter++;
   level = 0;
   // unsigned line_number = 0;

   // Get a stream from the input file
   std::ifstream instream(OldFile.c_str());
   // Test if the file has been correctly opened
   THROW_ASSERT(instream.is_open(), "INPUT FILE ERROR: Could not open input file: " + OldFile);
   while(!instream.eof())
   {
      std::string input_line;
      getline(instream, input_line);
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Read line <" + input_line + ">");
      std::string output_line = input_line;

      /// search for function name
      if(search_function)
      {
         std::string::size_type notwhite = output_line.find_first_of("(");
         if(notwhite != std::string::npos)
         {
            std::string Token = input_line;
            Token.erase(notwhite);
            name_function += Token;

            notwhite = name_function.find_last_not_of(" \t\r\n");
            name_function.erase(notwhite + 1);
            notwhite = name_function.find_last_of(" \t\n*>");
            name_function.erase(0, notwhite + 1);
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found function " + name_function);

            /// Pragma associated with called are added by pragma_analysis
            if(level == 0)
               PM->AddFunctionDefinitionPragmas(name_function, FunctionPragmas);

            name_function.clear();
            search_function = false;
            FunctionPragmas.clear();
         }
         else
            name_function += input_line + " ";
      }
      if(input_line.find("#pragma") != std::string::npos)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found a pragma");
         output_line = input_line;
         char const* delims = " \t\r\n";
         // trim leading whitespace
         std::string::size_type notwhite = output_line.find_first_not_of(delims);
         output_line.erase(0, notwhite);
         // trim trailing whitespace
         notwhite = output_line.find_last_not_of(delims);
         output_line.erase(notwhite + 1);

         analyze_pragma(output_line);
      }

      /// print out the new pragma line
      fileOutput << output_line << std::endl;

      /// manage nesting levels
      bool found = false;
      for(char i : input_line)
      {
         if(i == '{')
         {
            level++;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found {: Current level " + boost::lexical_cast<std::string>(level));
            if(!found)
            {
               for(auto& FloatingPragma : FloatingPragmas)
                  OpenPragmas[level].push_back(FloatingPragma);
               FloatingPragmas.clear();
            }
            found = true;
         }
         if(i == '}')
         {
            if(OpenPragmas.count(level))
            {
               for(auto& open_pragma : OpenPragmas[level])
                  fileOutput << std::string(STR_CST_pragma_function_end) + "(\"" << open_pragma << "\");" << std::endl;
               OpenPragmas[level].clear();
            }
            level--;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found }: Current level " + boost::lexical_cast<std::string>(level));
         }
      }

      /// increment line number
      // line_number++;
   }

   fileOutput.close();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Substituted pragma in " + OldFile);
   return FileName;
}

bool PragmaParser::analyze_pragma(std::string& Line)
{
   /// parallelism pragmas
   if(Line.find(STR_CST_pragma_keyword_omp) != std::string::npos)
   {
      if(!Param->getOption<bool>(OPT_ignore_parallelism))
      {
         return recognize_omp_pragma(Line);
      }
      else
      {
         return false;
      }
   }

   if(Line.find("call_hw") != std::string::npos)
   {
      /// mapping pragmas
      if(!Param->getOption<bool>(OPT_ignore_mapping) and !Param->getOption<bool>(OPT_mapping))
      {
         return recognize_mapping_pragma(Line);
      }
      else
      {
         return false;
      }
   }

   /// call_point_hw pragmas
   if(Line.find("call_point_hw") != std::string::npos)
      return recognize_call_point_hw_pragma(Line);

   /// issue pragmas
   if(Line.find("issue") != std::string::npos)
      return recognize_issue_pragma(Line);

   /// profiling pragmas
   if(Line.find("profiling") != std::string::npos)
      return recognize_profiling_pragma(Line);

   /// generate_hw pragmas
   if(Line.find("generate_hw") != std::string::npos)
   {
      Line.clear();
      return true;
   }

   /// generic pragmas
   return recognize_generic_pragma(Line);
}

bool PragmaParser::recognize_omp_pragma(std::string& line)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Looking for openmp pragma in " + line);
   std::string original_line = line;
   const pragma_manager::OmpPragmaType omp_pragma_type = pragma_manager::GetOmpPragmaType(line);
   if(omp_pragma_type == pragma_manager::OMP_UNKNOWN)
      THROW_ERROR("Unsupported openmp directive in line " + line);
   bool single_line_pragma = false;
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Found directive " + pragma_manager::omp_directive_keywords[omp_pragma_type]);
   switch(omp_pragma_type)
   {
      case(pragma_manager::OMP_ATOMIC):
      case(pragma_manager::OMP_FOR):
      case(pragma_manager::OMP_PARALLEL_FOR):
      case(pragma_manager::OMP_SIMD):
      case(pragma_manager::OMP_TARGET):
      {
         search_function = true;
         single_line_pragma = true;
         line = std::string(STR_CST_pragma_function_single_line_one_argument) + "(\"" STR_CST_pragma_keyword_omp "\", ";
         break;
      }
      case(pragma_manager::OMP_DECLARE_SIMD):
      {
         FunctionPragmas.insert(line);
         search_function = true;
         return true;
      }
      case(pragma_manager::OMP_BARRIER):
      case(pragma_manager::OMP_CRITICAL):
      case(pragma_manager::OMP_PARALLEL):
      case(pragma_manager::OMP_PARALLEL_SECTIONS):
      case(pragma_manager::OMP_SECTION):
      case(pragma_manager::OMP_SECTIONS):
      case(pragma_manager::OMP_TASK):
      {
         line = std::string(STR_CST_pragma_function_start) + "(\"" STR_CST_pragma_keyword_omp "\", ";
         break;
      }
      case(pragma_manager::OMP_UNKNOWN):
      {
         THROW_UNREACHABLE("Unsupported openmp directive in line " + line);
         break;
      }
      default:
      {
         THROW_UNREACHABLE("");
      }
   }
   const std::string omp_pragma_directive = pragma_manager::omp_directive_keywords[omp_pragma_type];
   line += "\"" + omp_pragma_directive + "\"";
#if 0
   std::string::size_type notwhite = original_line.find(omp_pragma_directive);
#endif
   original_line.erase(0, original_line.find(omp_pragma_directive) + omp_pragma_directive.size());
   if(not single_line_pragma)
   {
      FloatingPragmas.push_back(STR_CST_pragma_keyword_omp "\", \"" + omp_pragma_directive + (original_line.size() ? "\", \"" + original_line.substr(1, original_line.size() - 1) : ""));
   }

   if(original_line.size())
      line += ", \"" + original_line.substr(1, original_line.size() - 1) + "\"";
   line += ");";
   return true;
}

bool PragmaParser::recognize_call_point_hw_pragma(std::string& line) const
{
   const std::string old_line = line;
   line = std::string(STR_CST_pragma_function_single_line_two_arguments) + "(";
   line += "\"" + std::string(STR_CST_pragma_keyword_map) + "\"";
   line += ", ";
   std::vector<std::string> splitted = SplitString(old_line, " ");
   THROW_ASSERT(splitted.size() == 4 or splitted.size() == 5, "Error in syntax of mapping pragma: " + old_line);
   THROW_ASSERT(splitted[2] == std::string(STR_CST_pragma_keyword_call_point_hw), "Expecting " + std::string(STR_CST_pragma_keyword_call_point_hw) + " - Found : " + splitted[2]);
   line += "\"" + std::string(STR_CST_pragma_keyword_call_point_hw) + "\"";
   line += ", \"" + splitted[3] + "\"";
   if(splitted.size() == 5)
      line += ", \"" + splitted[4] + "\"";
   line += ");";
   return true;
}

bool PragmaParser::recognize_mapping_pragma(std::string& Line)
{
   if(level == 0)
   {
      FunctionPragmas.insert(Line);
      search_function = true;
      return true;
   }
   else
   {
      PM->setGenericPragma(number, Line);
      Line = "__pragma__" + boost::lexical_cast<std::string>(number) + "_();";
      number++;
      return false;
   }
}

bool PragmaParser::recognize_issue_pragma(std::string& Line)
{
   FunctionPragmas.insert(Line);
   search_function = true;
   return false;
}

bool PragmaParser::recognize_profiling_pragma(std::string& Line)
{
   FunctionPragmas.insert(Line);
   search_function = true;
   return false;
}

bool PragmaParser::recognize_generic_pragma(std::string& Line)
{
   // Line = "__pragma__(\"generic\", \"" + Line + "\");";
   if(level == 0)
   {
      FunctionPragmas.insert(Line);
      search_function = true;
   }
   else
   {
      PM->setGenericPragma(number, Line);
      Line = "__pragma__" + boost::lexical_cast<std::string>(number) + "_();";
      number++;
   }
   return false;
}
