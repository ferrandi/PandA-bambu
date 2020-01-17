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
 * @file utility.hpp
 * @brief This file collects some utility functions and macros.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "config_HAVE_ASSERTS.hpp"

#include <boost/concept/usage.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "string_manipulation.hpp"

/// INT representing infinite
#define INFINITE_INT (std::numeric_limits<int>::max())

/// UNSIGNED INT representing infinite
#define INFINITE_UINT (std::numeric_limits<unsigned int>::max())

/// SIZE_T representing infinite
#define INFINITE_SIZE_T (std::numeric_limits<size_t>::max())

/// DOUBLE representing infinite
#define INFINITE_DOUBLE (std::numeric_limits<double>::max())

/// long double representing infinite
#define INFINITE_LONG_DOUBLE (std::numeric_limits<long double>::max())

/// LONGLONG representing infinite
#define INFINITE_ULONGLONG_INT (std::numeric_limits<unsigned long long int>::max())

/// long double representing -infinite
#define MINUS_INFINITE_LONG_DOUBLE (std::numeric_limits<long double>::min())

/// macro used to solve problem of parameters used only in not-release
#ifdef NDEBUG
#define DEBUG_PARAMETER(parameter)
#else
#define DEBUG_PARAMETER(parameter) parameter
#endif
#if HAVE_ASSERTS
#define ASSERT_PARAMETER(parameter) parameter
#else
#define ASSERT_PARAMETER(parameter)
#endif

/**
 * Macro returning the name of a class. It uses the static version of get_kind_text
 */
#define GET_CLASS_NAME(meth) #meth

/**
 * Macro returning the debug_level of a function
 */
#define GET_FUNCTION_DEBUG_LEVEL() parameters->GetFunctionDebugLevel(GET_CLASS(*this), __func__)

template <class G>
std::string convert_to_binary(G _value, unsigned int precision)
{
   auto value = static_cast<unsigned long long int>(_value);
   std::string bin_value;
   for(unsigned int ind = 0; ind < precision; ind++)
      bin_value = bin_value + (((1LLU << (precision - ind - 1)) & value) ? '1' : '0');
   return bin_value;
}

template <class G>
std::string convert_vector_to_string(const std::vector<G>& vector_form, const std::string& separator, bool trim_empty_elements = true)
{
   std::string string_form;
   for(unsigned int i = 0; i < vector_form.size(); i++)
   {
      std::string element_string = boost::lexical_cast<std::string>(vector_form[i]);
      if(trim_empty_elements and element_string.size() == 0)
         continue;
      if(string_form.size())
         string_form += separator;
      string_form += element_string;
   }
   return string_form;
}

template <class G>
std::vector<G> convert_string_to_vector(const std::string& string_form, const std::string& separator, bool trim_empty_elements = true)
{
   std::vector<G> vector_form;
   std::vector<std::string> tmp_vector_form = SplitString(string_form, separator);
   for(auto& i : tmp_vector_form)
   {
      if(trim_empty_elements and i.size() == 0)
         continue;
      vector_form.push_back(boost::lexical_cast<G>(i));
   }
   return vector_form;
}

/**
 * Functor to tokenize string used with boost::tokenizer
 */
class string_separator
{
 private:
   /// The delimiter
   const std::string delimiter;

 public:
   /**
    * Empty constructor
    */
   string_separator() : delimiter(std::string())
   {
   }

   /**
    * Constructor
    * @param delimiter is the string used to divide the string
    */
   explicit string_separator(std::string _delimiter) : delimiter(std::move(_delimiter))
   {
   }

   /**
    * Tokenize operator
    * @param next is the start of the portion of the string to be tokenized
    * @param end is the end of the string
    * @param tok is the token found
    * @return true if a token has been found)
    */
   bool operator()(std::string::const_iterator& next, std::string::const_iterator& end, std::basic_string<char, std::char_traits<char>, std::allocator<char>>& tok)
   {
      if(next == end)
         return false;
      std::string current(next, end);
      if(current.find(delimiter) != std::string::npos)
      {
         tok = current.substr(0, current.find(delimiter));
         for(size_t counter = current.find(delimiter) + delimiter.size(); counter != 0; counter--)
            next++;
         return true;
      }
      else
      {
         tok = current;
         next = end;
         return true;
      }
   }

   /**
    * Reset function (required to implement boost tokenizerFunction model
    */
   void reset()
   {
   }
};

/**
 * Macro which "pretty prints" a multi-line string
 */
#define PP_ONE_LINE(multi_line_string) boost::regex_replace(multi_line_string, boost::regex("\\n"), "\\\\n")

/**
 * Concept checking class
 * This class is used to check that an object can be converted into long double
 */
template <class T>
struct check_long_double
{
 private:
   T example;

 public:
   BOOST_CONCEPT_USAGE(check_long_double)
   {
      long double ld = example;
      (void)ld;
   }
};

/**
 * Randomly shuffle a vector
 * @param shuffle is the vector to be shuffled
 * @param seed is the seed to be used in the random generator
 */
template <class T>
void ShuffleVector(typename std::vector<T>& shuffle, const unsigned int seed)
{
   srand(seed);
   size_t size = shuffle.size();
   while(size > 1)
   {
      size_t k = static_cast<size_t>(rand()) % size;

      size--;

      T temp = shuffle[size];
      shuffle[size] = shuffle[k];
      shuffle[k] = temp;
   }
}

/// The type used for timestamp
struct TimeStamp
{
   std::string timestamp;
   /**
    * Empty constructor: timestamp is set to 01-01-1970 00.00
    */
   TimeStamp();

   /**
    * Constructor from string
    * @param timestamp is the string containing the timestamp
    */
   explicit TimeStamp(const std::string& timestamp);

   /**
    * Return a timestamp of the current time
    */
   static std::string GetCurrentTimeStamp();

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param timestamp is the timestamp to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const TimeStamp& timestamp);

   friend bool operator<=(const TimeStamp& timestamp1, const TimeStamp& timestamp2);
};
#endif
