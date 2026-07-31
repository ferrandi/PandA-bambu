#include <cstdio>

#define GRAPHSAGE_VERTEX_COUNT 6
#define GRAPHSAGE_EDGE_COUNT 12
#define GRAPHSAGE_FEATURE_DIMENSION 3
#define GRAPHSAGE_OUTPUT_COUNT (GRAPHSAGE_VERTEX_COUNT * GRAPHSAGE_FEATURE_DIMENSION)

extern "C" void graphsage_mean(const int row_offsets[GRAPHSAGE_VERTEX_COUNT + 1],
                               const int neighbors[GRAPHSAGE_EDGE_COUNT], const int features[GRAPHSAGE_OUTPUT_COUNT],
                               int output[GRAPHSAGE_OUTPUT_COUNT]);

static int run_case(const char* name, const int row_offsets[GRAPHSAGE_VERTEX_COUNT + 1],
                    const int neighbors[GRAPHSAGE_EDGE_COUNT], const int features[GRAPHSAGE_OUTPUT_COUNT])
{
   int saved_rows[GRAPHSAGE_VERTEX_COUNT + 1];
   int saved_neighbors[GRAPHSAGE_EDGE_COUNT];
   int saved_features[GRAPHSAGE_OUTPUT_COUNT];
   int expected[GRAPHSAGE_OUTPUT_COUNT];
   int observed[GRAPHSAGE_OUTPUT_COUNT];
   for(int i = 0; i <= GRAPHSAGE_VERTEX_COUNT; ++i)
      saved_rows[i] = row_offsets[i];
   for(int i = 0; i < GRAPHSAGE_EDGE_COUNT; ++i)
      saved_neighbors[i] = neighbors[i];
   for(int i = 0; i < GRAPHSAGE_OUTPUT_COUNT; ++i)
   {
      saved_features[i] = features[i];
      observed[i] = -777;
   }

   // Independent sequential reference; no aggregation helper is shared with the kernel.
   for(int vertex = 0; vertex < GRAPHSAGE_VERTEX_COUNT; ++vertex)
   {
      const int degree = row_offsets[vertex + 1] - row_offsets[vertex];
      for(int feature = 0; feature < GRAPHSAGE_FEATURE_DIMENSION; ++feature)
      {
         int reference_sum = 0;
         for(int position = row_offsets[vertex]; position < row_offsets[vertex + 1]; ++position)
            reference_sum += features[neighbors[position] * GRAPHSAGE_FEATURE_DIMENSION + feature];
         expected[vertex * GRAPHSAGE_FEATURE_DIMENSION + feature] = degree ? reference_sum / degree : 0;
      }
   }
   graphsage_mean(row_offsets, neighbors, features, observed);

   int mismatches = 0;
   for(int vertex = 0; vertex < GRAPHSAGE_VERTEX_COUNT; ++vertex)
   {
      std::printf("%s vertex %d output:", name, vertex);
      for(int feature = 0; feature < GRAPHSAGE_FEATURE_DIMENSION; ++feature)
      {
         const int index = vertex * GRAPHSAGE_FEATURE_DIMENSION + feature;
         std::printf(" %d", observed[index]);
         if(observed[index] != expected[index])
         {
            std::printf("\n%s mismatch vertex=%d feature=%d expected=%d observed=%d\n", name, vertex, feature,
                        expected[index], observed[index]);
            ++mismatches;
         }
      }
      std::printf("\n");
   }
   for(int i = 0; i <= GRAPHSAGE_VERTEX_COUNT; ++i)
      if(row_offsets[i] != saved_rows[i])
      {
         std::printf("%s input modified: row_offsets[%d] expected=%d observed=%d\n", name, i, saved_rows[i],
                     row_offsets[i]);
         ++mismatches;
      }
   for(int i = 0; i < GRAPHSAGE_EDGE_COUNT; ++i)
      if(neighbors[i] != saved_neighbors[i])
      {
         std::printf("%s input modified: neighbors[%d] expected=%d observed=%d\n", name, i, saved_neighbors[i],
                     neighbors[i]);
         ++mismatches;
      }
   for(int i = 0; i < GRAPHSAGE_OUTPUT_COUNT; ++i)
      if(features[i] != saved_features[i])
      {
         std::printf("%s input modified: features[%d] expected=%d observed=%d\n", name, i, saved_features[i],
                     features[i]);
         ++mismatches;
      }
   return mismatches;
}

int main()
{
   const int features[GRAPHSAGE_OUTPUT_COUNT] = {0, 12, 24, 12, 24, 36, 24, 36, 48, 36, 48, 60, 48, 60, 72, 60, 72, 84};
   const int regular_rows[GRAPHSAGE_VERTEX_COUNT + 1] = {0, 2, 4, 6, 8, 10, 12};
   const int regular_neighbors[GRAPHSAGE_EDGE_COUNT] = {1, 2, 0, 2, 0, 1, 4, 5, 3, 5, 3, 4};
   const int irregular_rows[GRAPHSAGE_VERTEX_COUNT + 1] = {0, 1, 4, 6, 6, 10, 12};
   const int irregular_neighbors[GRAPHSAGE_EDGE_COUNT] = {1, 0, 2, 4, 1, 4, 0, 1, 2, 5, 2, 4};

   int mismatches = run_case("regular", regular_rows, regular_neighbors, features);
   mismatches += run_case("irregular", irregular_rows, irregular_neighbors, features);
   if(mismatches)
      std::printf("GraphSAGE verification failed with %d mismatch(es)\n", mismatches);
   else
      std::printf("GraphSAGE verification passed\n");
   return mismatches ? 1 : 0;
}
