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
 * @file clique_covering.cpp
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 */

/// Header include
#include "clique_covering.hpp"
const std::string CliqueCovering_AlgorithmToString(const CliqueCovering_Algorithm clique_covering_algorithm)
{
   switch(clique_covering_algorithm)
   {
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING_FAST:
         return "TTT_FAST";
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING_FAST2:
         return "TTT_FAST2";
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING:
         return "TTT_FULL";
      case CliqueCovering_Algorithm::TTT_CLIQUE_COVERING2:
         return "TTT_FULL2";
      case CliqueCovering_Algorithm::TS_CLIQUE_COVERING:
         return "TS";
      case CliqueCovering_Algorithm::TS_WEIGHTED_CLIQUE_COVERING:
         return "WEIGHTED_TS";
      case(CliqueCovering_Algorithm::COLORING):
         return "COLORING";
      case CliqueCovering_Algorithm::WEIGHTED_COLORING:
         return "WEIGHTED_COLORING";
      case CliqueCovering_Algorithm::BIPARTITE_MATCHING:
         return "BIPARTITE_MATCHING";
#if HAVE_EXPERIMENTAL
      case CliqueCovering_Algorithm::RANDOMIZED:
         return "RANDOMIZED";
#endif
      default:
         THROW_UNREACHABLE("");
   }
   return "";
}
