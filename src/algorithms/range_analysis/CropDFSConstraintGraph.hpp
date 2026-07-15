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
 *   Copyright (C) 2019-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file CropDFSConstraintGraph.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _RANGE_ANALYSIS_CROPDFS_CONSTRAINT_GRAPH_HPP_
#define _RANGE_ANALYSIS_CROPDFS_CONSTRAINT_GRAPH_HPP_
#include "ConstraintGraph.hpp"

class CropDFSConstraintGraph : public ConstraintGraph
{
   void preUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& entryPoints) override;

   void posUpdate(const UseMap& compUseMap, std::set<VarNode::key_type, VarNode::key_compare>& /*activeVars*/,
                  const CustomSet<VarNode*>& component) override;

 public:
   CropDFSConstraintGraph(application_managerRef _AppM, int _debug_level, int _graph_debug);
};

#endif // _RANGE_ANALYSIS_CROPDFS_CONSTRAINT_GRAPH_HPP_
