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
 * @file structuralIO.cpp
 * @brief Input and output functions used to read and write the structural data structures.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * @warning This file is still in a work in progress state
 * @warning Last modified by $Author$
 *
 */
#include "structuralIO.hpp"

#include "Parameter.hpp"          // for ParameterRef
#include "structural_manager.hpp" // for structural_managerRef, str...
#include "structural_objects.hpp" // for structural_type_descriptorRef
#include "xml_document.hpp"       // for xml_document
#include "xml_dom_parser.hpp"     // for XMLDomParser
#include <iostream>               // for operator<<, endl, basic_os...
#include <string>                 // for operator<<, string, char_t...

structural_managerRef read_structural_File(const std::string& fn, const ParameterRef& Param)
{
   try
   {
      structural_managerRef CM;
      XMLDomParser parser(fn);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         CM = structural_managerRef(new structural_manager(Param));
         structural_type_descriptorRef build_type = structural_type_descriptorRef(new structural_type_descriptor("BUILD"));
         CM->set_top_info("BUILD", build_type);
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         structural_manager::xload(node, CM);
      }
      return CM;
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
   return structural_managerRef();
}

void write_structural_File(const std::string& f, structural_managerRef const& CM)
{
   try
   {
      xml_document document;
      xml_element* nodeRoot = document.create_root_node("circuit");
      CM->xwrite(nodeRoot);
      document.write_to_file_formatted(f);
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
