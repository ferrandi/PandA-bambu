Modular arithmetic networks
---------------------------

The file ``mockturtle/generators/modular_arithmetic.hpp`` implements several
functions to generate modular arithmetic networks.

Addition and Subtraction
~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: mockturtle::modular_adder_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&)
.. doxygenfunction:: mockturtle::modular_adder_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)
.. doxygenfunction:: mockturtle::modular_subtractor_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&)
.. doxygenfunction:: mockturtle::modular_subtractor_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)

Multiplication
~~~~~~~~~~~~~~

.. doxygenfunction:: modular_multiplication_inplace(Ntk& ntk, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)
