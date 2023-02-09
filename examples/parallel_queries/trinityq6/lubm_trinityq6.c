#include "simple_API.h"
#include <stdio.h>

// var_2 = "<http://www.University0.edu>"
// p_var_3 = "ub:subOrganizationOf"
// p_var_4 = "ub:Department"
// p_var_5 = "a"
// p_var_7 = "ub:worksFor"
// p_var_8 = "ub:FullProfessor"
// p_var_9 = "a"

__attribute__((noinline)) void kernel(size_t i_var_3, Graph* graph, NodeId var_2, PropertyId p_var_3,
                                      PropertyId p_var_4, PropertyId p_var_5, PropertyId p_var_7, PropertyId p_var_8,
                                      PropertyId p_var_9, size_t in_degree_var_2, Edge* var_2_1_inEdges)
{
   unsigned localCounter = 0;
   PropertyId var_3; // corresponding to element having label "ub:subOrganizationOf"
   var_3 = var_2_1_inEdges[i_var_3].property;
   NodeId var_1; // corresponding to element having label "?Y"
   var_1 = var_2_1_inEdges[i_var_3].node;
   int cond_level_2 = (var_3 == p_var_3);
   if(cond_level_2)
   {
      size_t in_degree_var_1 = getInDegree(graph, var_1);
      Edge* var_1_3_inEdges = getInEdges(graph, var_1);
      size_t i_var_7;
      for(i_var_7 = 0; i_var_7 < in_degree_var_1; i_var_7++)
      {
         PropertyId var_7; // corresponding to element having label "ub:worksFor"
         var_7 = var_1_3_inEdges[i_var_7].property;
         NodeId var_6; // corresponding to element having label "?X"
         var_6 = var_1_3_inEdges[i_var_7].node;
         int cond_level_4 = (var_7 == p_var_7);
         if(cond_level_4)
         {
            size_t out_degree_var_6 = getOutDegree(graph, var_6);
            Edge* var_6_5_outEdges = getOutEdges(graph, var_6);
            size_t i_var_9;
            for(i_var_9 = 0; i_var_9 < out_degree_var_6; i_var_9++)
            {
               PropertyId var_9; // corresponding to element having label "a"
               var_9 = var_6_5_outEdges[i_var_9].property;
               NodeId var_8; // corresponding to element having label "ub:FullProfessor"
               var_8 = var_6_5_outEdges[i_var_9].node;
               int cond_level_6 = ((var_9 == p_var_9) & (var_8 == p_var_8));
               if(cond_level_6)
               {
                  size_t out_degree_var_1 = getOutDegree(graph, var_1);
                  Edge* var_1_7_outEdges = getOutEdges(graph, var_1);
                  size_t i_var_5;
                  for(i_var_5 = 0; i_var_5 < out_degree_var_1; i_var_5++)
                  {
                     PropertyId var_5; // corresponding to element having label "a"
                     var_5 = var_1_7_outEdges[i_var_5].property;
                     NodeId var_4; // corresponding to element having label "ub:Department"
                     var_4 = var_1_7_outEdges[i_var_5].node;
                     int cond_level_8 = ((var_5 == p_var_5) & (var_4 == p_var_4));
                     if(cond_level_8)
                     {
                        // here the "required" results are written (if any)
                        localCounter++;
                     }
                  }
               }
            }
         }
      }
      atomicIncrement(&(counter[i_var_3 % N_THREADS]), localCounter);
   }
}

__attribute__((noinline)) void parallel(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_4,
                                        PropertyId p_var_5, PropertyId p_var_7, PropertyId p_var_8, PropertyId p_var_9,
                                        size_t in_degree_var_2, Edge* var_2_1_inEdges)
{
   size_t i_var_3;
#pragma omp parallel for
   for(i_var_3 = 0; i_var_3 < in_degree_var_2; i_var_3++)
   {
      kernel(i_var_3, graph, var_2, p_var_3, p_var_4, p_var_5, p_var_7, p_var_8, p_var_9, in_degree_var_2,
             var_2_1_inEdges);
   }
}

__attribute__((noinline)) int search(Graph* graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_4,
                                     PropertyId p_var_5, PropertyId p_var_7, PropertyId p_var_8, PropertyId p_var_9)
{
   size_t in_degree_var_2 = getInDegree(graph, var_2);
#ifndef NDEBUG
   printf("In degree %d\n", in_degree_var_2);
#endif
   Edge* var_2_1_inEdges = getInEdges(graph, var_2);
   parallel(graph, var_2, p_var_3, p_var_4, p_var_5, p_var_7, p_var_8, p_var_9, in_degree_var_2, var_2_1_inEdges);
   for(int i = 0; i < N_THREADS; ++i)
      numAnswers += counter[i];
   return numAnswers;
}

int test(NodeId var_2, PropertyId p_var_3, PropertyId p_var_4, PropertyId p_var_5, PropertyId p_var_7,
         PropertyId p_var_8, PropertyId p_var_9)
{
#if defined(DATASETInVertexFile) && defined(DATASETOutVertexFile) && defined(DATASETInEdgeFile) && \
    defined(DATASETOutEdgeFile)
   loadGraph(DATASETInVertexFile, DATASETOutVertexFile, DATASETInEdgeFile, DATASETOutEdgeFile);
#else
   // loadGraph("dataset/40-InVertexFile.bin", "dataset/40-OutVertexFile.bin", "dataset/40-InEdgeFile.bin",
   // "dataset/40-OutEdgeFile.bin");
   loadGraph("dataset/1-InVertexFile.bin", "dataset/1-OutVertexFile.bin", "dataset/1-InEdgeFile.bin",
             "dataset/1-OutEdgeFile.bin");
#endif

   // var_2 = "<http://www.University0.edu>" 6231
   // p_var_3 = "ub:subOrganizationOf" 5
   // p_var_4 = "ub:Department" 11412
   // p_var_5 = "a" 14
   // p_var_7 = "ub:worksFor" 16
   // p_var_8 = "ub:FullProfessor" 21219
   // p_var_9 = "a" 14
   int ret_value = search(&TheGraph, var_2, p_var_3, p_var_4, p_var_5, p_var_7, p_var_8, p_var_9);

#ifndef NDEBUG
   printf("%d\n", ret_value);
#endif
   return ret_value;
}

#ifndef NDEBUG
int main()
{
   return test(7293, 14, 1828, 10, 2, 6764, 10) != 125;
}
#endif
