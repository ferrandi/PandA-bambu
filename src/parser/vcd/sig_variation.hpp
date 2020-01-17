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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#ifndef VCD_DATA_HPP
#define VCD_DATA_HPP

#include <limits>
#include <string>

/**
 * This class models a single variation of a signal in vcd
 */
struct sig_variation
{
   /**
    * time stamp of the variation
    */
   unsigned long long time_stamp;

   /**
    * new value of the signal. it's a string because it can have non-binary
    * values like X or Z
    */
   std::string value;

   /**
    * time duration of the value. it's the difference between the time_stamp of
    * the next variation (if any) and the current
    */
   unsigned long long duration;

   /**
    * constructor
    */
   sig_variation(unsigned long long ts = 0, std::string val = "", unsigned long long d = std::numeric_limits<decltype(sig_variation::duration)>::max());
};

bool operator<(const sig_variation& w, const sig_variation& v);
bool operator<=(const sig_variation& w, const sig_variation& v);
bool operator>(const sig_variation& w, const sig_variation& v);
bool operator>=(const sig_variation& w, const sig_variation& v);
bool operator==(const sig_variation& w, const sig_variation& v);
bool operator!=(const sig_variation& w, const sig_variation& v);

bool operator<(const unsigned long long t, const sig_variation& v);
bool operator<=(const unsigned long long t, const sig_variation& v);
bool operator>(const unsigned long long t, const sig_variation& v);
bool operator>=(const unsigned long long t, const sig_variation& v);
bool operator==(const unsigned long long t, const sig_variation& v);
bool operator!=(const unsigned long long t, const sig_variation& v);

bool operator<(const sig_variation& w, const unsigned long long t);
bool operator<=(const sig_variation& w, const unsigned long long t);
bool operator>(const sig_variation& w, const unsigned long long t);
bool operator>=(const sig_variation& w, const unsigned long long t);
bool operator==(const sig_variation& w, const unsigned long long t);
bool operator!=(const sig_variation& w, const unsigned long long t);

#endif
