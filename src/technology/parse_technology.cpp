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
 * @file parse_technology.cpp
 * @brief Implementation of the technology parsing interface function.
 *
 * Implementation of the functions that parse the technology information from files.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 *
 */

/// Autoheader include
#include "config_HAVE_BOOLEAN_PARSER_BUILT.hpp"
#include "config_HAVE_FROM_LIBERTY.hpp"

#include "parse_technology.hpp"

#if HAVE_FROM_LIBERTY
#include "lib2xml.hpp"
#endif
#if HAVE_EXPERIMENTAL
#include "lef2xml.hpp"
#endif

#if HAVE_BOOLEAN_PARSER_BUILT
#include "dump_genlib.hpp"
#endif

#include "library_manager.hpp"
#include "technology_manager.hpp"
#include "technology_node.hpp"

#include "exceptions.hpp"
#include "fileIO.hpp"
#include "polixml.hpp"
#include "xml_dom_parser.hpp"

#include <iosfwd>
#include <string>

#include "simple_indent.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>

#include "Parameter.hpp"
#include "cpu_time.hpp"
#include "utility.hpp"

/// STL include
#include <vector>

/// utility include
#include "string_manipulation.hpp"

void read_technology_File(const std::string& fn, const technology_managerRef& TM, const ParameterConstRef& Param, const target_deviceRef& device)
{
   try
   {
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         TM->xload(node, device);

         std::vector<std::string> input_libraries;
         if(Param->isOption(OPT_input_libraries))
         {
            auto input_libs = Param->getOption<std::string>(OPT_input_libraries);
            input_libraries = convert_string_to_vector<std::string>(input_libs, ";");
         }
         const std::vector<std::string>& libraries = TM->get_library_list();
         for(const auto& librarie : libraries)
         {
            if(WORK_LIBRARY == librarie or DESIGN == librarie or PROXY_LIBRARY == librarie)
            {
               continue;
            }
            if(std::find(input_libraries.begin(), input_libraries.end(), librarie) == input_libraries.end())
            {
               input_libraries.push_back(librarie);
            }
         }
         /// FIXME: setting paraemeters
         const_cast<Parameter*>(Param.get())->setOption(OPT_input_libraries, convert_vector_to_string<std::string>(input_libraries, ";"));
      }
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error during technology file parsing: " + std::string(msg));
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error during technology file parsing: " + msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error during technology file parsing: " + std::string(ex.what()));
   }
   catch(...)
   {
      THROW_ERROR("Error during technology file parsing");
   }
}

void read_technology_library(const technology_managerRef& TM, const ParameterConstRef& Param, const target_deviceRef& device)
{
#if HAVE_EXPERIMENTAL || HAVE_FROM_LIBERTY || HAVE_LIBRARY_COMPILER
   int output_level = Param->getOption<int>(OPT_output_level);
#endif
#ifndef NDEBUG
   int debug_level = Param->get_class_debug_level("parse_technology");
#endif

#if HAVE_FROM_LIBERTY
   // parsing of liberty format
   if(Param->isOption(OPT_input_liberty_library_file))
   {
      long lib2xmlTime;
      std::string LibFile = Param->getOption<std::string>(OPT_input_liberty_library_file);

      std::string XmlList;
      std::vector<std::string> SplittedLibs = SplitString(LibFile, ",");
      for(unsigned int i = 0; i < SplittedLibs.size(); i++)
      {
         if(SplittedLibs.size() == 0)
            continue;
         boost::trim(SplittedLibs[i]);
         std::vector<std::string> SplittedLib = SplitString(SplittedLibs[i], ":");
         std::string LibraryFile;
         if(SplittedLib.size() < 1 or SplittedLib.size() > 2)
            THROW_ERROR("Malformed input liberty description: \"" + SplittedLibs[i] + "\"");
         if(SplittedLib.size() == 1)
         {
            LibraryFile = SplittedLib[0];
         }
         else
         {
            LibraryFile = SplittedLib[1];
         }
         if(!boost::filesystem::exists(LibraryFile))
            THROW_ERROR("Liberty file \"" + LibraryFile + "\" does not exists!");
         START_TIME(lib2xmlTime);
         std::string TargetXML = Param->getOption<std::string>(OPT_output_temporary_directory) + "/library_" + boost::lexical_cast<std::string>(i) + ".xml";
         lib2xml(LibraryFile, TargetXML, Param);
         if(XmlList.size())
            XmlList += ";";
         XmlList += TargetXML;
         STOP_TIME(lib2xmlTime);
         PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(koala) Read and converted the liberty file \"" + LibraryFile + "\" in " + boost::lexical_cast<std::string>(print_cpu_time(lib2xmlTime)) + " seconds;\n");
      }
      /// FIXME: setting parameters
      const_cast<Parameter*>(Param.get())->setOption("input_xml_library_file", XmlList);
   }
#endif

#if HAVE_BOOLEAN_PARSER_BUILT
   if(Param->isOption("input_genlib_library_file"))
   {
      long genlibTime;
      START_TIME(genlibTime);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "(koala) Reading the genlib technology library");
      std::string LibraryName = Param->getOption<std::string>("genlib_library_file");
      technology_managerRef local_TM = technology_managerRef(new technology_manager(Param));
      read_genlib_technology_File(LibraryName, local_TM, Param);
      STOP_TIME(genlibTime);
      PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(koala) Read the technology library file \"" + LibraryName + "\" in " + boost::lexical_cast<std::string>(print_cpu_time(genlibTime)) + " seconds;\n");
      write_technology_File(technology_manager::XML, "genlib", local_TM, device->get_type());
      /// FIXME: setting parameters
      const_cast<Parameter*>(Param.get())->setOption("input_xml_library_file", "genlib.xml");
      const_cast<Parameter*>(Param.get())->removeOption("input_genlib_library_file");
   }
#endif

   if(Param->isOption("input_xml_library_file"))
   {
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "(koala) Reading the XML technology library");

      std::string LibraryName;
      auto XmlLibraryList = Param->getOption<std::string>("input_xml_library_file");
      std::vector<std::string> SplittedLibs = SplitString(XmlLibraryList, ";");
      for(unsigned int i = 0; i < SplittedLibs.size(); i++)
      {
         if(SplittedLibs.empty())
         {
            continue;
         }
         LibraryName = SplittedLibs[i];
         long xmlTime;
         START_TIME(xmlTime);
         read_technology_File(SplittedLibs[i], TM, Param, device);
         STOP_TIME(xmlTime);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "(koala) Read the XML technology library file \"" + LibraryName + "\" in " + boost::lexical_cast<std::string>(print_cpu_time(xmlTime)) + " seconds;\n");
      }

#if HAVE_BOOLEAN_PARSER_BUILT
      if(Param->getOption<bool>(OPT_dump_genlib))
      {
#if HAVE_CIRCUIT_BUILT
         std::string genlib_name(LibraryName.substr(0, LibraryName.find_last_of('.')) + ".genlib");
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "(koala) Dumping the technology library in genlib format");
         dump_genlib(genlib_name, TM);
#endif
      }
#endif
   }

#if HAVE_EXPERIMENTAL
   if(Param->isOption("input_lef_library_file"))
   {
      std::string FileList = Param->getOption<std::string>("input_lef_library_file");
      std::vector<std::string> SplittedLibs = SplitString(FileList, ";");
      for(unsigned int i = 0; i < SplittedLibs.size(); i++)
      {
         if(SplittedLibs.size() == 0)
            continue;
         boost::trim(SplittedLibs[i]);

         std::vector<std::string> SplittedName = SplitString(SplittedLibs[i], ":");

         if(SplittedName.size() != 2)
            THROW_ERROR("Malformed LEF description: \"" + SplittedLibs[i] + "\"");
         std::string LibraryName = SplittedName[0];
         std::string LefFileName = SplittedName[1];

         if(!boost::filesystem::exists(LefFileName))
            THROW_ERROR("Lef file \"" + LefFileName + "\" does not exists!");

         if(!TM->is_library_manager(LibraryName))
            THROW_ERROR("Library \"" + LibraryName + "\" is not contained into the datastructure");
         const library_managerRef& LM = TM->get_library_manager(LibraryName);
         LM->set_info(library_manager::LEF, LefFileName);

         PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(koala) Stored LEF file \"" + LefFileName + "\" for library \"" + LibraryName + "\";\n");
      }
   }
#endif

#if HAVE_LIBRARY_COMPILER
   if(Param->isOption("input_db_library_file"))
   {
      std::string FileList = Param->getOption<std::string>("input_db_library_file");
      std::vector<std::string> SplittedLibs = SplitString(FileList, ";");
      for(unsigned int i = 0; i < SplittedLibs.size(); i++)
      {
         if(SplittedLibs.size() == 0)
            continue;
         boost::trim(SplittedLibs[i]);

         std::vector<std::string> SplittedName = SplitString(SplittedLibs[i], ":");

         if(SplittedName.size() != 2)
            THROW_ERROR("Malformed db description: \"" + SplittedLibs[i] + "\"");
         std::string LibraryName = SplittedName[0];
         std::string dbFileName = SplittedName[1];

         if(!TM->is_library_manager(LibraryName))
            THROW_ERROR("Library \"" + LibraryName + "\" is not contained into the datastructure");
         const library_managerRef LM = TM->get_library_manager(LibraryName);
         LM->set_info(library_manager::DB, dbFileName);

         PRINT_OUT_MEX(OUTPUT_LEVEL_MINIMUM, output_level, "(koala) Stored DB file \"" + dbFileName + "\" for library \"" + LibraryName + "\";\n");
      }
   }
#endif
}

#if HAVE_BOOLEAN_PARSER_BUILT
void read_genlib_technology_File(const std::string& fn, const technology_managerRef TM, const ParameterConstRef Param)
{
   try
   {
      boost::filesystem::path sourceLib(fn);
      if(!boost::filesystem::exists(sourceLib))
         THROW_ERROR("Library \"" + fn + "\" cannot be found");
      std::string Name = GetLeafFileName(sourceLib);

      fileIO_istreamRef sname = fileIO_istream_open(fn);
      technology_manager::gload(Name, sname, TM, Param);
   }
   catch(const char* msg)
   {
      THROW_ERROR("Error during technology file parsing: " + std::string(msg));
   }
   catch(const std::string& msg)
   {
      THROW_ERROR("Error during technology file parsing: " + msg);
   }
   catch(const std::exception& ex)
   {
      THROW_ERROR("Error during technology file parsing: " + std::string(ex.what()));
   }
   catch(...)
   {
      THROW_ERROR("Error during technology file parsing");
   }
}
#endif

void write_technology_File(unsigned int type,
                           const std::string&
#if HAVE_EXPERIMENTAL || HAVE_FROM_LIBERTY
                               f
#endif
                           ,
                           const technology_managerRef&
#if HAVE_EXPERIMENTAL || HAVE_FROM_LIBERTY
                               TM
#endif
                           ,
                           TargetDevice_Type
#if HAVE_EXPERIMENTAL
                               dv_type
#endif
                           ,
                           const CustomOrderedSet<std::string>&
#if HAVE_EXPERIMENTAL || HAVE_FROM_LIBERTY
                               libraries
#endif
)
{
   if((type & technology_manager::XML) != 0)
   {
      THROW_UNREACHABLE("Unexpected case");
#if HAVE_EXPERIMENTAL
      write_lef_technology_File(f + ".lef", TM, dv_type, libraries);
#endif
   }
#if HAVE_FROM_LIBERTY
   if((type & technology_manager::LIB) != 0)
   {
      write_lib_technology_File(f + ".lib", TM, libraries);
   }
#endif
#if HAVE_EXPERIMENTAL
   if((type & technology_manager::LEF) != 0)
   {
      write_lef_technology_File(f + ".lef", TM, dv_type, libraries);
   }
#endif
}

void write_technology_File(unsigned int type, const std::string& f, library_manager* LM, TargetDevice_Type dv_type)
{
   if((type & technology_manager::XML) != 0)
   {
      write_xml_technology_File(f + ".xml", LM, dv_type);
   }
#if HAVE_FROM_LIBERTY
   if((type & technology_manager::LIB) != 0)
   {
      write_lib_technology_File(f + ".lib", LM, dv_type);
   }
#endif
#if HAVE_EXPERIMENTAL
   if((type & technology_manager::LEF) != 0)
   {
      write_lef_technology_File(f + ".lef", LM, dv_type);
   }
#endif
}

void write_xml_technology_File(const std::string& f, library_manager* LM, TargetDevice_Type dv_type)
{
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("technology");
      LM->xwrite(nodeRoot, dv_type);
      document.write_to_file_formatted(f);
      LM->set_info(library_manager::XML, f);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}

#if HAVE_FROM_LIBERTY
void write_lib_technology_File(const std::string& f, technology_managerRef const& TM, const CustomOrderedSet<std::string>& libraries)
{
   try
   {
      size_t count_cells = 0;
      for(CustomOrderedSet<std::string>::const_iterator n = libraries.begin(); n != libraries.end(); ++n)
      {
         if(WORK_LIBRARY == *n or DESIGN == *n or PROXY_LIBRARY == *n)
            continue;
         count_cells += TM->get_library_count(*n);
      }
      if(!count_cells)
      {
         THROW_WARNING("Specified libraries do not contain any cell");
         return;
      }
#if HAVE_CMOS_BUILT
      TM->lib_write(f, TargetDevice_Type::IC, libraries);
#endif
      return;
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   THROW_ERROR("Error while converting file " + f + " into XML");
}

void write_lib_technology_File(const std::string& f, library_manager* LM, TargetDevice_Type dv_type)
{
   write_xml_technology_File("__xml_library__.xml", LM, dv_type);
   xml2lib("__xml_library__.xml", f, 0, 0);
   boost::filesystem::remove("__xml_library__.xml");
   LM->set_info(library_manager::LIBERTY, f);
}
#endif

#if HAVE_EXPERIMENTAL
void write_lef_technology_File(const std::string& f, technology_managerRef const& TM, TargetDevice_Type dv_type, const CustomOrderedSet<std::string>& libraries)
{
   try
   {
      size_t count_cells = 0;
      for(CustomOrderedSet<std::string>::const_iterator n = libraries.begin(); n != libraries.end(); ++n)
      {
         if(WORK_LIBRARY == *n)
            continue;
         count_cells += TM->get_library_count(*n);
      }
      if(!count_cells)
      {
         THROW_WARNING("Specified libraries do not contain any cell");
         return;
      }
      TM->lef_write(f, dv_type, libraries);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
}

void write_lef_technology_File(const std::string& f, library_manager* LM, TargetDevice_Type dv_type)
{
   write_xml_technology_File("__library__.xml", LM, dv_type);
   xml2lef("__library__.xml", f, 0, 0);
   boost::filesystem::remove("__library__.xml");
   LM->set_info(library_manager::LEF, f);
}
#endif
