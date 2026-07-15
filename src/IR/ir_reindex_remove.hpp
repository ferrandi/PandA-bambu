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
 *   Copyright (C) 2024-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file ir_reindex_remove.hpp
 * @brief IR reindex remove class
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef IR_REINDEX_REMOVE_HPP
#define IR_REINDEX_REMOVE_HPP

#include "ir_node_mask.hpp"
#include "refcount.hpp"

#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

class ir_manager;
REF_FORWARD_DECL(ir_node);

struct ir_reindex_remove : public ir_node_mask
{
   /**
    * @brief Construct a new IR node dup object
    *
    * @param TM the IR manager instance
    */
   ir_reindex_remove(const ir_manager& TM);

   /// ir_node visitors
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO_DECL, BOOST_PP_EMPTY, OBJ_SPECIALIZED_SEQ)
   BOOST_PP_SEQ_FOR_EACH(OPERATOR_MACRO, BOOST_PP_EMPTY, OBJ_NOT_SPECIALIZED_SEQ)

   void operator()(const ir_nodeRef& tn);

 private:
   const ir_manager& TM;
   ir_nodeRef source_tn;
   CustomUnorderedSet<unsigned int> already_visited;

   void fix_reference(ir_nodeRef& tn) const;
};

#endif
