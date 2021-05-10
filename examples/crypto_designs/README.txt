This example starts from the reference C description of Keccak crypto function distributed through this website http://keccak.noekeon.org/.
Keccak has been selected by NIST to become the new SHA-3 standard (see http://www.nist.gov/hash-competition and http://ehash.iaik.tugraz.at/wiki/The_SHA-3_Zoo).
Further details can be found at: http://ehash.iaik.tugraz.at/wiki/Keccak.
Together with the C implementation optimized for processors, there exist several implementations for FPGA and ASIC.
So, as a referenced it has been selected two implementations developed by the authors of the Keccak algorithm (i.e., Guido Bertoni-STMicroelectronics, Joan Daemen-STMicroelectronics, Michaël Peeters-NXP Semiconductors and Gilles Van Assche-STMicroelectronics) which use external system memory.

The results reported at this link http://ehash.iaik.tugraz.at/wiki/SHA-3_Hardware_Implementations are:
Altera Cyclone III 1559LEs 47.8Mbit/s 181 MHz
Xilinx Virtex 5 444slices 70.1Mbit/s 265 MHz

After two days of hacking of the reference C code and after some design space exploration 5 different alternatives using different FPGAs have been obtained:

Altera Cyclone II 7049LEs 152.0Mbit/s 114MHz (directory keccak_CycloneII_10ns)
Altera Cyclone II 7434LEs 272.8Mbit/s 209MHz (directory keccak_CycloneII_4ns)
Lattice ECP3 4038slices 151.7Mbit/s 114MHz (directory keccak_ECP3_9ns)
Xilinx Virtex 5 1699slices 316.5Mbit/s 252MHz (directory keccak_V5_4ns)
Xilinx Virtex 7 1887slices 529.8Mbit/s 406MHz (directory keccak_V7_2ns)

These results have been obtained with PandA framework 0.9.5 and GCC version 6.0.0.

Along with this example comes another one showing how it is possible to build an Autotools based project for the high-level synthesis with bambu: directory crypto_designs/multi-keccak.

===================================================

New results obtained with PandA framework 0.9.7-dev and Clang version 11.
Altera Cyclone II 6818LEs 2844Mbit/s 180MHz 
Xilinx Virtex 7 1634slices 6930Mbit/s 333MHz 
