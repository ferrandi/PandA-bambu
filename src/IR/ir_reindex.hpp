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
 * @file ir_reindex.hpp
 * @brief Class specification of the ir_reindex support class.
 *
 * This class is used during the IR traversal to store the NODE_ID value.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef IR_REINDEX_HPP
#define IR_REINDEX_HPP

#include <ir_node.hpp>

/**
This class is used to perform the re-index of all IR nodes.
Each reference to an ir_node is implemented by this type of ir_node
*/
class ir_reindex : public ir_node
{
 private:
   friend class ir_manager;

   /**
    * Private constructor with index initialization
    * It can be accesses only by ir_manager
    * @param ind is the value of the index member.
    * @param tn is the actual reference to the ir_node.
    * NOTE: this has to be a reference since ir_nodeRef at the moment of the construction of this can not ready
    */
   ir_reindex(const unsigned int ind, const ir_nodeRef& tn);

 public:
   /**
    * Represent the actual reference to the ir_node.
    * NOTE: this has to be a reference since ir_nodeRef at the moment of the construction of this can not ready
    */
   const ir_nodeRef& actual_ir_node;

   /**
    * function that prints the class ir_reindex.
    */
   void print(std::ostream& os) const;
   /**
    * Redefinition of get_kind_text.
    */
   GET_KIND_TEXT(ir_reindex)

   /**
    * Redefinition of get_kind.
    */
   GET_KIND(ir_reindex)

   /**
    * virtual function used to traverse the ir_node data structure.
    * @param v is a reference to the ir_node visitor class
    */
   void visit(ir_node_visitor* const v) const override;
   /// visitor enum
   enum
   {
      GETID(ir_node) = 0,
      GETID(actual_ir_node)
   };
};

/// functor used to correctly compare two ir_reindex
struct lt_ir_reindex
{
   /// operator() used to compare two ir_reindex
   bool operator()(const ir_nodeRef& x, const ir_nodeRef& y) const;
};

#endif
