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

   /// filename for cosimulation
   std::string filename_bench;

   /// reference to the simulation tool
   SimulationToolRef sim_tool;
};

using SimulationInformationRef = refcount<SimulationInformation>;
using SimulationInformationConstRef = refcount<const SimulationInformation>;
#endif
