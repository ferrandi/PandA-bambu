Bi-decomposition
----------------

**Header:** ``mockturtle/algorithms/bi_decomposition.hpp``

The algorithm implements a method for 2-input gates logic synthesis using the bi-decomposition algorithm proposed in
"*An algorithm for bi-decomposition of logic functions*" by A. Mishchenko, B. Steinbach, and M. Perkowski (DAC, 2001).


This method is implemented in the function `bi_decomposition`, which takes as
input a function represented as truth table with possible care set (represented as truth table). 

An example to run bi-decomposition algorithm for the synthesis of a logic network is by doing:

.. code-block:: c++

         xag_network ntk;
         std::vector<xag_network::signal> pis( 4u );
         std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
         ntk.create_po( bi_decomposition( ntk, func, care, pis ) );

Here, the function returns a signal of the XAG and performs the synthesis of the function considering as inputs the signals in the vector pis. 'func' and 'care' are the functionality and the care set, respectively. 

.. doxygenfunction:: mockturtle::bi_decomposition(Ntk&, kitty::dynamic_truth_table const&, kitty::dynamic_truth_table const&, std::vector<signal<Ntk>> const&)

