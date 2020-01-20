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
 * @file Statistics.hpp
 * @brief This file collects some algebric statistical functions.
 *
 *
 * @author Carmine Galeone
 * $Revision$
 * $Date$
 *
 */
#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/normal.hpp>
/// Utility include
#include <boost/version.hpp>

#if BOOST_VERSION >= 104600
#else
#include <tuple>

#define BOOST_TR1_TUPLE_HPP_INCLUDED
#endif
#include <boost/math/distributions.hpp>
#include <vector>

/**
 * @return return a boost::math::normal random variable multiplied for a constant.
 * @param coeff is the constant.
 * @param var is the boost::math::normal random variable.
 */
boost::math::normal MultiplyVarForCoefficient(int coeff, boost::math::normal var);

/**
 * @return return the sum of a boost::math::normal random variable and a constant.
 * @param x is the random variable.
 * @param c is the constant.
 */
boost::math::normal VarSum(boost::math::normal x, double c);

/**
 * @return return the sum of two independent boost::math::normal random variables.
 * @param x is the first random variable.
 * @param y is the second random variable.
 */
boost::math::normal VarSum(boost::math::normal x, boost::math::normal y);

/**
 * @return return the sum of two boost::math::normal random variables.
 * @param x is the first random variable.
 * @param y is the second random variable.
 * @param p is the correlation between x and y.
 */
boost::math::normal VarSum(boost::math::normal x, boost::math::normal y, double p);

/**
 * @return return the sum of all the independent boost::math::normal random variables contained in a vector.
 * @param v is the list of random variables.
 */
boost::math::lognormal VarSum(std::vector<boost::math::lognormal> v);

/**
 * @return return the sum of two independent lognormal random variables.
 * @param x is the first random variable.
 * @param y is the second random variable.
 */
boost::math::lognormal VarSum(boost::math::lognormal x, boost::math::lognormal y);

/**
 * @return return the sum of two lognormal random variables.
 * @param x is the first random variable.
 * @param y is the second random variable.
 * @param p is the correlation between x and y.
 */
boost::math::lognormal VarSum(boost::math::lognormal x, boost::math::lognormal y, double p);

/**
 * @return return the max of two boost::math::normal random variables.
 * @param x is the first random variable.
 * @param y is the second random variable.
 * @param p is the correlation between x and y.
 */
boost::math::normal VarMax(boost::math::normal x, boost::math::normal y, double p);

/**
 * @return return the max of all the independent boost::math::normal random variables contained in a vector.
 * @param v is the list of random variables.
 */
boost::math::normal VarMax(std::vector<boost::math::normal> v);

/**
 * @return a boost::math::normal random variable representing the statististical delay of an operational vertex in the DFG.
 * @param d is the nominal delay of the operational vertex.
 * @param n is the number of random variables used to generate the statistical representation.
 */
boost::math::normal ComputeStatisticalDelay(double d, int n);

/**
 * @return a boost::math::normal random variable representing the statististical power consumption of an operational vertex in the DFG.
 * @param p is the nominal power consumption of the operational vertex.
 * @param n is the number of random variables used to generate the statistical representation.
 */
boost::math::lognormal ComputeStatisticalPower(double p, int n);

/**
 * @return a boost::math::normal random variable representing an attribute of one operational vertex in the DFG.
 * @param a is the nominal attribute value of the operational vertex.
 * @param n is the number of random variables used to generate the statistical representation.
 */
boost::math::normal CreateStatisticalAttribute(double a, int n);

/**
 * @return a boost::math::normal random variable representing an attribute of one operational vertex in the DFG.
 * @param a is the nominal attribute value of the operational vertex.
 * @param d is the list of the weights of the boost::math::normal random variables.
 * @param mean is mean of the single boost::math::normal random variable.
 * @param st_dev is standard deviation of the single boost::math::normal random variable.
 * @param d_rand is weight of the boost::math::normal variable representing the random component.
 * @param mean_rand is mean of the boost::math::normal variable representing the random component.
 * @param st_dev_rand is standard deviation of the boost::math::normal  variable representing the random component.
 */
boost::math::normal CreateStatisticalAttribute(double a, std::vector<int> d, double mean, double st_dev, int d_rand, double mean_rand, double st_dev_rand);

#endif
