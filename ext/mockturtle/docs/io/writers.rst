Write into file formats
-----------------------

Write into BENCH files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_bench.hpp``

.. doxygenfunction:: mockturtle::write_bench(Ntk const&, std::string const&)

.. doxygenfunction:: mockturtle::write_bench(Ntk const&, std::ostream&)

Write into structural Verilog files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_verilog.hpp``

.. doxygenfunction:: mockturtle::write_verilog(Ntk const&, std::string const&)

.. doxygenfunction:: mockturtle::write_verilog(Ntk const&, std::ostream&)

Write into DIMACS files (CNF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_cnf.hpp``

.. doxygenfunction:: mockturtle::write_dimacs(Ntk const&, std::string const&)

.. doxygenfunction:: mockturtle::write_dimacs(Ntk const&, std::ostream&)

Write into DOT files (Graphviz)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_dot.hpp``

.. doxygenfunction:: mockturtle::write_dot(Ntk const&, std::string const&, Drawer const&)

.. doxygenfunction:: mockturtle::write_dot(Ntk const&, std::ostream&, Drawer const&)
