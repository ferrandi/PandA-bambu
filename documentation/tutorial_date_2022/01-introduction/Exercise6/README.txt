Simple example describing how to integrate and verify existing IP with functions written in C that receives structs passed by pointers.

Hereafter a small description of files
--------------------------------------

top.c: file to be compiled/synthesized by bambu.
module_lib.h: header that declares the interfaces to existing Verilog IPs.
module_lib.xml: XML file that describes interfaces of existing Verilog IPs.
module1.v: verilog of an existing synthesizable IP.
module1.c: C stub used to emulate the module1 IP in C.
module2.v: verilog of an existing synthesizable IP.
module2.c: C stub used to emulate the module2 IP in C.
printer1.v: verilog of an existing non-synthesizable IP.
printer1.c: C stub used to emulate the printer1 IP in C.
printer2.v: verilog of an existing non-synthesizable IP.
printer2.c: C stub used to emulate the printer2 IP in C.
main_test.c: C testbench
constraints_STD.xml: resource constraint file passed to bambu to generate a Verilog design with just 1 my_ip module.
test.xml: XML file describing the testbench inputs. It is empty since we use the main_test.c as testbench generator.
bambu.sh: synthesis and simulation script. It requires Vivado RTL and Verilator to properly work.

All C/H files were validated using the "gcc -c" command.
A C executable can be created with this command: "gcc -o ip_test main_test.c top.c module1.c module2.c printer1.c printer2.c

