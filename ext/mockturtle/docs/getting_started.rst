Getting started
===============

mockturtle is a header-only C++-17 library. It can be compiled as a stand-alone
tool for direct usage, or be integrated in another project as a library.

Compilation requirements
------------------------

Support of C++ 17 standard is required to compile mockturtle.
We constantly test building mockturtle on Linux using Clang 11, GCC 7, GCC 9
and GCC 10, and on Mac OS using Clang 12, GCC 9 and GCC 10.  It also 
compiles on Windows using the C++ compiler in Visual Studio 2019.

Using mockturtle as a stand-alone tool
--------------------------------------

mockturtle can be compiled and used as a stand-alone logic synthesis tool.
Compilation configuration can be easily done with CMake. For example, to 
configure, compile and run the example code in our GitHub page::

  mkdir build
  cd build
  cmake ..
  make cut_enumeration
  ./examples/cut_enumeration

Please note that CMake version >= 3.8 is required. For a more interactive
interface, you could also use ``ccmake``.

If you experience that the system compiler does not suffice the requirements,
you can manually pass a compiler to CMake using::

  cmake -DCMAKE_CXX_COMPILER=/path/to/c++-compiler ..

To write your own code to run mockturtle algorithms, simply create a new cpp
file in the examples directory, and then re-configure with CMake::

  cd build
  cmake .

Then, a make target of your file name should become available.

For most of the algorithms, there is a corresponding experiment code in the 
experiments directory, which demonstrates how the algorithm can be called.
To compile the experiments, you need to turn on the ``MOCKTURTLE_EXPERIMENTS``
option in CMake::

  cmake -DMOCKTURTLE_EXPERIMENTS=ON ..

Note that many of the experiments check circuit equivalence with a system-command
call to ABC_. If you see ``abc: command not found``, you could either install ABC_
and add it to the PATH variable, or ignore this error as it does not affect the
operation of the algorithms.

.. _ABC: https://github.com/berkeley-abc/abc

Using mockturtle as a library in another project
------------------------------------------------

Being header-only, mockturtle can be easily integrated into existing and new projects.
Just add the include directory of mockturtle to your include directories, and simply
include mockturtle by

.. code-block:: c++

   #include <mockturtle/mockturtle.hpp>

Some external projects using mockturtle can be found on our showcase_ page.

.. _showcase: https://github.com/lsils/lstools-showcase#external-projects-using-the-epfl-logic-synthesis-libraries

Building tests
--------------

In order to run the tests, you need to init the submodules and enable tests
in CMake::

  mkdir build
  cd build
  cmake -DMOCKTURTLE_TEST=ON ..
  make run_tests
  ./test/run_tests
