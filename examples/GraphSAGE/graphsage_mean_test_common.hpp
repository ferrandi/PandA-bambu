#ifndef GRAPHSAGE_MEAN_TEST_COMMON_HPP
#define GRAPHSAGE_MEAN_TEST_COMMON_HPP
#include <cstdio>
#include "graphsage_constants.hpp"
typedef void (*graphsage_kernel)(const int*, const int*, const int*, int*);
static void print_vector(const char* key, const int* values, int count) {
   std::printf("|%s=", key);
   for(int i = 0; i < count; ++i) std::printf("%s%d", i ? "," : "", values[i]);
}
static int run_case(const char* name, const int rows[7], const int neighbors[12],
                    const int features[18], graphsage_kernel kernel) {
   int saved_rows[7], saved_neighbors[12], saved_features[18], expected[18], observed[18];
   for(int i = 0; i < 7; ++i) saved_rows[i] = rows[i];
   for(int i = 0; i < 12; ++i) saved_neighbors[i] = neighbors[i];
   for(int i = 0; i < 18; ++i) { saved_features[i] = features[i]; observed[i] = -777; }
   // Independent sequential golden implementation; no aggregation code is shared with either kernel.
   for(int vertex = 0; vertex < GRAPHSAGE_VERTEX_COUNT; ++vertex) {
      const int degree = rows[vertex + 1] - rows[vertex];
      for(int feature = 0; feature < GRAPHSAGE_FEATURE_DIMENSION; ++feature) {
         int sum = 0;
         for(int edge = rows[vertex]; edge < rows[vertex + 1]; ++edge)
            sum += features[neighbors[edge] * GRAPHSAGE_FEATURE_DIMENSION + feature];
         expected[vertex * GRAPHSAGE_FEATURE_DIMENSION + feature] = degree == 0 ? 0 : sum / degree;
      }
   }
   kernel(rows, neighbors, features, observed);
   int mismatches = 0, immutable = 1;
   for(int i = 0; i < 18; ++i) mismatches += observed[i] != expected[i];
   for(int i = 0; i < 7; ++i) immutable &= rows[i] == saved_rows[i];
   for(int i = 0; i < 12; ++i) immutable &= neighbors[i] == saved_neighbors[i];
   for(int i = 0; i < 18; ++i) immutable &= features[i] == saved_features[i];
   std::printf("GRAPHSAGE_CASE|id=%s", name);
   print_vector("rows", rows, 7); print_vector("neighbors", neighbors, 12);
   print_vector("features", features, 18); print_vector("golden", expected, 18);
   print_vector("observed", observed, 18);
   std::printf("|count=18|mismatches=%d|inputs_immutable=%d\n", mismatches, immutable);
   return mismatches + (immutable ? 0 : 1);
}
static int run_graphsage_suite(graphsage_kernel kernel) {
   const int positive[18] = {0,12,24,12,24,36,24,36,48,36,48,60,48,60,72,60,72,84};
   const int signed_values[18] = {-8,5,-2,7,-4,11,-5,-8,3,10,1,-7,-11,14,-5,4,-13,8};
   const int regular_rows[7] = {0,2,4,6,8,10,12};
   const int regular_neighbors[12] = {1,2,0,2,0,1,4,5,3,5,3,4};
   const int irregular_rows[7] = {0,1,4,6,6,10,12};
   const int irregular_neighbors[12] = {1,0,2,4,1,4,0,1,2,5,2,4};
   const int negative_rows[7] = {0,3,4,6,8,9,12};
   const int negative_neighbors[12] = {1,2,4,0,1,5,2,4,3,0,3,5};
   const int duplicate_rows[7] = {0,1,4,6,9,9,12};
   const int duplicate_neighbors[12] = {0,2,2,1,2,3,3,3,5,0,4,5};
   int failures = run_case("regular", regular_rows, regular_neighbors, positive, kernel);
   failures += run_case("irregular-zero-degree", irregular_rows, irregular_neighbors, positive, kernel);
   failures += run_case("negative-signed-division", negative_rows, negative_neighbors, signed_values, kernel);
   failures += run_case("duplicate-self-mixed", duplicate_rows, duplicate_neighbors, signed_values, kernel);
   std::printf("GraphSAGE verification %s with %d failure(s)\n", failures ? "failed" : "passed", failures);
   return failures ? 1 : 0;
}
#endif
