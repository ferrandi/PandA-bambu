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
 * @file xml_dom_parser.hpp
 * @brief XML DOM parser.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef XML_DOM_PARSER_HPP
#define XML_DOM_PARSER_HPP

/// STD include
#include <string>

/// utility includes
#include "fileIO.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(xml_dom_parser);
REF_FORWARD_DECL(xml_document);
//@}

/**
 * XML DOM parser.
 */
class XMLDomParser
{
 protected:
   /// The name of the parsed file or of the string
   const std::string name;

   /// The string to be parsed or the name for the file to be parsed
   const std::string to_be_parsed;

   /// The data structure extracted from the file
   xml_documentRef doc;

 public:
   /**
    * Constructor from string
    * @param name is the name of the string
    * @param string_to_be_parsed is the string to be parsed
    */
   XMLDomParser(const std::string& name, const std::string& string_to_be_parsed);

   /**
    * Constructor from file
    * @param filename is the file to be parsed
    */
   explicit XMLDomParser(const std::string& filename);

   /** Parse an XML document from a file.
    * @param filename The path to the file.
    */
   void Exec();

   /** Test whether a document has been parsed.
    */
   operator bool() const;

   /** Obtain the parsed document.
    * @return the xml document pointer.
    */
   xml_documentRef get_document();

   /** Obtain the parsed document. Const version.
    * @return the xml document pointer.
    */
   const xml_documentRef get_document() const;
};
typedef refcount<XMLDomParser> XMLDomParserRef;
#endif
