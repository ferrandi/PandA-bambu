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

#ifndef VCD_PARSER_HPP
#define VCD_PARSER_HPP

// include from STL
#include <list>
#include <stack>
#include <string>
#include <utility>

#include "custom_map.hpp"
#include "custom_set.hpp"

// include from parser/vcd/
#include "sig_variation.hpp"

// include from utility/
#include "refcount.hpp"

// forward declarations
CONSTREF_FORWARD_DECL(Parameter);

class vcd_sig_info
{
 public:
   /// the type of the signal in vcd file
   std::string type;
   /// true if the vcd_signal is part of a vector
   bool is_vec;
   /// position of the msb of this signal in the vector. valid only if is_vec == true
   size_t msb;
   /// position of the lsb of this signal in the vector. valid only if is_vec == true
   size_t lsb;
   /*
    * if the signal is a vector, this puts in relationship every bit in the
    * vector with the vcd_id related to that bit. the size of this map can
    * be: 0 if the signal is not a vector, 1 if the signal is a bit vector
    * but in the vcd the variations are already grouped together, or (msb -
    * lsb + 1) if the signal is a vector and in the vcd a different id is
    * used for every bit
    */
   CustomUnorderedMap<std::string, size_t> vcd_id_to_bit;

   vcd_sig_info(std::string _type, const bool _is_vec, const size_t _msb, const size_t _lsb) : type(std::move(_type)), is_vec(_is_vec), msb(_msb), lsb(_lsb)
   {
   }
};

class vcd_parser
{
 public:
   /**
    * constructor
    * @param [in] params: is the class holding bambu parameters
    */
   explicit vcd_parser(const ParameterConstRef& params);

   /**
    * this is the type used to select which signals have to be filtered during
    * parsing.
    * the key of the map is a std::string representing a scope.
    * the value type is a UnorderedSetStd containing all the names of the
    * signal that have to be selected in that scope
    */
   typedef UnorderedMapStd<std::string, UnorderedSetStdStable<std::string>> vcd_filter_t;

   /**
    * this type is the result of a parse.
    * the primary key is the scope.
    * the secondary key is the name of the signal.
    * the value type is std::list of sig_variation representing the waveform
    */
   typedef UnorderedMapStd<std::string, CustomUnorderedMapStable<std::string, std::list<sig_variation>>> vcd_trace_t;

   /**
    * parses a file selecting only a predefined set of signals.
    * @param [in] vcd_file_to_parse: name of the file to parse
    * @param [in] selected_signals: signals to be selected. all signals are
    * selected if this parameter is empty
    * @return: the traces of the selected vcd signals
    */
   vcd_trace_t parse_vcd(const std::string& vcd_file_to_parse, const vcd_parser::vcd_filter_t& selected_signals);

 private:
   /**
    * Debug Level
    */
   const int debug_level;

   // ---- METADATA ON THE PARSED FILE ----

   /**
    * name of the vcd file to parse
    */
   std::string vcd_filename;

   /**
    * file pointer to the vcd file to parse
    */
   FILE* vcd_fp;

   /**
    * total number of signals in the vcd file
    */
   unsigned long sig_n;

   /**
    * set of signals to select from the vcd file.
    * if it's empty all the signals will be selected, otherwise the data about
    * uninteresting signals are discarded to save memory
    */
   vcd_filter_t filtered_signals;

   // ---- LOCAL MEMBERS TO HOLD INTERMEDIATE RESULTS DURING PARSE ----

   /**
    * holds the parsed vcd trace during the parsing
    */
   vcd_trace_t parse_result;

   /**
    * maps every pair (scope, signal name) to the corresponding sig_info
    */
   std::map<std::pair<std::string, std::string>, vcd_sig_info> scope_and_name_to_sig_info;

   /**
    * maps every signal id in the vcd to the set of the corresponding pairs (scope, hdl signal name)
    */
   std::map<std::string, CustomUnorderedSet<std::pair<std::string, std::string>>> vcd_id_to_scope_and_name;

   /* Parses the simulation part in the vcd_file */
   int vcd_parse_sim();

   /* Parses the definition part in the vcd_file */
   int vcd_parse_def();

   /* Parses and ignore useless token */
   int vcd_parse_skip_to_end();

   /* Parses $var token */
   int vcd_parse_def_var(const std::string& scope);

   void vcd_push_def_scope(std::stack<std::string>& scope);

   void vcd_pop_def_scope(std::stack<std::string>& scope);

   /* Parses vector in simulation part */
   int vcd_parse_sim_vector(char* value, unsigned long timestamp);

   /* Parses real in sumlation part (actually unused)*/
   int vcd_parse_sim_real(char* value, unsigned long timestamp);

   /**
    * Checks if a signal is to be monitored
    * @return: true if the signal has to be monitored, false if not.
    * @param [in] name: is the name of the signal to be monitored.
    * @param [in, out] scope_str: in input this is the signal scope in the vcd;
    * if the signal has to be monitored the string referenced by this parameter
    * is modified, trimming the initial part of the scope to avoid
    * simulator-dependent path names. After this change the remaining part of
    * the path starts from the top functional unit
    */
   bool check_filter_list(const std::string& scope_str, const std::string& name);

   /**
    * insert a signal in the maps needed for selecting the vcd data and for
    * relating it back to HDL and C variables
    * @param [in] scope: is the string representing the scope in the parsed vcd
    * @param [in] name: the signal name in the parsed vcd
    * @param [in] vcd_id: a string used as unique identifier for the signal in the vcd
    * @param [in] type: string describing the signal type
    * @param [in] isvect: true if the signal is a bit vector
    * @param [in] msb: most significant bit of the signal
    * @param [in] lsb: less significant bit of the signal
    */
   void vcd_add_signal(const std::string& scope, const std::string& name, const std::string& vcd_id, const std::string& type, const bool isvect, const unsigned int msb, const unsigned int lsb);

   /**
    * check if the port vectors declarations are consistent
    */
   bool check_signals() const;

   /* initializes the variations before parsing */
   void init_variations();

   /* add the parsed variation to the proper signal */
   void add_variation(const std::string& id, const std::string& value, unsigned long long ts);
};
#endif
