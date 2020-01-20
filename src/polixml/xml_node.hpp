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
 * @file xml_node.hpp
 * @brief
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef XML_NODE_HPP
#define XML_NODE_HPP

#include "refcount.hpp"
#include "simple_indent.hpp"

#include <iosfwd>
#include <list>
#include <string>
#include <utility>

/// utility include
#include "custom_set.hpp"

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(xml_node);
class xml_element;
class xml_text_node;
class xml_comment_node;
class xml_att_decl_node;
//@}

#define XML_TAB_SIZE 2

class xml_node
{
 private:
   /// name of the node
   std::string name;

   /// The line number in the XML file (not unsigned because lineno function of the lexer returns an int)
   int line;

 public:
   /**
    * constructor
    */
   explicit xml_node(std::string _name) : name(std::move(_name)), line(0)
   {
   }
   /// destructor
   virtual ~xml_node()
   {
   }

   /// type for list of xml nodes
   typedef std::list<xml_nodeRef> node_list;

   /**
    * Print the class.
    * @param os is the stream.
    * @param formatted when true the xml is formatted in human readable way.
    * @param pp is the pretty print helper.
    */
   virtual void print(std::ostream& os, bool formatted, simple_indent* pp) const = 0;

   /**
    * Friend definition of the << operator. Reference version
    */
   friend std::ostream& operator<<(std::ostream& os, const xml_node& s)
   {
      simple_indent PP(STD_OPENING_CHAR, STD_CLOSING_CHAR, XML_TAB_SIZE);
      s.print(os, true, &PP);
      return os;
   }
   /**
    * Friend definition of the << operator. Pointer version
    */
   friend std::ostream& operator<<(std::ostream& os, const xml_node* s)
   {
      simple_indent PP(STD_OPENING_CHAR, STD_CLOSING_CHAR, XML_TAB_SIZE);
      s->print(os, true, &PP);
      return os;
   }
   /**
    * Friend definition of the << operator. Refcount version
    */
   friend std::ostream& operator<<(std::ostream& os, const xml_nodeRef& s)
   {
      simple_indent PP(STD_OPENING_CHAR, STD_CLOSING_CHAR, XML_TAB_SIZE);
      s->print(os, true, &PP);
      return os;
   }

   /**
    * Get the name of this node.
    * @returns The node's name.
    */
   std::string get_name() const
   {
      return name;
   }

   /**
    * Set the name of this node.
    * @param _name The new name for the node.
    */
   void set_name(const std::string& _name)
   {
      name = _name;
   }

   /** Discover at what line number this node occurs in the XML file.
    * @returns The line number.
    */
   int get_line() const;

   /**
    * Set the line this node occurs in the XML file.
    * @param _line The line number.
    */
   void set_line(int _line);

   /**
    * Convert unescaped characters. Return a reference to the converted string.
    * Supported characters are '&', '<', '>', "'", and '"'.
    * @param ioString is the converted string.
    */
   static void convert_unescaped(std::string& ioString)
   {
      std::string::size_type lPos = 0;
      while((lPos = ioString.find_first_of("&<>'\"", lPos)) != std::string::npos)
      {
         switch(ioString[lPos])
         {
            case '&':
               ioString.replace(lPos++, 1, "&amp;");
               break;
            case '<':
               ioString.replace(lPos++, 1, "&lt;");
               break;
            case '>':
               ioString.replace(lPos++, 1, "&gt;");
               break;
            case '\'':
               ioString.replace(lPos++, 1, "&apos;");
               break;
            case '"':
               ioString.replace(lPos++, 1, "&quot;");
               break;
            default:
            {
               // Do nothing
            }
         }
      }
   }
   /**
    * Convert escaped characters. Return a reference to the converted string.
    * Supported characters are '&', '<', '>', "'", and '"'.
    * Managed even strings with '\n' character
    * @param ioString is the converted string.
    */
   static void convert_escaped(std::string& ioString)
   {
      std::string::size_type lPos = 0;
      while((lPos = ioString.find("&amp;", lPos)) != std::string::npos)
         ioString.replace(lPos++, 5, "&");
      lPos = 0;
      while((lPos = ioString.find("&lt;", lPos)) != std::string::npos)
         ioString.replace(lPos++, 4, "<");
      lPos = 0;
      while((lPos = ioString.find("&gt;", lPos)) != std::string::npos)
         ioString.replace(lPos++, 4, ">");
      lPos = 0;
      while((lPos = ioString.find("&apos;", lPos)) != std::string::npos)
         ioString.replace(lPos++, 6, "\'");
      lPos = 0;
      while((lPos = ioString.find("&quot;", lPos)) != std::string::npos)
         ioString.replace(lPos++, 6, "\"");
   }
};

class xml_child : public xml_node
{
 public:
   /**
    * constructor
    */
   explicit xml_child(const std::string& _name) : xml_node(_name), first_text(nullptr)
   {
   }

   /**
    * Print the class.
    */
   void print(std::ostream& os, bool formatted, simple_indent* pp) const override
   {
      auto it_end = child_list.end();
      for(auto it = child_list.begin(); it != it_end; ++it)
      {
         (*it)->print(os, formatted, pp);
      }
   }

   /** Add a child element to this node
    * @param name The new node name
    * @returns The newly-created element
    */
   xml_element* add_child_element(const std::string& name);

   /** Add a child element to this node starting from a given node
    * @param node is the given datastructure to be added
    * @returns The newly-created element
    */
   xml_element* add_child_element(const xml_nodeRef& node);

   /** Append a new text node.
    * @param content The text. This should be unescaped - see ContentNode::set_content().
    * @returns The new text node.
    */
   xml_text_node* add_child_text(const std::string& content);

   /** Get the first child text content node.
    * This is a convenience method, meant as an alternative to iterating over all the child nodes to find the first suitable node then and getting the text directly.
    * @returns The first text node, if any.
    */
   xml_text_node* get_child_text()
   {
      return first_text;
   }

   /** Get the first child text content node.
    * This is a convenience method, meant as an alternative to iterating over all the child nodes to find the first suitable node then and getting the text directly.
    * @returns The first text node, if any.
    */
   const xml_text_node* get_child_text() const
   {
      return first_text;
   }

   /** Append a new comment node.
    * @param content The text. This should be unescaped - see xml_comment_node::set_content().
    * @returns The new comment node.
    */
   xml_comment_node* add_child_comment(const std::string& content);

   /** Append a new attribute declaration node.
    * @param name The name of the declaration.
    * @returns The new attribute declaration node.
    */
   xml_att_decl_node* add_child_attribute_declaration(const std::string& name);

   /** Remove the child node.
    * @param el The child node to remove. This Node will be deleted and therefore unusable after calling this method.
    */
   void remove_child(xml_node* el)
   {
      child_list.remove(xml_nodeRef(el, null_deleter()));
   }

   /** Obtain the list of child nodes.
    * @returns The list of child nodes.
    */
   node_list const& get_children()
   {
      return child_list;
   }

   /** Obtain the list of child nodes.
    * @returns The list of child nodes.
    */
   const node_list& get_children() const
   {
      return child_list;
   }

   /**
    * @returns Whether this node has children, or is empty.
    */
   bool has_child() const
   {
      return !child_list.empty();
   }

   /**
    * Return the set of descendants with a specific path
    * @param path is the / separated list of the tags to be crossed from the current node to the ancestor
    * @return the set of xml nodes
    */
   const CustomSet<xml_nodeRef> CGetDescendants(const std::string& path) const;

 private:
   node_list child_list;
   xml_text_node* first_text;
};

#endif
