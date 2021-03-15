This directory includes an example showing how to integrate Python for design verification.

The main idea is that in addition to the xml, the user
pass to bambu some extra information that are used only in the
generation of the testbench and for the discrepancy analysis (not for
HLS). First the user has to encapsulate the python golden reference
function in a C file (sum.c in the example). The use of python
bindings must be all enclosed into the C wrapper function, so that
they are not parsed for HLS. The name and signature of the C wrapper
must be the same used in main.c to call the function. In this way,
when everything is linked together, the main() calls the C wrapper
which internally calls the python golden reference. Once the user has
setup the python golden reference and the C wrapper used to call it,
then it is possible to run bambu with the following options:

timeout 2h bambu main.c user_module.xml
  --C-python-no-parse=c_wrapper_for_golden.c
  --testbench-extra-gcc-options=" -I includedir/for/python2.7/ -lpython2.7 " --simulate

plus any other option controlling the bambu behavior.
This will start the simulation using the function contained in c_wrapper_for_golden_.c
instead of the HW module for which the user does not have the C code.
Here it is used python 2.7 but it should work also with python 3.4. The only thing
that matters is that the user passes the correct include directories and link
options as extra testbench options. Note that everything included in the
--testbench-extra-gcc-options is passed to the compiler when it
creates the executable used to create the testbench memory
initialization values and the software traces used for discrepancy
analysis. These flags are not passed to bambu when it builds the intermediate
representation for the high-level synthesis.

Here some further recommendations. First of all, in order to get it working
the user needs to install the i386 version of libpython. The reason is that
for simulation and discrepancy analysis bambu generates back the C
code, with python bindings. This code is compiled for 32bits
architecture, so if the user does not link the i386 version of the library the
executable will crash on python calls. On Ubuntu 14.04 LTS
the package name is libpython2.7:i386. 

Another recommendation is related to a requirements for 
discrepancy analysis on C+python designs. The the C wrapper function for 
the python golden reference has to be declared with this attribute:
__attribute__((no_sanitize_address)).
This is important because the discrepancy analysis enables the gcc address
sanitizer option. If the user does not add this attribute the sanitizer may
abort the program due to bugs coming from python, which are not
actually present in the user C code nor in the Verilog IP.
This is a measure to suppress false positives that may hide other real bugs.

