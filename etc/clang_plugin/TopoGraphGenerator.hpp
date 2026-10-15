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
 * @file TopoGraphGenerator.hpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#ifndef TOPOGRAPHGENERATOR_HPP
#define TOPOGRAPHGENERATOR_HPP

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OmpTaskGraphConstants.hpp"
#include "OpenMPTaskPassConfig.hpp"
#include "Types.hpp"
#include "Utils.hpp"

#include <llvm/Support/raw_ostream.h>

#include "pugixml.hpp"

#include <cassert>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace Bambu
{
   namespace OpenMP
   {
      namespace Task
      {

         class TopoGraphGenerator
         {
            using Resources = std::unordered_map<unsigned, unsigned>;
            using NodeId = std::string;
            using NodeType = std::unordered_map<NodeId, unsigned>;

          private:
            AdjMap adjMap;
            Nodes nodes;
            std::unordered_map<std::string, std::pair<Inputs, ConstInputs>> nodesInputs;
            Resources resources;
            NodeType nodeTypes;
            std::vector<std::vector<NodeId>> topo;
            std::unordered_map<NodeId, unsigned> parentsCount;
            std::map<InputId, InputIdIndex> topModuleInputs;
            std::vector<std::string> taskEntriesNames;
            std::unordered_map<std::string, std::string> nodesTaskEntryMap;

            /**
             * @brief Initializes the node types for all nodes.
             *
             * This function iterates through all nodes and assigns a type to each node
             * based on its ID. The type is determined using the Utils::id_num_from_id
             * function.
             */
            void initNodeTypes()
            {
               for(auto& n : nodes)
               {
                  const auto num_id = Utils::id_num_from_id(n);
                  nodeTypes[n] = num_id;
               }
            }

          public:
            /**
             * @brief Prints the topological graph.
             *
             * This function logs the topological graph structure to the error stream.
             * It iterates through the provided 2D vector of NodeIds and logs each node.
             * After logging all nodes in a sub-vector, it logs a separator line.
             *
             * @param topo A 2D vector representing the topological graph, where each
             *             sub-vector contains NodeIds.
             */
            void printTopo(const std::vector<std::vector<NodeId>>& topo)
            {
               Utils::log(llvm::errs(), 0, 1, "Parsed topo graph:");
               for(auto& t : topo)
               {
                  for(auto& n : t)
                  {
                     Utils::log(llvm::errs(), 2, 1, n);
                     Utils::log(llvm::errs(), 2, 1, "Inputs:");
                     if(nodesInputs.find(n) != nodesInputs.end())
                     {
                        for(auto& i : nodesInputs[n].first)
                        {
                           Utils::log(llvm::errs(), 4, 1,
                                      std::get<0>(i) == InputType::Base ? "Base addr" : "Offset addr", std::get<1>(i),
                                      std::get<2>(i));
                        }
                     }
                     else
                     {
                        Utils::log(llvm::errs(), 4, 1, "No inputs");
                     }
                  }
                  Utils::log(llvm::errs(), 2, 1, "-----------------------------------------------");
               }
            }

            /**
             * @brief Builds the topological order of nodes for scheduling.
             *
             * This function constructs a topological order of nodes based on their
             * dependencies defined in the adjacency map. It logs the process and results
             * at various stages. The function performs the following steps:
             * 1. Logs the start of the scheduler.
             * 2. Checks if there are any nodes or adjacency mappings to process.
             * 3. Computes the in-degree (number of incoming edges) for each node.
             * 4. Logs the in-degrees of all nodes.
             * 5. Initializes the starting queue with nodes that have zero in-degree.
             * 6. Processes nodes in the queue to build the topological order, updating
             * in-degrees and adding nodes with zero in-degree to the next queue.
             * 7. Logs the completion of the scheduler and the number of tasks executed.
             * 8. Asserts that the number of visited nodes matches the total number of
             * nodes.
             */
            void buildTopo()
            {
               initNodeTypes();
               Utils::log(llvm::errs(), 0, 1, "Building topo graph");
               if(adjMap.size() == 0 || nodes.size() == 0)
               {
                  Utils::log(llvm::errs(), 0, 1, "Nothing to do");
               }

               std::unordered_map<NodeId, unsigned> inDegree;
               for(auto& n : nodes)
               {
                  for(auto& a : adjMap[n])
                  {
                     inDegree[a]++;
                  }
               }
               for(auto& n : nodes)
               {
                  if(inDegree.find(n) == inDegree.end())
                  {
                     inDegree[n] = 0;
                  }
               }
               // Cache the count
               parentsCount = inDegree;

               Utils::log(llvm::errs(), 0, 1, "In degrees:");
               for(auto& in : inDegree)
               {
                  Utils::log(llvm::errs(), 2, 1, in.first, ":", in.second);
               }

               // Starting queue
               std::vector<NodeId> queue;
               for(auto& p : inDegree)
               {
                  if(p.second == 0)
                  {
                     queue.push_back(p.first);
                  }
               }
               topo.push_back(queue);

               unsigned visited = 0;
               while(!queue.empty())
               {
                  std::vector<NodeId> next_queue;
                  for(auto& n : queue)
                  {
                     visited++;
                     for(auto& a : adjMap[n])
                     {
                        inDegree[a]--;
                        if(inDegree[a] == 0)
                        {
                           next_queue.push_back(a);
                        }
                     }
                  }
                  if(!next_queue.empty())
                  {
                     topo.push_back(next_queue);
                  }
                  queue = next_queue;
               }
#ifdef VERBOSE
               printTopo(topo);
#endif

               assert(visited == nodes.size() && "can not build topological graph");
            }

            TopoGraphGenerator(AdjMap const& _adjMap, Nodes const& _nodes, Resources const& _resources,
                               std::unordered_map<std::string, std::pair<Inputs, ConstInputs>> const& _nodesInputs,
                               std::map<InputId, InputIdIndex> const& _topModuleInputs)
                : adjMap(_adjMap),
                  nodes(_nodes),
                  resources(_resources),
                  nodesInputs(_nodesInputs),
                  topModuleInputs(_topModuleInputs)
            {
               initNodeTypes();
               buildTopo();
            }

            TopoGraphGenerator()
            {
            }

            void setGraphParams(AdjMap const& _adjMap, Nodes const& _nodes, Resources const& _resources,
                                std::unordered_map<std::string, std::pair<Inputs, ConstInputs>> const& _nodesInputs,
                                std::map<InputId, InputIdIndex> const& _topModuleInputs)
            {
               adjMap = _adjMap;
               nodes = _nodes;
               resources = _resources;
               nodesInputs = _nodesInputs;
               topModuleInputs = _topModuleInputs;
            }

            void setTaskEntries(std::vector<std::string> const& _taskEntriesNames)
            {
               taskEntriesNames = _taskEntriesNames;
            }

            void setNodeTaskEntryMap(std::unordered_map<std::string, std::string> const& _map)
            {
               nodesTaskEntryMap = _map;
            }

            void dump_xml(std::string const& filename)
            {
               const auto recoverLambdaIdFromUniqueId = [](std::string const& uniqueId)
               {
                  unsigned s = 0;
                  while(s < uniqueId.size() && (uniqueId[s] < '0' || uniqueId[s] > '9'))
                  {
                     s++;
                  }
                  const auto pos = uniqueId.find('#');
                  return static_cast<unsigned>(std::stoi(uniqueId.substr(s, pos)));
               };

               pugi::xml_document doc;

               // Create root element <graph>
               pugi::xml_node graph = doc.append_child("graph");

               // Create <nodes> section
               pugi::xml_node xml_nodes = graph.append_child("nodes");
               for(const auto& level : topo)
               {
                  for(const auto& node : level)
                  {
                     pugi::xml_node xml_node = xml_nodes.append_child("node");
                     xml_node.append_attribute("id") = node.c_str();
                     xml_node.append_attribute("activating_conditions_count") = parentsCount[node];
                     xml_node.append_attribute("out_activating_conditions_count") =
                         static_cast<unsigned>(adjMap[node].size());
                     const unsigned id = recoverLambdaIdFromUniqueId(node);
                     xml_node.append_attribute("omp_entry_name") =
                         nodesTaskEntryMap[id > 0 ? "0" + std::to_string(id) : std::to_string(id)].c_str();

                     const auto& inputs = nodesInputs[node];
                     // ptr inputs
                     for(const auto& input : inputs.first)
                     {
                        pugi::xml_node xml_input = xml_node.append_child("input");
                        xml_input.append_attribute("is_base_addr") = std::get<0>(input) == InputType::Base ? 1 : 0;
                        xml_input.append_attribute("base_ref") = std::get<1>(input);
                        xml_input.append_attribute("offset") = std::get<2>(input);
                     }
                     // const inputs
                     for(const auto& input : inputs.second)
                     {
                        pugi::xml_node xml_input = xml_node.append_child("const_input");
                        xml_input.append_attribute("value") = input;
                     }
                  }
               }

               // Create <edges> section
               pugi::xml_node xml_edges = graph.append_child("edges");
               for(const auto& node : nodes)
               {
                  const auto& edges = adjMap[node];
                  for(const auto& edge : edges)
                  {
                     pugi::xml_node xml_edge = xml_edges.append_child("edge");
                     xml_edge.append_attribute("source") = node.c_str();
                     xml_edge.append_attribute("target") = edge.c_str();
                  }
               }

               // Create <top_inputs> section
               pugi::xml_node xml_inputs = graph.append_child("top_inputs");
               for(auto const& i : topModuleInputs)
               {
                  auto const& id = i.first;
                  auto const& idIndex = i.second;
                  pugi::xml_node xml_input = xml_inputs.append_child("input");
                  xml_input.append_attribute("id") = id;
                  xml_input.append_attribute("index") = idIndex;
               }

               // Create <omp_entries> section
               pugi::xml_node xml_omp_entries = graph.append_child("omp_entries");
               for(auto const& name : taskEntriesNames)
               {
                  pugi::xml_node xml_entry = xml_omp_entries.append_child("entry");
                  xml_entry.append_attribute("id") = name.c_str();
               }

               doc.save_file(filename.c_str(), "  ", pugi::format_indent | pugi::format_no_empty_element_tags);
            }
         };
      } // namespace Task
   } // namespace OpenMP
} // namespace Bambu

#endif

#endif
