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
