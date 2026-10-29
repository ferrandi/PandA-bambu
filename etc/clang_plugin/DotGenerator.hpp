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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
#include "panda_clang_compat.hpp"

/**
 * @file DotGenerator.hpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#ifndef DOT_GENERATOR_HPP
#define DOT_GENERATOR_HPP

#if PANDA_LLVM_CLANG_MAJOR >= 19

#include "OpenMPTaskPassConfig.hpp"

#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

struct Generator
{
   const std::vector<std::string> colours = {"slateblue4",
                                             "cadetblue3",
                                             "chocolate",
                                             "brown",
                                             "cadetblue4",
                                             "darkslateblue",
                                             "darksalmon",
                                             "gold",
                                             "darkolivegreen3",
                                             "darkorange",
                                             "darkorchid",
                                             "deepskyblue",
                                             "firebrick",
                                             "forestgreen",
                                             "goldenrod",
                                             "hotpink",
                                             "indianred",
                                             "khaki",
                                             "lavender",
                                             "lightcoral",
                                             "lightseagreen",
                                             "lightskyblue",
                                             "limegreen",
                                             "mediumaquamarine",
                                             "mediumorchid",
                                             "mediumpurple",
                                             "mediumseagreen",
                                             "mediumslateblue",
                                             "mediumspringgreen",
                                             "mediumturquoise",
                                             "mediumvioletred",
                                             "midnightblue",
                                             "navajowhite",
                                             "olivedrab",
                                             "orange",
                                             "orangered",
                                             "orchid",
                                             "palegreen",
                                             "paleturquoise",
                                             "palevioletred",
                                             "peachpuff",
                                             "peru",
                                             "pink",
                                             "plum",
                                             "powderblue",
                                             "rosybrown",
                                             "royalblue",
                                             "saddlebrown",
                                             "salmon",
                                             "sandybrown"};

   void generate_dot(llvm::raw_fd_ostream& out, std::string const& file_name, std::vector<std::string> const& nodes,
                     std::vector<std::pair<std::string, std::string>> const& edges) const
   {
      const auto originalId = [](std::string const& id) {
         std::string res = "";
         unsigned i = 0;
         while(i < id.size() && !(id[i] >= '0' && id[i] <= '9'))
         {
            i++;
         }
         while(i < id.size() && (id[i] >= '0' && id[i] <= '9'))
         {
            res += id[i];
            i++;
         }
         return res;
      };

      const auto fixNodeLabel = [](std::string label) {
         for(auto& c : label)
         {
            if(c == ',')
            {
               c = '_';
            }
         }
         label.erase(std::remove(label.begin(), label.end(), '{'), label.end());
         label.erase(std::remove(label.begin(), label.end(), '}'), label.end());
         label.erase(std::remove(label.begin(), label.end(), ' '), label.end());
         label.erase(std::remove(label.begin(), label.end(), '#'), label.end());
         label = "T" + label;
         return label;
      };

      std::ofstream dot_file(file_name);
      auto col = colours.begin();

      if(!dot_file.is_open())
      {
         out << "Failed to open the file: " << file_name << '\n';
         return;
      }

      std::unordered_map<std::string, std::string> used_id;
      std::unordered_map<std::string, std::string> node_colors;
      for(auto const& node : nodes)
      {
         const auto id = originalId(node);
         if(used_id.find(id) == used_id.end())
         {
            if(col == colours.end())
            {
               col = colours.begin();
            }
            used_id[id] = *col;
            node_colors[node] = *col;
            col++;
         }
         else
         {
            node_colors[node] = used_id[id];
         }
      }

      // Write DOT format
      dot_file << "digraph G {\n";
      // Add nodes with colors
      for(const auto& node : node_colors)
      {
         dot_file << "    " << fixNodeLabel(node.first) << " [color=" << node.second
                  << ", style=filled, fontcolor=white];\n";
      }
      // Add edges
      for(const auto& edge : edges)
      {
         dot_file << "    " << fixNodeLabel(edge.first) << " -> " << fixNodeLabel(edge.second) << ";\n";
      }
      dot_file << "}\n";
      dot_file.close();

      out << "Dot file created\n";
   }
};

#endif

#endif
