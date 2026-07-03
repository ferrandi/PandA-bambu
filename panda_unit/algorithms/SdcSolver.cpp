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
 *                Copyright (C) 2026 Politecnico di Milano
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License with BAMBU exceptions, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   A copy of the License can be found in the root directory of this repository.
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file SdcSolver.cpp
 * @brief Unit tests for the system of difference constraints solver.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#include "sdc_solver.hpp"

#include <boost/test/unit_test.hpp>

namespace
{
   void check_constraint(const std::map<unsigned int, int>& vals, unsigned int i, unsigned int j, int weight)
   {
      BOOST_REQUIRE(vals.find(i) != vals.end());
      BOOST_REQUIRE(vals.find(j) != vals.end());
      BOOST_CHECK_LE(vals.at(j) - vals.at(i), weight);
   }
} // namespace

BOOST_AUTO_TEST_CASE(sdc_solver_feasible_constraints)
{
   sdc_solver solver;
   solver.add_constraint(0, 1, 3);
   solver.add_constraint(1, 2, -1);
   solver.add_constraint(0, 2, 4);

   std::map<unsigned int, int> vals;
   BOOST_REQUIRE(solver.solve_SDC(vals));

   check_constraint(vals, 0, 1, 3);
   check_constraint(vals, 1, 2, -1);
   check_constraint(vals, 0, 2, 4);
}

BOOST_AUTO_TEST_CASE(sdc_solver_negates_solution)
{
   sdc_solver solver;
   solver.add_constraint(0, 1, -2);
   solver.add_constraint(1, 2, 5);

   std::map<unsigned int, int> vals;
   std::map<unsigned int, int> negated_vals;
   BOOST_REQUIRE(solver.solve_SDC(vals));
   BOOST_REQUIRE(solver.solve_SDCNeg(negated_vals));

   BOOST_REQUIRE_EQUAL(vals.size(), negated_vals.size());
   for(const auto& val : vals)
   {
      BOOST_REQUIRE(negated_vals.find(val.first) != negated_vals.end());
      BOOST_CHECK_EQUAL(negated_vals.at(val.first), -val.second);
   }
}

BOOST_AUTO_TEST_CASE(sdc_solver_detects_negative_cycle)
{
   sdc_solver solver;
   solver.add_constraint(0, 1, -1);
   solver.add_constraint(1, 0, -1);

   std::map<unsigned int, int> vals;
   BOOST_CHECK(!solver.solve_SDC(vals));
}

BOOST_AUTO_TEST_CASE(sdc_solver_preserves_sparse_variable_ids)
{
   sdc_solver solver;
   solver.add_constraint(2, 4, 7);

   std::map<unsigned int, int> vals;
   BOOST_REQUIRE(solver.solve_SDC(vals));

   BOOST_REQUIRE_EQUAL(vals.size(), 5);
   BOOST_CHECK_EQUAL(vals.at(0), 0);
   BOOST_CHECK_EQUAL(vals.at(1), 0);
   BOOST_CHECK_EQUAL(vals.at(3), 0);
   check_constraint(vals, 2, 4, 7);
}
