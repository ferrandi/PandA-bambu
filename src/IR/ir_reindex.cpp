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
 * @file ir_reindex.cpp
 * @brief Class implementation of the ir_reindex support class.
 *
 * This class is used during the IR traversal to store the NODE_ID value.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "ir_reindex.hpp"

ir_reindex::ir_reindex(const unsigned int i, const ir_nodeRef& tn) : ir_node(i), actual_ir_node(tn)
{
}

void ir_reindex::print(std::ostream& os) const
{
   os << "@" << index;
}

void ir_reindex::visit(ir_node_visitor* const v) const
{
   unsigned int mask = ALL_VISIT;
   (*v)(this, mask);
   VISIT_MEMBER(mask, actual_ir_node, visit(v));
}

bool lt_ir_reindex::operator()(const ir_nodeRef& x, const ir_nodeRef& y) const
{
   return x->index < y->index;
}
