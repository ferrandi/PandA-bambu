#include "graphsage_mean_test_common.hpp"
extern "C" void graphsage_mean_serial(const int*, const int*, const int*, int*);
int main() { return run_graphsage_suite(graphsage_mean_serial); }
