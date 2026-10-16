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

#ifndef UTILS_HPP
#define UTILS_HPP

#if PANDA_LLVM_CLANG_MAJOR >= 13

#include "OpenMPTaskPassConfig.hpp"

#include <llvm/Support/raw_ostream.h>

#include <pugixml.hpp>

#include <cassert>
#include <chrono>
#include <string>
#include <vector>

namespace Bambu
{
   namespace OpenMP
   {
      namespace Task
      {
         namespace Utils
         {
#ifdef PUGIXML_NO_XPATH
            template <typename CallBack>
            void searchAndApply(pugi::xml_node node, const std::string& targetNode, CallBack callback);
#endif
            /// Log util
            /// @tparam T A generic message type
            /// @param out The desired output stream
            /// @param indent The number of spaces to prefix the message content
            /// @param noNewLines The number of new lines to postfix the message content
            template <typename... T>
            void log(llvm::raw_fd_ostream& out, uint8_t indent, uint8_t noNewLines, T&&... args)
            {
#if defined(__cplusplus) && __cplusplus >= 201703L
               std::string indentStr = "";
               for(uint8_t i = 0; i < indent; ++i)
               {
                  indentStr += ' ';
               }
               out << indentStr;
               ((out << args << " "), ...);
               for(uint8_t i = 0; i < noNewLines; ++i)
               {
                  out << "\n";
               }
#endif
            }

            /// @brief Appends a sequence of elements to an XML file.
            ///
            /// This function loads an XML file, verifies the root node, appends a new section,
            /// and then appends each element in the provided sequence to this new section using
            /// a callback function.
            ///
            /// @tparam T The type of elements in the sequence.
            /// @tparam CallBack The type of the callback function.
            /// @param xmlFileName The name of the XML file to which the elements will be appended.
            /// @param topChild The expected name of the root node in the XML file.
            /// @param sectionName The name of the new section to be appended to the root node.
            /// @param elements The sequence of elements to be appended.
            /// @param callback The callback function to be called for each element.
            template <typename T, typename CallBack>
            void appendSeqToXML(std::string const& xmlFileName, std::string const& topChild,
                                std::string const& sectionName, std::vector<T> const& elements, CallBack callback)
            {
               pugi::xml_document doc;
               pugi::xml_parse_result result = doc.load_file(xmlFileName.c_str());
               assert(result);

               pugi::xml_node root = doc.document_element();
               assert(root && std::string(root.name()) == topChild);

               pugi::xml_node ompEntryNodes = root.append_child(sectionName.c_str());

               for(const auto& element : elements)
               {
                  callback(ompEntryNodes, element);
               }

               assert(doc.save_file(xmlFileName.c_str()));
            }

            /// @brief Adds content to a specified node in an XML file using a callback function.
            ///
            /// This function loads an XML file, selects nodes matching the specified target node,
            /// and applies a callback function to each of the selected nodes. The modified XML
            /// document is then saved back to the file.
            ///
            /// @tparam Callback The callback type
            /// @param xmlFileName The name of the XML file to be modified.
            /// @param targetNode The name of the target node to which content will be added.
            /// @param callback A callback function that takes a pugi::xml_node as an argument and modifies it.
            template <typename CallBack>
            void addToNodeXML(std::string const& xmlFileName, std::string const& targetNode, CallBack callback)
#ifndef PUGIXML_NO_XPATH
            {
               pugi::xml_document doc;
               pugi::xml_parse_result result = doc.load_file(xmlFileName.c_str());
               assert(result);

               pugi::xpath_node_set nodes = doc.select_nodes(("//" + targetNode).c_str());
               for(auto& xpath_node : nodes)
               {
                  pugi::xml_node node = xpath_node.node();
                  callback(node);
               }

               assert(doc.save_file(xmlFileName.c_str()));
            }
#else
            {
               pugi::xml_document doc;
               pugi::xml_parse_result result = doc.load_file(xmlFileName.c_str());
               assert(result);

               // Start traversal from the root node.
               // Note that some XML documents may have a root declaration, so you might choose the first child
               // or work directly with doc.document_element() depending on your XML structure.
               for(pugi::xml_node node : doc.children())
               {
                  searchAndApply(node, targetNode, callback);
               }

               assert(doc.save_file(xmlFileName.c_str()));
            }

            /// Recursive helper function to traverse the XML tree.
            /// @tparam CallBack a generic callback to be applied to a XML node
            /// @param node the node to be visited
            /// @param targetNode the target node id
            /// @param callback the callback function to be applied to a target node
            template <typename CallBack>
            void searchAndApply(pugi::xml_node node, const std::string& targetNode, CallBack callback)
            {
               if(targetNode == node.name())
               {
                  callback(node);
               }

               for(pugi::xml_node child : node.children())
               {
                  searchAndApply(child, targetNode, callback);
               }
            }
#endif

            /// Retrieve the omp entry lambda id from an unique id referrig to a given node in the task graph
            /// @param id The unique id
            /// @return The omp lambda id
            template <int start_idx = 0>
            unsigned id_num_from_id(std::string& id)
            {
               unsigned i = start_idx;
               std::string num = "";
               while(i < id.size() && id[i] >= '0' && id[i] <= '9')
               {
                  num += id[i++];
               }
               return std::stoi(num);
            }

            /// @brief Measures the execution time of a non void function and logs the duration.
            ///
            /// This function takes a function and its arguments, measures the time it takes
            /// to execute the function, and logs the duration in milliseconds. Then returns the computation output.
            ///
            /// @tparam Fn The type of the function to be measured.
            /// @tparam Args The types of the arguments to be passed to the function.
            /// @param name A string representing the name of the function being measured.
            /// @param fn The function to be measured.
            /// @param args The arguments to be passed to the function.
            template <typename Fn, typename... Args>
            static auto measure_not_void(std::string const& name, Fn fn, Args&&... args)
            {
               auto start = std::chrono::high_resolution_clock::now();
               const auto ret = std::forward<Fn>(fn)(std::forward<Args>(args)...);
               auto end = std::chrono::high_resolution_clock::now();
               auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

               log(llvm::errs(), 0, 2, "\n######################### Time taken for", name, ":", duration, "ms");
               return ret;
            }

            /// @brief Measures the execution time of a void function and logs the duration.
            ///
            /// This function takes a function and its arguments, measures the time it takes
            /// to execute the function, and logs the duration in milliseconds.
            ///
            /// @tparam Fn The type of the function to be measured.
            /// @tparam Args The types of the arguments to be passed to the function.
            /// @param name A string representing the name of the function being measured.
            /// @param fn The function to be measured.
            /// @param args The arguments to be passed to the function.
            template <typename Fn, typename... Args>
            static void measure_void(std::string const& name, Fn fn, Args&&... args)
            {
               auto start = std::chrono::high_resolution_clock::now();
               std::forward<Fn>(fn)(std::forward<Args>(args)...);
               auto end = std::chrono::high_resolution_clock::now();
               auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

               log(llvm::errs(), 0, 2, "\n######################### Time taken for", name, ":", duration, "ms");
            }
         }; // namespace Utils
      } // namespace Task
   } // namespace OpenMP
} // namespace Bambu

#endif

#endif