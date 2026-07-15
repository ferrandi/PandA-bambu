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

   sig_variation(unsigned long long ts = 0, std::string val = "",
                 unsigned long long d = std::numeric_limits<decltype(sig_variation::duration)>::max());
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
