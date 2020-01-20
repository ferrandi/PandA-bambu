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
 * @file Statistics.cpp
 * @brief This file collects some algebric statistical functions.
 *
 *
 * @author Carmine Galeone
 * $Revision$
 * $Date$
 *
 */
#include <boost/math/special_functions/fpclassify.hpp>
#include <cmath>

#include "Statistics.hpp"

boost::math::normal MultiplyVarForCoefficient(int coeff, boost::math::normal var)
{
   double mean, st_dev;
   mean = var.mean() * coeff;
   st_dev = var.standard_deviation() * coeff;
   boost::math::normal s(mean, st_dev);
   return s;
}

boost::math::normal VarSum(boost::math::normal x, double c)
{
   boost::math::normal s((x.mean() + c), x.standard_deviation());
   return s;
}

boost::math::normal VarSum(boost::math::normal x, boost::math::normal y)
{
   return VarSum(x, y, 0);
}

boost::math::normal VarSum(boost::math::normal x, boost::math::normal y, double p)
{
   double mean, st_dev;
   mean = x.mean() + y.mean();

   // Mathematically, Cov(X,Y) = p * StandardVariation(x) * StandardVariation(y)
   st_dev = sqrt(pow(x.standard_deviation(), 2) + pow(y.standard_deviation(), 2) + 2 * p * x.standard_deviation() * y.standard_deviation());
   boost::math::normal s(mean, st_dev);
   return s;
}

boost::math::lognormal VarSum(std::vector<boost::math::lognormal> v)
{
   if(!v.empty())
   {
      boost::math::lognormal s = v.at(0);
      for(unsigned int i = 1; i < v.size(); i++)
      {
         s = VarSum(s, v.at(i));
      }
      return s;
   }
   return boost::math::lognormal();
}

boost::math::lognormal VarSum(boost::math::lognormal x, boost::math::lognormal y)
{
   return VarSum(x, y, 0);
}

boost::math::lognormal VarSum(boost::math::lognormal s1, boost::math::lognormal s2, double p)
{
   double mean, st_dev;
   double u1, u2;

   u1 = exp(s1.location() + (pow(s1.scale(), 2)) / 2) + exp(s2.location() + (pow(s2.scale(), 2)) / 2);
   u2 = exp(2 * s1.location() + 2 * pow(s1.scale(), 2)) + exp(2 * s2.location() + 2 * pow(s2.scale(), 2)) + 2 * exp(s1.location() + s2.location()) * exp(0.5 * (pow(s1.scale(), 2) + pow(s2.scale(), 2) + 2 * p * s1.scale() * s2.scale()));

   mean = 2 * log(u1) - 0.5 * log(u2);
   st_dev = sqrt(log(u2) - 2 * log(u1));

   boost::math::lognormal s(mean, st_dev);
   return s;
}

boost::math::normal VarMax(boost::math::normal x, boost::math::normal y, double p)
{
   double mean = 0, st_dev = 0, var = 0;
   double a = 0, alpha = 0;

   a = pow((pow(x.standard_deviation(), 2) + pow(y.standard_deviation(), 2) - 2 * p * x.standard_deviation() * y.standard_deviation()), 0.5);
   alpha = (x.mean() + y.mean()) / a;

   mean = x.mean() * cdf(x, alpha) + y.mean() * cdf(y, -alpha) + a * pdf(x, alpha);
   var = (pow(x.standard_deviation(), 2) + pow(x.mean(), 2)) * cdf(x, alpha) + (pow(y.standard_deviation(), 2) + pow(y.mean(), 2)) * cdf(y, -alpha) + (x.mean() + y.mean()) * a * pdf(x, alpha) - pow(mean, 2);

   st_dev = sqrt(var);
   boost::math::normal s(mean, st_dev);
   return s;
}

boost::math::normal VarMax(std::vector<boost::math::normal> v)
{
   if(!v.empty())
   {
      boost::math::normal s = v.at(0);
      for(unsigned int i = 1; i < v.size(); i++)
      {
         s = VarMax(s, v.at(i), 0);
      }
      return s;
   }
   return boost::math::normal();
}

boost::math::normal ComputeStatisticalDelay(double d, int n)
{
   return CreateStatisticalAttribute(d, n);
}

boost::math::lognormal ComputeStatisticalPower(double p, int n)
{
   boost::math::normal s = CreateStatisticalAttribute(p, n);
   boost::math::lognormal v(s.mean(), s.standard_deviation());
   return v;
}

boost::math::normal CreateStatisticalAttribute(double a, int n)
{
   std::vector<int> v;
   v.reserve(static_cast<size_t>(n));
   for(int i = 0; i < n; i++)
   {
      v.push_back(1);
   }

   return CreateStatisticalAttribute(a, v, 0, 1, 1, 0, 1);
}

boost::math::normal CreateStatisticalAttribute(double a, std::vector<int> d, double mean, double st_dev, int d_rand, double mean_rand, double st_dev_rand)
{
   boost::math::normal ris;
   boost::math::normal x_rand(mean_rand, st_dev_rand);
   ris = MultiplyVarForCoefficient(d_rand, x_rand);

   for(int i : d)
   {
      boost::math::normal temp(mean, st_dev);
      ris = VarSum(ris, MultiplyVarForCoefficient(i, temp));
   }

   return VarSum(ris, a);
}
