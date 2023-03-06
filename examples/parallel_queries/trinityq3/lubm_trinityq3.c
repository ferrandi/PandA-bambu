#include <stdio.h>
#include "simple_API.h"

// var_2 = "ub:UndergraduateStudent"
// p_var_3 = "a"
// p_var_5 = "ub:University"
// p_var_6 = "a"
// p_var_8 = "ub:Department"
// p_var_9 = "a"
// p_var_10 = "ub:memberOf"
// p_var_11 = "ub:subOrganizationOf"
// p_var_12 = "ub:undergraduateDegreeFrom"

__attribute__((noinline))
void kernel(size_t i_var_3, Graph * graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5, PropertyId p_var_6, PropertyId p_var_8, PropertyId p_var_9, PropertyId p_var_10, PropertyId p_var_11, PropertyId p_var_12, size_t in_degree_var_2, Edge * var_2_1_inEdges)
{
  unsigned localCounter = 0;
  PropertyId var_3;  //corresponding to element having label "a"
  var_3 = var_2_1_inEdges[i_var_3].property;
  NodeId var_1; //corresponding to element having label "?X"
  var_1 = var_2_1_inEdges[i_var_3].node;
  int  cond_level_2 = (var_3 == p_var_3);
  if(cond_level_2)
  {
    size_t out_degree_var_1 = getOutDegree(graph, var_1);
    Edge * var_1_3_outEdges = getOutEdges(graph, var_1);
    size_t i_var_10;
    for(i_var_10=0; i_var_10 < out_degree_var_1; i_var_10++)
    {
      PropertyId var_10;  //corresponding to element having label "ub:memberOf"
      var_10 = var_1_3_outEdges[i_var_10].property;
      NodeId  var_7;   //corresponding to element having label "?Z"
      var_7 = var_1_3_outEdges[i_var_10].node;
      int  cond_level_4 = (var_10 == p_var_10);
      if(cond_level_4)
      {
        size_t out_degree_var_7 = getOutDegree(graph, var_7);
        Edge * var_7_5_outEdges = getOutEdges(graph, var_7);
        size_t i_var_9;
        for(i_var_9=0; i_var_9 < out_degree_var_7; i_var_9++)
        {
          PropertyId var_9;  //corresponding to element having label "a"
          var_9 = var_7_5_outEdges[i_var_9].property;
          NodeId  var_8;   //corresponding to element having label "ub:Department"
          var_8 = var_7_5_outEdges[i_var_9].node;
          int  cond_level_6 = ((var_9 == p_var_9) & (var_8 == p_var_8));
          if(cond_level_6)
          {
            Edge * var_1_7_outEdges = getOutEdges(graph, var_1);
            size_t i_var_12;
            for(i_var_12=0; i_var_12 < out_degree_var_1; i_var_12++)
            {
              PropertyId var_12;  //corresponding to element having label "ub:undergraduateDegreeFrom"
              var_12 = var_1_7_outEdges[i_var_12].property;
              NodeId  var_4;   //corresponding to element having label "?Y"
              var_4 = var_1_7_outEdges[i_var_12].node;
              int  cond_level_8 = (var_12 == p_var_12);
              if(cond_level_8)
              {
                size_t in_degree_var_4 = getInDegree(graph, var_4);
                Edge * var_4_9_inEdges = getInEdges(graph, var_4);
                size_t i_var_11;
                for(i_var_11=0; i_var_11 < in_degree_var_4; i_var_11++)
                {
                  PropertyId var_11;  //corresponding to element having label "ub:subOrganizationOf"
                  var_11 = var_4_9_inEdges[i_var_11].property;
                  NodeId var_7_10; //corresponding to element having label "?Z"
                  var_7_10 = var_4_9_inEdges[i_var_11].node;
                  int  cond_level_10 = ((var_11 == p_var_11) & (var_7_10 == var_7));
                  if(cond_level_10)
                  {
                    size_t out_degree_var_4 = getOutDegree(graph, var_4);
                    Edge * var_4_11_outEdges = getOutEdges(graph, var_4);
                    size_t i_var_6;
                    for(i_var_6=0; i_var_6 < out_degree_var_4; i_var_6++)
                    {
                      PropertyId var_6;  //corresponding to element having label "a"
                      var_6 = var_4_11_outEdges[i_var_6].property;
                      NodeId  var_5;   //corresponding to element having label "ub:University"
                      var_5 = var_4_11_outEdges[i_var_6].node;
                      int  cond_level_12 = ((var_6 == p_var_6) & (var_5 == p_var_5));
                      if(cond_level_12)
                      {
                        // here the "required" results are written (if any)
                        localCounter++;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    atomicIncrement(&(counter[i_var_3 % N_THREADS]), localCounter);
  }
}

__attribute__((noinline))
void parallel(Graph * graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5, PropertyId p_var_6, PropertyId p_var_8, PropertyId p_var_9, PropertyId p_var_10, PropertyId p_var_11, PropertyId p_var_12, size_t in_degree_var_2, Edge * var_2_1_inEdges)
{
   size_t i_var_3;
   #pragma omp parallel for
   for(i_var_3=0; i_var_3 < in_degree_var_2; i_var_3++)
   {
     kernel(i_var_3, graph, var_2, p_var_3, p_var_5, p_var_6, p_var_8, p_var_9, p_var_10, p_var_11, p_var_12, in_degree_var_2, var_2_1_inEdges);
   }
}

__attribute__((noinline))
int search(Graph * graph, NodeId var_2, PropertyId p_var_3, PropertyId p_var_5, PropertyId p_var_6, PropertyId p_var_8, PropertyId p_var_9, PropertyId p_var_10, PropertyId p_var_11, PropertyId p_var_12)
{
  size_t in_degree_var_2 = getInDegree(graph, var_2);
#ifndef NDEBUG
  printf("In degree %d\n", in_degree_var_2);
#endif
  Edge * var_2_1_inEdges = getInEdges(graph, var_2);
  parallel(graph, var_2, p_var_3, p_var_5, p_var_6, p_var_8, p_var_9, p_var_10, p_var_11, p_var_12, in_degree_var_2, var_2_1_inEdges);
  for (int i = 0; i < N_THREADS; ++i)
    numAnswers += counter[i];
  return numAnswers;
}


int test(NodeId var_2, PropertyId p_var_3, PropertyId p_var_5, PropertyId p_var_6, PropertyId p_var_8, PropertyId p_var_9, PropertyId p_var_10, PropertyId p_var_11, PropertyId p_var_12)
{
#if defined(DATASETInVertexFile) && defined(DATASETOutVertexFile) && defined(DATASETInEdgeFile) && defined(DATASETOutEdgeFile)
  loadGraph(DATASETInVertexFile, DATASETOutVertexFile, DATASETInEdgeFile, DATASETOutEdgeFile);
#else
  //loadGraph("dataset/40-InVertexFile.bin", "dataset/40-OutVertexFile.bin", "dataset/40-InEdgeFile.bin", "dataset/40-OutEdgeFile.bin");
  loadGraph("dataset/1-InVertexFile.bin", "dataset/1-OutVertexFile.bin", "dataset/1-InEdgeFile.bin", "dataset/1-OutEdgeFile.bin");
#endif

// var_2 = "ub:Undergraduate-Student" 14398
// p_var_3 = "a" 14
// p_var_5 = "ub:University" 11347
// p_var_6 = "a" 14
// p_var_8 = "ub:Department" 11412
// p_var_9 = "a" 14 
// p_var_10 = "ub:memberOf" 4
// p_var_11 = "ub:subOrganizationOf" 5
// p_var_12 = "ub:undergraduateDegreeFrom" 6 0
  int ret_value = search(&TheGraph, var_2, p_var_3, p_var_5, p_var_6, p_var_8, p_var_9, p_var_10, p_var_11, p_var_12);
#ifndef NDEBUG
  printf("%d\n", ret_value);
#endif
  return ret_value;
}

#ifndef NDEBUG
int main() {
  return test(25273, 10, 1685, 10, 1828, 10, 4, 14, 3) != 0;
  //return test(74, 14, 11347, 14, 11412, 14, 4, 5, 0);
}
#endif
