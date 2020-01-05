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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file asn_type.hpp
 * @brief Data classes storing information for asn types
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef ASN_TYPE_HPP
#define ASN_TYPE_HPP

/// STD include
#include <string>

/// STL include
#include <list>

/// utility include
#include "refcount.hpp"

REF_FORWARD_DECL(AsnType);

enum class AsnType_Kind
{
   BOOLEAN,
   CHOICE,
   ENUMERATED,
   INTEGER,
   OCTET_STRING,
   REAL,
   REDEFINE,
   SEQUENCE,
   SEQUENCEOF,
   SET,
   SETOF
};

class AsnType
{
 protected:
   /// The actual kind
   const AsnType_Kind kind;

 public:
   /**
    * Constructor
    */
   explicit AsnType(const AsnType_Kind kind);

   /**
    * Destructor
    */
   virtual ~AsnType();

   /**
    * Return the type
    */
   AsnType_Kind GetKind();
};

class BooleanAsnType : public AsnType
{
 public:
   /**
    * Constructor
    */
   BooleanAsnType();
};

class ChoiceAsnType : public AsnType
{
 public:
   /// The list of name type
   const std::list<std::pair<std::string, AsnTypeRef>> element_type_list;

   /**
    * Constructor
    * @param element_type_list is the types composing this choice
    */
   explicit ChoiceAsnType(std::list<std::pair<std::string, AsnTypeRef>> element_type_list);
};

class EnumeratedAsnType : public AsnType
{
 public:
   /// The list of enum
   const std::list<std::pair<std::string, unsigned int>> named_number_list;

   /**
    * Constructor
    * @param the list of enum
    */
   explicit EnumeratedAsnType(std::list<std::pair<std::string, unsigned int>> named_number_list);
};

class IntegerAsnType : public AsnType
{
 public:
   /**
    * Constructor
    */
   IntegerAsnType();
};

class OctetStringAsnType : public AsnType
{
 public:
   /// The size
   size_t size;

   /**
    * Constructor
    */
   explicit OctetStringAsnType(const std::string& size);

   /**
    * Destructor
    */
   ~OctetStringAsnType() override;
};

class RealAsnType : public AsnType
{
 public:
   /**
    * Constructor
    */
   RealAsnType();
};

class RedefineAsnType : public AsnType
{
 public:
   /// The redefined type
   const std::string name;

   /**
    * Constructor
    * @param name is the redefined type
    */
   explicit RedefineAsnType(std::string name);
};

class SequenceAsnType : public AsnType
{
 public:
   /// The list of fields
   const std::list<std::pair<std::string, AsnTypeRef>> fields;

   /**
    * Constructor
    */
   explicit SequenceAsnType(std::list<std::pair<std::string, AsnTypeRef>> element_type_list);
};

class SequenceOfAsnType : public AsnType
{
 public:
   /// The type of the element
   std::string element;

   /// The number of elements in the sequence
   size_t size;

   /**
    * Constructor
    */
   SequenceOfAsnType(std::string element, const std::string& size);
};

class SetAsnType : public AsnType
{
 public:
   /// The list of fields
   const std::list<std::pair<std::string, AsnTypeRef>> fields;

   /**
    * Constructor
    */
   explicit SetAsnType(std::list<std::pair<std::string, AsnTypeRef>> element_type_list);
};

class SetOfAsnType : public AsnType
{
 public:
   /// The type of the element
   std::string element;

   /// The number of element in the set
   size_t size;

   /**
    * Constructor
    */
   SetOfAsnType(std::string element, const std::string& size);
};

typedef refcount<AsnType> AsnTypeRef;
typedef refcount<const AsnType> AsnTypeConstRef;
#endif
