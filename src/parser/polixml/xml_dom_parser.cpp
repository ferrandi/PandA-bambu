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
 * @file xml_dom_parser.cpp
 * @brief XML DOM parser.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#include "xml_dom_parser.hpp"

#include "fileIO.hpp"

#include <utility>

XMLDomParser::XMLDomParser(const std::string& _name, const std::string& string_to_be_parsed)
    : name(_name), to_be_parsed(string_to_be_parsed)

{
}

XMLDomParser::XMLDomParser(const std::string& filename) : name(filename), to_be_parsed(filename)
{
}

XMLDomParser::operator bool() const
{
   return doc != nullptr;
}

xml_documentRef XMLDomParser::get_document()
{
   return doc;
}

const xml_documentRef XMLDomParser::get_document() const
{
   return doc;
}
