#ifndef SIMULATION_INFORMATION_HPP
#define SIMULATION_INFORMATION_HPP

#include "refcount.hpp"

#include <map>
#include <string>
#include <vector>

REF_FORWARD_DECL(SimulationTool);

class SimulationInformation
{
 public:
   /// every element of this vector maps the parameters of the top function
   //  to be tested onto strings representing their values for in a certain
   //  test vector
   std::vector<std::map<std::string, std::string>> test_vectors;

   /// for a given test vector index, this map gives the address map of the
   //  paramters of the top function to be tested
   std::map<unsigned int, std::map<unsigned int, unsigned int>> param_address;

   /// for a given test vector index, this map gives, for every parameter
   //  index, the total size of the memory reserved for it
   std::map<unsigned int, std::map<unsigned int, size_t>> param_mem_size;

   /// for a given test vector index, this map gives, for every parameter
   //  index, the offset (in bytes) of the position of the next aligned byte
   std::map<unsigned int, std::map<unsigned int, size_t>> param_next_off;

   /// filename for cosimulation
   std::string filename_bench;

   /// reference to the simulation tool
   SimulationToolRef sim_tool;

   /// name of the simulation script
   std::string generated_simulation_script;

   /// total number of executed simulations
   unsigned int n_testcases = 0;

   /// total number of cycles of the executed simulations
   unsigned long long int tot_n_cycles = 0;

   /// average number of cycles of the executed simulations
   unsigned long long avg_n_cycles = 0;

      /// total number of cycles of the executed simulations
      unsigned long long int tot_n_cycles = 0;

      /// average number of cycles of the executed simulations
      unsigned long long avg_n_cycles = 0;

      /// results available
      bool results_available = false;
};

typedef refcount<SimulationInformation> SimulationInformationRef;
typedef refcount<const SimulationInformation> SimulationInformationConstRef;
#endif
