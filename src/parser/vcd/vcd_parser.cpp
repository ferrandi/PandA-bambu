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

// include class header
#include "vcd_parser.hpp"
#include <iostream>
// include from ./
#include "Parameter.hpp"

#include "dbgPrintHelper.hpp" // for DEBUG_LEVEL_
#include "hash_helper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS
#include "structural_objects.hpp"

vcd_parser::vcd_parser(const ParameterConstRef& param) : debug_level(param->get_class_debug_level(GET_CLASS(*this))), vcd_fp(nullptr), sig_n(0)
{
}

vcd_parser::vcd_trace_t vcd_parser::parse_vcd(const std::string& vcd_file_to_parse, const vcd_filter_t& selected_signals)
{
   // ---- initialization ----
   // open file
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Opening VCD file to parse: " + vcd_file_to_parse);
   vcd_fp = nullptr;
   vcd_fp = fopen(vcd_file_to_parse.c_str(), "r");
   if(vcd_fp == nullptr)
   {
      THROW_ERROR("Unable to open VCD file: " + vcd_file_to_parse);
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Opened VCD file to parse: " + vcd_file_to_parse);
   // initialize member file name
   vcd_filename = vcd_file_to_parse;

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Initializing parser");
   parse_result = vcd_trace_t();
   // avoid useless parse
   if(selected_signals.empty())
   {
      return std::move(parse_result);
   }

   // initialize signals to select
   filtered_signals = selected_signals;
   // reset parsed signals
   sig_n = 0;
   // reserve space in hash maps
   size_t tot_filtered_signals_size = 0;
   parse_result.reserve(filtered_signals.size());
   for(const auto& scope : filtered_signals)
   {
      size_t n_signals_in_scope = scope.second.size();
      parse_result[scope.first].reserve(n_signals_in_scope);
      tot_filtered_signals_size += n_signals_in_scope;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Initialized parser");

   // ---- parse ----
   // parse definitions
   vcd_parse_def();
   // parse waveforms
   vcd_parse_sim();
   // ---- statistics ----
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Number of selected signals: " + STR(scope_and_name_to_sig_info.size()) + "/" + STR(sig_n));
   // ---- cleanup ----
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "-->Cleaning up VCD parser");
   fclose(vcd_fp);
   filtered_signals.clear();
   vcd_filename.clear();
   scope_and_name_to_sig_info.clear();
   vcd_id_to_scope_and_name.clear();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "<--Cleaned up VCD parser");
   return std::move(parse_result);
}

/**
 * Parses specified file until $end keyword is seen, ignoring all text inbetween.
 */
int vcd_parser::vcd_parse_skip_to_end()
{
   char token[256];     /* String value of current token */
   int chars_read = -1; /* Number of characters scanned in */

   while(fscanf(vcd_fp, "%255s%n", token, &chars_read) != EOF)
   {
      if(chars_read < 0)
      {
         continue;
      }

      if(chars_read > 256)
      {
         THROW_ERROR("buffer overflow. read token too long");
      }

      if(strncmp("$end", token, 4) == 0)
      {
         return 0;
      }

      chars_read = -1;
   }
   return -1;
}

/**
 * Parses definition $var keyword line until $end keyword is seen.
 */
int vcd_parser::vcd_parse_def_var(const std::string& scope)
{
   char type[256];       /* Variable type */
   unsigned int size;    /* Bit width of specified variable */
   char id_code[256];    /* Unique variable identifier_code */
   char ref[256];        /* Name of variable in design */
   char tmp[15];         /* Temporary string holder */
   unsigned int msb = 0; /* Most significant bit */
   unsigned int lsb = 0; /* Least significant bit */

   if(fscanf(vcd_fp, "%255s %u %255s %255s %14s", type, &size, id_code, ref, tmp) == 5)
   {
      bool isvect = false; /* check if the signal is a vector */
      /* Make sure that we have not exceeded array boundaries */
      if(strlen(type) > 256)
      {
         THROW_ERROR("Overflow. Read token too long (type var)");
      }

      if(strlen(ref) > 256)
      {
         THROW_ERROR("Overflow. Read token too long (name var)");
      }

      if(strlen(tmp) > 15)
      {
         THROW_ERROR("Overflow. Read token too long ($end token)");
      }

      if(strlen(id_code) > 256)
      {
         THROW_ERROR("Overflow. Read token too long (id_code)");
      }

      if(strncmp("real", type, 4) == 0)
      {
         msb = 63;
         lsb = 0;
         isvect = true;
      }
      else
      {
         char reftmp[256]; /* Temporary variable name */
         if(strncmp("$end", tmp, 4) != 0)
         {
            /* A bit select was specified for this signal, get the size */
            if(sscanf(tmp, "[%u:%u]", &msb, &lsb) != 2)
            {
               if(sscanf(tmp, "[%u]", &lsb) != 1)
               {
                  THROW_ERROR("Unrecognized $var format");
               }
               else
               {
                  msb = lsb;
               }
            }
            isvect = true;

            if((fscanf(vcd_fp, "%14s", tmp) != 1) || (strncmp("$end", tmp, 4) != 0))
            {
               THROW_ERROR("Unrecognized $var format");
            }
         }
         else if(sscanf(ref, "%*[a-zA-Z0-9_][%*s].") == 2)
         {
            /* This is a hierarchical reference so we shouldn't modify ref -- quirky behavior from VCS */
            msb = size - 1;
            lsb = 0;
            /* this is the case of signal (like integer) that are defined in the VCD in the same way of bit but they are arrays */
            if(msb > 0)
            {
               isvect = true;
            }
         }
         else if(sscanf(ref, "%255[a-zA-Z0-9_][%14s]", reftmp, tmp) == 2)
         {
            strcpy(ref, reftmp);

            if(sscanf(tmp, "%u:%u", &msb, &lsb) != 2)
            {
               if(sscanf(tmp, "%u", &lsb) != 1)
               {
                  THROW_ERROR("Unrecognized $var format");
               }
               else
               {
                  msb = lsb;
               }
            }
            isvect = true;
         }
         else
         {
            msb = size - 1;
            lsb = 0;
            /* this is the case of signal (like integer) that are defined in the VCD in the same way of bit but they are arrays */
            if(msb > 0)
            {
               isvect = true;
            }
         }
      }

      sig_n++;

      /* if check fails do nothing: this signal is useless */
      if(!check_filter_list(scope, ref))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Filtered SIGNAL: " + scope + ref);
         return 0;
      }
      PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "SELECTED SIGNAL: " + scope + ref);

      vcd_add_signal(scope, ref, id_code, type, isvect, msb, lsb);
   }
   else
   {
      THROW_ERROR("Unrecognized $var format");
   }
   return 0;
}

/**
 * Parses definition $scope keyword line until $end keyword is seen.
 */
void vcd_parser::vcd_push_def_scope(std::stack<std::string>& scope)
{
   char new_scope[256]; /* scope name */

   if(fscanf(vcd_fp, " %*s %255s $end ", new_scope) == 1)
   {
      /* Make sure that we have not exceeded any array boundaries */
      if(strlen(new_scope) > 256)
      {
         THROW_ERROR("Overflow. Read token too long.");
      }
   }
   else
   {
      THROW_ERROR("Unrecognized $scope format");
   }
   scope.push(scope.top() + new_scope + STR(HIERARCHY_SEPARATOR));
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "NEW_SCOPE: " + scope.top());
   return;
}

void vcd_parser::vcd_pop_def_scope(std::stack<std::string>& scope)
{
   char token[256];

   if(fscanf(vcd_fp, " %255s ", token) == 1)
   {
      /* Make sure that we have not exceeded any array boundaries */
      if(strlen(token) > 256)
      {
         THROW_ERROR("Overflow. Read token too long");
      }

      if(strncmp(token, "$end", 4) == 0)
      {
         scope.pop();
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "UP_SCOPE: " + scope.top());
         return;
      }
      else
      {
         THROW_ERROR("missing $end after $upscope token");
      }
   }
   THROW_ERROR("Unrecognized $upscope format");
}

/*
 * Parses all definition information from specified file.
 */
int vcd_parser::vcd_parse_def()
{
   /*
    * stack for the scopes. it holds the string representing the current scope,
    * i. e. the path down through the structural components, using their names
    * separated by HIERARCHY_SEPARATOR
    */
   std::stack<std::string> scope;
   scope.push("");

   bool enddef_found = false; /* If set to true, definition section is finished */
   char keyword[256];         /* Holds keyword value */
   int chars_read;            /* Number of characters scanned in */

   while(!enddef_found && (fscanf(vcd_fp, "%255s%n", keyword, &chars_read) == 1))
   {
      if(chars_read > 256)
      {
         THROW_ERROR("Overflow. Read token too long");
      }

      /* check the token and chose the right action */
      if(keyword[0] == '$')
      {
         if(strncmp("var", (keyword + 1), 3) == 0)
         {
            vcd_parse_def_var(scope.top());
         }
         else if(strncmp("scope", (keyword + 1), 5) == 0)
         {
            vcd_push_def_scope(scope);
         }
         else if(strncmp("upscope", (keyword + 1), 7) == 0)
         {
            vcd_pop_def_scope(scope);
         }
         else if(strncmp("enddefinitions", (keyword + 1), 14) == 0)
         {
            enddef_found = true;
            if(vcd_parse_skip_to_end())
            {
               THROW_ERROR("missing $end token in parsed vcd file: " + vcd_filename);
            }
         }
         else if(vcd_parse_skip_to_end())
         {
            THROW_ERROR("missing $end token in parsed vcd file: " + vcd_filename);
         }
      }
      else
      {
         THROW_ERROR("undefined token '" + STR(keyword) + "' in parsed vcd file: " + vcd_filename);
      }
   }

   if(!enddef_found)
   {
      THROW_ERROR("Specified VCD file is not a valid VCD file");
   }

   if(!check_signals())
   {
      THROW_ERROR("Missing bits in port vector declaration");
   }

   return 0;
}

/**
 * Reads the next token from the file and calls the appropriate fuction
 */
int vcd_parser::vcd_parse_sim_vector(char* value, unsigned long timestamp)
{
   char sym[256];  /* String value of signal symbol */
   int chars_read; /* Number of characters scanned in */

   if(fscanf(vcd_fp, "%255s%n", sym, &chars_read) == 1)
   {
      if(chars_read > 256)
      {
         THROW_ERROR("Overflow. Read token too long");
      }

      /* add variation to signal list -> normal vector */
      add_variation(sym, value, timestamp);
   }
   else
   {
      THROW_ERROR("Bad file format");
   }

   return 0;
}

/**
 * Reads the next token from the file and calls the appropriate function
 */
int vcd_parser::vcd_parse_sim_real(char* value, unsigned long timestamp)
{
   char sym[256];  /* String value of signal symbol */
   int chars_read; /* Number of characters scanned in */

   if(fscanf(vcd_fp, "%255s%n", sym, &chars_read) == 1)
   {
      if(chars_read > 256)
      {
         THROW_ERROR("Overflow. Read token too long");
      }

      /* add variation to signal list */
      add_variation(sym, value, timestamp);
   }
   else
   {
      THROW_ERROR("Bad file format");
   }
   return 0;
}

/**
 * Parses all lines that occur in the simulation portion of the VCD file.
 */
int vcd_parser::vcd_parse_sim()
{
   char token[4100];                /* Current token from VCD file */
   unsigned long last_timestep = 0; /* Value of last timestamp from file */
   int chars_read;                  /* Number of characters scanned in */
   bool carry_over = false;         /* Specifies if last string was too long */

   // initialize the waveforms
   init_variations();
   while(!feof(vcd_fp) && (fscanf(vcd_fp, "%4099s%n", token, &chars_read) == 1))
   {
      if(chars_read < 4099)
      {
         if(token[0] == '$')
         {
            /* Maybe could be a comment area */
            if(strncmp("comment", (token + 1), 7) == 0)
            {
               if(vcd_parse_skip_to_end())
               {
                  THROW_ERROR("missing $end token in parsed vcd file: " + vcd_filename);
               }
            }
         }
         else if((token[0] == 'b') || (token[0] == 'B'))
         {
            if(vcd_parse_sim_vector((token + 1), last_timestep))
            {
               THROW_ERROR("can't parse binary value change for signal: " + STR(token));
            }
         }
         else if((token[0] == 'r') || (token[0] == 'R') || carry_over)
         {
            if(vcd_parse_sim_real((token + 1), last_timestep))
            {
               THROW_ERROR("can't parse real value change for signal: " + STR(token));
            }
            carry_over = false;
         }
         else if(token[0] == '#')
         {
            last_timestep = std::stoul(token + 1, nullptr, 10);
         }
         else if((token[0] == '0') || (token[0] == '1') || (token[0] == 'x') || (token[0] == 'X') || (token[0] == 'z') || (token[0] == 'Z'))
         {
            /* normal signal -> add to vector */
            char tmp[2];
            tmp[0] = token[0];
            tmp[1] = '\0';

            add_variation(token + 1, tmp, last_timestep);
         }
         else
         {
            THROW_ERROR("Badly placed token in simulation part");
         }
      }
      else
      {
         carry_over = true;
      }
   }

   /* simulation is done correctly */
   return 0;
}

bool vcd_parser::check_filter_list(const std::string& scope_str, const std::string& name)
{
   if(filtered_signals.empty())
   {
      return false;
   }
   const auto scope_it = filtered_signals.find(scope_str);
   if(scope_it == filtered_signals.end())
   {
      return false;
   }
   const auto& signal_names = scope_it->second;
   return signal_names.find(name) != signal_names.end();
}

void vcd_parser::vcd_add_signal(const std::string& scope, const std::string& name, const std::string& vcd_id, const std::string& type, const bool isvect, const unsigned int msb, const unsigned int lsb)
{
   THROW_ASSERT(!scope.empty() && !name.empty(), "scope = \"" + scope + "\" name = \"" + name + "\"");

   std::pair<std::string, std::string> key = std::make_pair(scope, name);
   auto it = scope_and_name_to_sig_info.find(key);
   if(it == scope_and_name_to_sig_info.end())
   { // the signal is new
      scope_and_name_to_sig_info.insert(std::make_pair(key, vcd_sig_info(type, isvect, msb, lsb)));
      scope_and_name_to_sig_info.at(key).vcd_id_to_bit[vcd_id] = lsb;
      vcd_id_to_scope_and_name[vcd_id].insert(key);
      parse_result.at(scope)[name] = std::list<sig_variation>();
   }
   else
   { // some bits of the signal have already been declared
      THROW_ASSERT(msb == lsb, "partial vector declaration supported only one bit at a time");
      size_t bit = lsb;
      vcd_sig_info& old_sig_info = scope_and_name_to_sig_info.at(key);
      if(type != old_sig_info.type || !old_sig_info.is_vec)
      {
         THROW_ERROR("type for signal " + scope + name + " is not consistent");
      }
      old_sig_info.lsb = std::min(bit, old_sig_info.lsb);
      old_sig_info.msb = std::max(bit, old_sig_info.msb);
      scope_and_name_to_sig_info.at(key).vcd_id_to_bit[vcd_id] = bit;
      vcd_id_to_scope_and_name[vcd_id].insert(key);
   }
}

bool vcd_parser::check_signals() const
{
   for(const auto& si : scope_and_name_to_sig_info)
   {
      const vcd_sig_info& info = si.second;
      if(info.is_vec)
      {
         if((info.lsb != 0) || (info.vcd_id_to_bit.size() > 1 && info.vcd_id_to_bit.size() != (info.msb - info.lsb + 1)))
         {
            PRINT_OUT_MEX(DEBUG_LEVEL_NONE, 0, "port vector declaration is not consistent for vcd signal: \n" + si.first.first + si.first.second + "\n");
            return false;
         }
      }
   }
   return true;
}

void vcd_parser::init_variations()
{
   for(const auto& vcd2sn : vcd_id_to_scope_and_name)
   {
      for(const auto& sn : vcd2sn.second)
      {
         const vcd_sig_info& siginfo = scope_and_name_to_sig_info.at(sn);
         std::list<sig_variation>& vars = parse_result.at(sn.first).at(sn.second);
         vars.emplace_back(0, std::string(siginfo.msb - siginfo.lsb + 1, 'x'));
      }
   }
}

void vcd_parser::add_variation(const std::string& sig_id, const std::string& value, unsigned long long ts)
{
   THROW_ASSERT(!value.empty(), "trying to add an empty variation for vcd id " + sig_id + " at time " + STR(ts));
   THROW_ASSERT(!sig_id.empty(), "adding a variation to unspecified vcd signal");
   auto it = vcd_id_to_scope_and_name.find(sig_id);
   if(it != vcd_id_to_scope_and_name.end())
   {
      for(const auto& sn : it->second)
      {
         const vcd_sig_info& siginfo = scope_and_name_to_sig_info.at(sn);
         std::list<sig_variation>& vars = parse_result.at(sn.first).at(sn.second);
         THROW_ASSERT(vars.back().time_stamp <= ts, "Variations are not being added in time order: id = '" + sig_id + "', ts = " + STR(vars.back().time_stamp) + " > " + STR(ts));
         THROW_ASSERT(!siginfo.vcd_id_to_bit.empty(), "signal " + sn.first + STR(HIERARCHY_SEPARATOR) + sn.second + " has no mapped vcd_id");
         /* prepare the new value for variation to insert */
         std::string new_value;
         if(siginfo.vcd_id_to_bit.size() > 1)
         {
            /*
             * the signal is a port vector with a separate id for every bit,
             * so we must keep all the previous bits and change only the new
             */
            THROW_ASSERT(value.size() == 1, "variation of a bit is larger than a bit");
            new_value = std::string(vars.back().value);
            THROW_ASSERT(siginfo.vcd_id_to_bit.find(sig_id) != siginfo.vcd_id_to_bit.end(), "vcd id " + sig_id + " is not assigned to any bit of port" + sn.first + STR(HIERARCHY_SEPARATOR) + sn.second);
            size_t idx = siginfo.vcd_id_to_bit.at(sig_id);
            THROW_ASSERT(idx < new_value.size(), "vcd_id " + sig_id + " for signal " + sn.first + STR(HIERARCHY_SEPARATOR) + sn.second + " is mapped to a bit higher than port size");
            new_value.at(new_value.size() - idx - 1) = value.front();
         }
         else
         {
            /*
             * the signal can be a bit or a port vector, but in the vcd it
             * has a single unique tag to represent all the bits
             */
            new_value = value;
            /* check bit extension */
            if((siginfo.msb - siginfo.lsb + 1) > value.size())
            {
               char leading = *new_value.begin();
               char to_prepend = '0';
               if(leading != '0' && leading != '1')
               {
                  to_prepend = leading;
               }
               while(new_value.size() < (siginfo.msb - siginfo.lsb + 1))
               {
                  new_value.insert(0, 1, to_prepend);
               }
            }
         }

         if(vars.back().time_stamp == ts)
         {
            /*
             * another variation for this signal was already added in this
             * cycle. this can happen, especially in vcds produced by event
             * based simulators. in this case the last variation must
             * override the others
             */
            vars.back().value = new_value;
         }
         else
         {
            vars.back().duration = ts - vars.back().time_stamp;
            vars.emplace_back(ts, new_value);
         }
      }
   }
}
