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
 * @file test_dsatur_coloring.hpp
 * @brief Test unit of the Boost-based implementation of the sequential coloring algorithm based on the work of Daniel Brelaz (Version 1).
 *
 * Some ideas come from the Joseph Culberson's implementation of DSATUR heuristic of Daniel Brelaz.
 * For more details see
 *  - http://web.cs.ualberta.ca/~joe/Coloring/index.html
 *  - New Methods to Color the Vertices of a Graph, Daniel Brelaz, CACM 22, 251--256, 1979.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "dsatur_coloring.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_list_io.hpp>
#include <cstring>
#include <fstream>
#include <iosfwd>
#include <utility>

using namespace boost;
int main(int argc, char* argv[])
{
   typedef adjacency_list<vecS, vecS, undirectedS> Graph1;
   typedef graph_traits<Graph1>::vertex_descriptor vertex_descriptor;
   typedef graph_traits<Graph1>::vertices_size_type vertices_size_type;
   typedef property_map<Graph1, vertex_index_t>::const_type vertex_index_map;
   std::vector<vertex_descriptor> verts;
   if(argc < 2)
   {
      printf("Provide data file in command.\n");
      exit(1);
   }
   // read Graph1
   Graph1 g1;
   const int EDGE_FIELDS = 2;         /* no of fields in edge line  */
   const int P_FIELDS = 3;            /* no of fields in problem line */
   const char* PROBLEM_TYPE = "edge"; /* name of problem type*/
   long VERTICES,                     /*number of vertices*/
       number_of_edges,               /*  number of edges and nodes */
       head, tail;

   long no_lines = 0, /* no of current input line */
       no_plines = 0, /* no of problem-lines */
       no_elines = 0; /* no of edge-lines */

   std::string in_line; /* for reading input line */
   char pr_type[5];     /* for reading type of the problem */

   int err_no; /* no of detected error */

   /* -------------- error numbers & error messages ---------------- */
   const int EN1 = 0;
   const int EN2 = 1;
   const int EN3 = 2;
   const int EN4 = 3;
   //  const int EN6   = 4;
   //  const int EN10  = 5;
   //  const int EN7   = 6;
   const int EN8 = 7;
   // const int EN9   = 8;
   // const int EN11  = 9;
   // const int EN12 = 10;
   //  const int EN13 = 11;
   // const int EN14 = 12;
   // const int EN16 = 13;
   const int EN15 = 14;
   const int EN17 = 15;
   const int EN18 = 16;
   const int EN21 = 17;
   const int EN19 = 18;
   // const int EN20 = 19;
   const int EN22 = 20;

   static const char* err_message[] = {/* 0*/ "more than one problem line.",
                                       /* 1*/ "wrong number of parameters in the problem line.",
                                       /* 2*/ "it is not a Max Flow problem line.",
                                       /* 3*/ "bad value of a parameter in the problem line.",
                                       /* 4*/ "can't obtain enough memory to solve this problem.",
                                       /* 5*/ "more than one line with the problem name.",
                                       /* 6*/ "can't read problem name.",
                                       /* 7*/ "problem description must be before node description.",
                                       /* 8*/ "this parser doesn't support multiply sources and sinks.",
                                       /* 9*/ "wrong number of parameters in the node line.",
                                       /*10*/ "wrong value of parameters in the node line.",
                                       /*11*/ " ",
                                       /*12*/ "source and sink descriptions must be before arc descriptions.",
                                       /*13*/ "too many arcs in the input.",
                                       /*14*/ "wrong number of parameters in the arc line.",
                                       /*15*/ "wrong value of parameters in the arc line.",
                                       /*16*/ "unknown line type in the input.",
                                       /*17*/ "reading error.",
                                       /*18*/ "not enough arcs in the input.",
                                       /*19*/ "source or sink doesn't have incident arcs.",
                                       /*20*/ "can't read anything from the input file."};
   /* --------------------------------------------------------------- */

   /* The main loop:
      -  reads the line of the input,
      -  analyses its type,
      -  checks correctness of parameters,
      -  puts data to the arrays,
      -  does service functions
   */
   std::ifstream in(argv[1]);

   while(std::getline(in, in_line))
   {
      ++no_lines;

      switch(in_line[0])
      {
         case 'c':  /* skip lines with comments */
         case '\n': /* skip empty lines   */
         case '\0': /* skip empty lines at the end of file */
            break;

         case 'p': /* problem description      */
         {
            if(no_plines > 0)
            /* more than one problem line */
            {
               err_no = EN1;
               goto error;
            }

            no_plines = 1;
            if(
                /* reading problem line: type of problem, no of nodes, no of arcs */
                sscanf(in_line.c_str(), "%*c %4s %ld %ld", pr_type, &VERTICES, &number_of_edges) != P_FIELDS)
            /*wrong number of parameters in the problem line*/
            {
               err_no = EN2;
               goto error;
            }

            if(strcmp(pr_type, PROBLEM_TYPE))
            /*wrong problem type*/
            {
               err_no = EN3;
               goto error;
            }

            if(VERTICES <= 0 || number_of_edges <= 0)
            /*wrong value of no of edges or nodes*/
            {
               err_no = EN4;
               goto error;
            }

            {
               for(long vi = 0; vi < VERTICES; ++vi)
               {
                  verts.push_back(add_vertex(g1));
               }
            }
            break;
         }

         case 'e': /* edge description */
         {
            if(no_plines == 0)
            /* there was not problem line above */
            {
               err_no = EN8;
               goto error;
            }

            if(
                /* reading an edge description */
                sscanf(in_line.c_str(), "%*c %ld %ld", &tail, &head) != EDGE_FIELDS)
            /* arc description is not correct */
            {
               err_no = EN15;
               goto error;
            }
            --tail; // index from 0, not 1
            --head;
            if(tail < 0 || tail > VERTICES || head < 0 || head > VERTICES)
            /* wrong value of nodes */
            {
               err_no = EN17;
               goto error;
            }
            graph_traits<Graph1>::edge_descriptor e1;
            bool in1;
            tie(e1, in1) = add_edge(verts[tail], verts[head], g1);
            if(!in1)
            {
               std::cerr << "unable to add edge (" << head << "," << tail << ")" << std::endl;
               exit(1);
            }

            ++no_elines;
            break;
         }
         default:
         {
            /* unknown type of line */
            err_no = EN18;
            goto error;
         }

      } /* end of switch */
   }    /* end of input loop */

   /* ----- all is read  or  error while reading ----- */

   if(!in.eof()) /* reading error */
   {
      err_no = EN21;
      goto error;
   }

   if(no_lines == 0) /* empty input */
   {
      err_no = EN22;
      goto error;
   }

   if(no_elines < number_of_edges) /* not enough arcs */
   {
      err_no = EN19;
      goto error;
   }

   /* Thanks God! all is done */

   {
      std::vector<vertices_size_type> color_vec(num_vertices(g1));
      iterator_property_map<vertices_size_type*, vertex_index_map, vertices_size_type, vertices_size_type&> color(&color_vec.front(), get(vertex_index, g1));
      vertices_size_type num_colors = dsatur_coloring(g1, color);
      std::cout << "Boost colors are: " << num_colors << std::endl;
   }
   return (0);
   /* ---------------------------------- */
error: /* error found reading input */

   printf("\nline %ld of input - %s\n", no_lines, err_message[err_no]);

   exit(1);
   return (1); /* to avoid warning */
}
