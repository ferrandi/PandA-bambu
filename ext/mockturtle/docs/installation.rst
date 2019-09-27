Installation
============

mockturtle is a header-only C++-17 library.  Just add the include directory
of mockturtle to your include directories, and you can integrate mockturte into
your source files using

.. code-block:: c++

   #include <mockturtle/mockturtle.hpp>

Requirements
------------

We tested building mockturtle on Mac OS and Linux using Clang 6.0.0, GCC 7.3.0,
and GCC 8.1.0.  It also compiles on Windows using the C++ compiler in Visual
Studio 2017.

If you experience that the system compiler does not suffice the requirements,
you can manually pass a compiler to CMake using::

  cmake -DCMAKE_CXX_COMPILER=/path/to/c++-compiler ..

Building tests
--------------

In order to run the tests, you need to init the submodules and enable tests
in CMake::

  mkdir build
  cd build
  cmake -DMOCKTURTLE_TEST=ON ..
  make
  ./test/run_tests
