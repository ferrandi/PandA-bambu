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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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

#include "fileIO.hpp"
#include "refcount.hpp"

#include <string>

REF_FORWARD_DECL(xml_dom_parser);
REF_FORWARD_DECL(xml_document);

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

   /** Parse an XML document from a file. */
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
using XMLDomParserRef = refcount<XMLDomParser>;
#endif
