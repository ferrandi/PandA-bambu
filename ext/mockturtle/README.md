[![Actions Status](https://github.com/lsils/mockturtle/workflows/Linux%20CI/badge.svg)](https://github.com/lsils/mockturtle/actions)
[![Actions Status](https://github.com/lsils/mockturtle/workflows/MacOS%20CI/badge.svg)](https://github.com/lsils/mockturtle/actions)
[![Actions Status](https://github.com/lsils/mockturtle/workflows/Windows%20CI/badge.svg)](https://github.com/lsils/mockturtle/actions)
[![Coverage Status](https://codecov.io/gh/lsils/mockturtle/branch/master/graph/badge.svg?token=KSC1MP2VCM)](https://codecov.io/gh/lsils/mockturtle)
[![Documentation Status](https://readthedocs.org/projects/mockturtle/badge/?version=latest)](http://mockturtle.readthedocs.io/en/latest/?badge=latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# mockturtle

<img src="https://cdn.jsdelivr.net/gh/lsils/mockturtle@master/mockturtle.svg" width="64" height="64" align="left" style="margin-right: 12pt" />
mockturtle is a C++-17 logic network library.  It provides several logic
network implementations (such as And-inverter graphs, Majority-inverter graphs,
and k-LUT networks), and generic algorithms for logic synthesis and logic
optimization.

[Read the full documentation.](http://mockturtle.readthedocs.io/en/latest/?badge=latest)

## Example

The following code snippet reads an AIG from an Aiger file, enumerates all cuts
and prints them for each node.

```c++
#include <mockturtle/mockturtle.hpp>
#include <lorina/aiger.hpp>

mockturtle::aig_network aig;
auto const result = lorina::read_aiger( "file.aig", mockturtle::aiger_reader( aig ) );
assert( result == lorina::return_code::success );

auto const cuts = cut_enumeration( aig );
aig.foreach_node( [&]( auto node ) {
  std::cout << cuts.cuts( aig.node_to_index( node ) ) << "\n";
} );
```

## Installation requirements

A modern compiler is required to build *mockturtle*.  We are continuously
testing with Clang 12.0.1, GCC 9.3.0, and GCC 10.2.0.  More information can be
found in the [documentation](http://mockturtle.readthedocs.io/en/latest/getting_started.html).

## EPFL logic synthesis libraries

mockturtle is part of the [EPFL logic synthesis](https://lsi.epfl.ch/page-138455-en.html) libraries.  The other libraries and several examples on how to use and integrate the libraries can be found in the [logic synthesis tool showcase](https://github.com/lsils/lstools-showcase).
