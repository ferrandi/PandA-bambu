Write into file formats
-----------------------

Write into AIGER files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_aiger.hpp``

.. doxygenfunction:: mockturtle::write_aiger(Ntk const&, std::string const&)

.. doxygenfunction:: mockturtle::write_aiger(Ntk const&, std::ostream&)

Write into BENCH files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_bench.hpp``

.. doxygenfunction:: mockturtle::write_bench(Ntk const&, std::string const&)

.. doxygenfunction:: mockturtle::write_bench(Ntk const&, std::ostream&)

Write into BLIF files
~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_blif.hpp``

.. doxygenfunction:: mockturtle::write_blif(Ntk const&, std::string const&, write_blif_params const&)

.. doxygenfunction:: mockturtle::write_blif(Ntk const&, std::ostream&, write_blif_params const&)

Write into structural Verilog files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_verilog.hpp``

.. doxygenfunction:: mockturtle::write_verilog(Ntk const&, std::string const&, write_verilog_params const&)

.. doxygenfunction:: mockturtle::write_verilog(Ntk const&, std::ostream&, write_verilog_params const&)

.. doxygenfunction:: mockturtle::write_verilog_with_binding(Ntk const&, std::ostream&, write_verilog_params const&)

Write into DIMACS files (CNF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_cnf.hpp``

.. doxygenfunction:: mockturtle::write_dimacs(Ntk const&, std::string const&)

.. doxygenfunction:: mockturtle::write_dimacs(Ntk const&, std::ostream&)

.. _write_dot:

Write into DOT files (Graphviz)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_dot.hpp``

.. doxygenfunction:: mockturtle::write_dot(Ntk const&, std::string const&, Drawer const&)

.. doxygenfunction:: mockturtle::write_dot(Ntk const&, std::ostream&, Drawer const&)

Write simulation patterns into file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_patterns.hpp``

.. doxygenfunction:: mockturtle::write_patterns(Simulator const&, std::string const&)

.. doxygenfunction:: mockturtle::write_patterns(Simulator const&, std::ostream&)

Write library into GENLIB file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/io/write_genlib.hpp``

.. doxygenfunction:: mockturtle::write_genlib(std::vector<gate> const&, std::string const&)

.. doxygenfunction:: mockturtle::write_genlib(std::vector<gate> const&, std::ostream&)
