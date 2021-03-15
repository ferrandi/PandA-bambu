Change Log
==========

v0.2 (not yet released)
-----------------------

* Network interface:
    - `is_function` `#148 <https://github.com/lsils/mockturtle/pull/148>`_
    - `is_nary_and`, `is_nary_or`, `is_nary_xor` `#304 <https://github.com/lsils/mockturtle/pull/304>`_
* Framework for performing quality and performance experiments `#140 <https://github.com/lsils/mockturtle/pull/140>`_
* Algorithms:
    - CNF generation (`generate_cnf`) `#145 <https://github.com/lsils/mockturtle/pull/145>`_
    - SAT-based LUT mapping (`satlut_mapping`) `#122 <https://github.com/lsils/mockturtle/pull/122>`_
    - Miter generation (`miter`) `#148 <https://github.com/lsils/mockturtle/pull/148>`_
    - Combinational equivalence checking (`equivalence_checking`) `#149 <https://github.com/lsils/mockturtle/pull/149>`_
    - CNF based cut enumeration (`cnf_cut`) `#155 <https://github.com/lsils/mockturtle/pull/155>`_
    - Fast cut enumeration for small networks (`fast_small_cut_enumeration`, contributed by Sahand Kashani-Akhavan) `#161 <https://github.com/lsils/mockturtle/pull/161>`_
    - Shannon decomposition (`shannon_decomposition`) `#183 <https://github.com/lsils/mockturtle/pull/183>`_
    - Cleanup LUT networks (`cleanup_luts`) `#191 <https://github.com/lsils/mockturtle/pull/191>`_
    - Extract linear subcircuits in XAGs (`extract_linear_circuit` and `merge_linear_circuit`) `#204 <https://github.com/lsils/mockturtle/pull/204>`_
    - Linear resynthesis using Paar algorithm (`linear_resynthesis_paar`) `#211 <https://github.com/lsils/mockturtle/pull/211>`_
    - XAG optimization by computing transitive linear fanin `#232 <https://github.com/lsils/mockturtle/pull/232>`_
    - SAT-based satisfiability don't cares checker (`satisfiability_dont_cares_checker`) `#236 <https://github.com/lsils/mockturtle/pull/236>`_
    - XAG optimization based on satisfiability don't cares (`xag_dont_cares_optimization`) `#237 <https://github.com/lsils/mockturtle/pull/237>`_
    - XMG optimization based on satisfiability don't cares (`xmg_dont_cares_optimization`) `#239 <https://github.com/lsils/mockturtle/pull/239>`_
    - Create circuit based on spectral equivalence transformation sequences and NPN transformations (`apply_spectral_transformations` `apply_npn_transformations`) `#263 <https://github.com/lsils/mockturtle/pull/263>`_ `#301 <https://github.com/lsils/mockturtle/pull/301>`_
    - Exact linear resynthesis using SAT (`exact_linear_resynthesis`, `exact_linear_synthesis`) `#265 <https://github.com/lsils/mockturtle/pull/265>`_
    - XAG optimization by linear resynthesis (`linear_resynthesis_optimization`, `exact_linear_resynthesis_optimization`) `#296 <https://github.com/lsils/mockturtle/pull/296>`_
    - Davio decomposition (`positive_davio_decomposition`, `positive_davio_decomposition`) `#308 <https://github.com/lsils/mockturtle/pull/308>`_
    - Collapse network into single node per output network `#309 <https://github.com/lsils/mockturtle/pull/309>`_
    - Generic balancing algorithm `#340 <https://github.com/lsils/mockturtle/pull/340>`_
    - Check functional equivalence (`circuit_validator`) `#346 <https://github.com/lsils/mockturtle/pull/346>`_
* Views:
    - Assign names to signals and outputs (`names_view`) `#181 <https://github.com/lsils/mockturtle/pull/181>`_ `#184 <https://github.com/lsils/mockturtle/pull/184>`_
    - Creates a CNF while creating a network (`cnf_view`) `#274 <https://github.com/lsils/mockturtle/pull/274>`_
* I/O:
    - Write networks to DIMACS files for CNF (`write_dimacs`) `#146 <https://github.com/lsils/mockturtle/pull/146>`_
    - Read BLIF files using *lorina* (`blif_reader`) `#167 <https://github.com/lsils/mockturtle/pull/167>`_
    - Write networks to BLIF files (`write_blif`) `#169 <https://github.com/lsils/mockturtle/pull/169>`_ `#184 <https://github.com/lsils/mockturtle/pull/184>`_
    - Create circuit from integer index list (`create_from_binary_index_list`) `#259 <https://github.com/lsils/mockturtle/pull/259>`_
* Resynthesis functions:
    - Resynthesis function based on DSD decomposition (`dsd_resynthesis`) `#182 <https://github.com/lsils/mockturtle/pull/182>`_
    - Resynthesis function based on Shannon decomposition (`shannon_resynthesis`) `#185 <https://github.com/lsils/mockturtle/pull/185>`_
    - Resynthesis function based on Davio decomposition (`positive_davio_resynthesis`, `negative_davio_resynthesis`) `#308 <https://github.com/lsils/mockturtle/pull/308>`_
    - Exact resynthesis function for XMGs using XOR3 and majority gates (`exact_xmg_resynthesis`) `#328 <https://github.com/lsils/mockturtle/pull/328>`_
* Generators:
    - Sideways sum generator (`sideways_sum_adder`, contributed by Jovan Blanuša) `#159 <https://github.com/lsils/mockturtle/pull/159>`_
    - Carry lookahead adder (`carry_lookahead_adder_inplace`) `#171 <https://github.com/lsils/mockturtle/pull/171>`_
    - Improved modular multiplication (based on doubling `modular_multiplication_inplace`) `#174 <https://github.com/lsils/mockturtle/pull/174>`_
    - Modular doubling and halving (`modular_doubling_inplace` and `modular_halving_inplace`) `#174 <https://github.com/lsils/mockturtle/pull/174>`_ `#175 <https://github.com/lsils/mockturtle/pull/175>`_
    - Create modulus vector from hex string for modular arithmetic functions (`bool_vector_from_hex`) `#176 <https://github.com/lsils/mockturtle/pull/176>`_
    - Modular addition based on Hiasat and modular subtraction `#177 <https://github.com/lsils/mockturtle/pull/177>`_
    - Majority-9 networks (`majority5`, `majority7`, `majority9_12`, `majority9_13`) `#185 <https://github.com/lsils/mockturtle/pull/185>`_
    - Modular multiplication of Montgomery numbers (`montgomery_multiplication`) `#227 <https://github.com/lsils/mockturtle/pull/227>`_
    - Constant modular multiplication (`modular_constant_multiplier`) `#227 <https://github.com/lsils/mockturtle/pull/227>`_
    - Out-of-place modular addition, subtraction, and multiplication (`modular_adder`, `modular_subtractor`, `modular_multiplication`) `#234 <https://github.com/lsils/mockturtle/pull/234>`_
    - Create self-dualization of a logic network (`self_dualize_aig`) `#331 <https://github.com/lsils/mockturtle/pull/331>`_
    - Binary decoder (`binary_decoder`) `#342 <https://github.com/lsils/mockturtle/pull/342>`_
    - 2^k MUX (`binary_mux` and `binary_mux_klein_paterson`) `#342 <https://github.com/lsils/mockturtle/pull/342>`_
    - Random logic networks for XAGs (`random_logic_generator`) `#366 <https://github.com/lsils/mockturtle/pull/366>`_
* Properties:
    - Costs based on multiplicative complexity (`multiplicative_complexity` and `multiplicative_complexity_depth`) `#170 <https://github.com/lsils/mockturtle/pull/170>`_

v0.1 (March 31, 2019)
---------------------

* Initial network interface
  `#1 <https://github.com/lsils/mockturtle/pull/1>`_ `#61 <https://github.com/lsils/mockturtle/pull/61>`_ `#96 <https://github.com/lsils/mockturtle/pull/96>`_ `#99 <https://github.com/lsils/mockturtle/pull/99>`_
* Network implementations:
    - AIG network (`aig_network`) `#1 <https://github.com/lsils/mockturtle/pull/1>`_ `#62 <https://github.com/lsils/mockturtle/pull/62>`_
    - MIG network (`mig_network`) `#4 <https://github.com/lsils/mockturtle/pull/4>`_
    - k-LUT network (`klut_network`) `#1 <https://github.com/lsils/mockturtle/pull/1>`_
    - XOR-majority graph (`xmg_network`) `#47 <https://github.com/lsils/mockturtle/pull/47>`_
    - XOR-and graph (`xag_network`) `#79 <https://github.com/lsils/mockturtle/pull/79>`_
* Algorithms:
    - Cut enumeration (`cut_enumeration`) `#2 <https://github.com/lsils/mockturtle/pull/2>`_
    - LUT mapping (`lut_mapping`) `#7 <https://github.com/lsils/mockturtle/pull/7>`_
    - Akers synthesis (`akers_synthesis`) `#9 <https://github.com/lsils/mockturtle/pull/9>`_
    - Create LUT network from mapped network (`collapse_mapped_network`) `#13 <https://github.com/lsils/mockturtle/pull/13>`_
    - MIG algebraic depth rewriting (`mig_algebraic_depth_rewriting`) `#16 <https://github.com/lsils/mockturtle/pull/16>`_ `#58 <https://github.com/lsils/mockturtle/pull/58>`_
    - Cleanup dangling nodes (`cleanup_dangling`) `#16 <https://github.com/lsils/mockturtle/pull/16>`_
    - Node resynthesis (`node_resynthesis`) `#17 <https://github.com/lsils/mockturtle/pull/17>`_
    - Reconvergency-driven cut computation (`reconv_cut`) `#24 <https://github.com/lsils/mockturtle/pull/24>`_
    - Simulate networks (`simulate`) `#25 <https://github.com/lsils/mockturtle/pull/25>`_
    - Simulate node values (`simulate_nodes`) `#28 <https://github.com/lsils/mockturtle/pull/28>`_
    - Cut rewriting (`cut_rewriting`) `#31 <https://github.com/lsils/mockturtle/pull/31>`_
    - Refactoring (`refactoring`) `#34 <https://github.com/lsils/mockturtle/pull/34>`_
    - Exact resynthesis for node resynthesis, cut rewriting, and refactoring `#46 <https://github.com/lsils/mockturtle/pull/46>`_ `#71 <https://github.com/lsils/mockturtle/pull/71>`_
    - Boolean resubstitution (`resubstitution`) `#50 <https://github.com/lsils/mockturtle/pull/50>`_ `#54 <https://github.com/lsils/mockturtle/pull/54>`_ `#82 <https://github.com/lsils/mockturtle/pull/82>`_
    - Compute satisfiability don't cares (`satisfiability_dont_cares`) `#70 <https://github.com/lsils/mockturtle/pull/70>`_
    - Compute observability don't cares (`observability_dont_cares`) `#82 <https://github.com/lsils/mockturtle/pull/82>`_
    - Optimum XMG resynthesis for node resynthesis, cut rewriting, and refactoring `#86 <https://github.com/lsils/mockturtle/pull/86>`_
    - XMG algebraic depth rewriting (`xmg_algebraic_depth_rewriting`) `#86 <https://github.com/lsils/mockturtle/pull/86>`_
    - Convert gate-based networks to node-based networks (`gates_to_nodes`) `#90 <https://github.com/lsils/mockturtle/pull/90>`_
    - Direct resynthesis of functions into primitives (`direct_resynthesis`) `#90 <https://github.com/lsils/mockturtle/pull/90>`_
    - XAG optimum multiplicative complexity resynthesis (`xag_minmc_resynthesis`) `#100 <https://github.com/lsils/mockturtle/pull/100>`_
    - AIG/XAG resynthesis (`xag_npn_resynthesis`) `#102 <https://github.com/lsils/mockturtle/pull/102>`_
    - DSD decomposition (`dsd_decomposition`) `#137 <https://github.com/lsils/mockturtle/pull/137>`_
* Views:
    - Visit nodes in topological order (`topo_view`) `#3 <https://github.com/lsils/mockturtle/pull/3>`_
    - Disable structural modifications to network (`immutable_view`) `#3 <https://github.com/lsils/mockturtle/pull/3>`_
    - View for mapped networks (`mapping_view`) `#7 <https://github.com/lsils/mockturtle/pull/7>`_
    - View compute depth and node levels (`depth_view`) `#16 <https://github.com/lsils/mockturtle/pull/16>`_
    - Cut view (`cut_view`) `#20 <https://github.com/lsils/mockturtle/pull/20>`_
    - Access fanout of a node (`fanout_view`) `#27 <https://github.com/lsils/mockturtle/pull/27>`_ `#49 <https://github.com/lsils/mockturtle/pull/49>`_
    - Compute MFFC of a node (`mffc_view`) `#34 <https://github.com/lsils/mockturtle/pull/34>`_
    - Compute window around a node (`window_view`) `#41 <https://github.com/lsils/mockturtle/pull/41>`_
* I/O:
    - Read AIGER files using *lorina* (`aiger_reader`) `#6 <https://github.com/lsils/mockturtle/pull/6>`_
    - Read BENCH files using *lorina* (`bench_reader`) `#6 <https://github.com/lsils/mockturtle/pull/6>`_
    - Write networks to BENCH files (`write_bench`) `#10 <https://github.com/lsils/mockturtle/pull/10>`_
    - Read Verilog files using *lorina* (`verilog_reader`) `#40 <https://github.com/lsils/mockturtle/pull/40>`_
    - Write networks to Verilog files (`write_verilog`) `#65 <https://github.com/lsils/mockturtle/pull/65>`_
    - Read PLA files using *lorina* (`pla_reader`) `#97 <https://github.com/lsils/mockturtle/pull/97>`_
    - Write networks to DOT files (`write_dot`) `#111 <https://github.com/lsils/mockturtle/pull/111>`_
* Generators for arithmetic circuits:
    - Carry ripple adder (`carry_ripple_adder`) `#5 <https://github.com/lsils/mockturtle/pull/5>`_
    - Carry ripple subtractor (`carry_ripple_subtractor`) `#32 <https://github.com/lsils/mockturtle/pull/32>`_
    - Carry ripple multiplier (`carry_ripple_multiplier`) `#45 <https://github.com/lsils/mockturtle/pull/45>`_
    - Modular adder (`modular_adder_inplace`) `#43 <https://github.com/lsils/mockturtle/pull/43>`_
    - Modular subtractor (`modular_subtractor_inplace`) `#43 <https://github.com/lsils/mockturtle/pull/43>`_
    - Modular multiplication (`modular_multiplication_inplace`) `#48 <https://github.com/lsils/mockturtle/pull/48>`_
    - 2k-to-k multiplexer (`mux_inplace`) `#43 <https://github.com/lsils/mockturtle/pull/43>`_
    - Zero padding (`zero_extend`) `#48 <https://github.com/lsils/mockturtle/pull/48>`_
    - Random logic networks for AIGs and MIGs (`random_logic_generator`) `#68 <https://github.com/lsils/mockturtle/pull/68>`_
* Utility data structures: `truth_table_cache`, `cut`, `cut_set`, `node_map`, `progress_bar`, `stopwatch`
    - Truth table cache (`truth_table_cache`) `#1 <https://github.com/lsils/mockturtle/pull/1>`_
    - Cuts (`cut` and `cut_set`) `#2 <https://github.com/lsils/mockturtle/pull/2>`_
    - Container to associate values to nodes (`node_map`) `#13 <https://github.com/lsils/mockturtle/pull/13>`_ `#76 <https://github.com/lsils/mockturtle/pull/76>`_
    - Progress bar (`progress_bar`) `#30 <https://github.com/lsils/mockturtle/pull/30>`_
    - Tracking time of computations (`stopwatch`, `call_with_stopwatch`, `make_with_stopwatch`) `#35 <https://github.com/lsils/mockturtle/pull/35>`_
* Others:
    - Network events `#107 <https://github.com/lsils/mockturtle/pull/107>`_
    - MIG cost functions `#115 <https://github.com/lsils/mockturtle/pull/115>`_
