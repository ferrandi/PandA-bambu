#include <memory>

#ifndef CLOCK_PORT_NAME
#error CLOCK_PORT_NAME must be defined
#endif

#ifndef CLOCK_PERIOD
#define CLOCK_PERIOD 2
#endif

#define HALF_CLOCK_PERIOD (CLOCK_PERIOD / 2)

#include <verilated.h>
#if VM_TRACE
#include <verilated_vcd_c.h>
#ifndef VCD_OUT_FILENAME
#error VCD_OUT_FILENAME must be defined
#endif
#endif

#include "Vbambu_testbench.h"

vluint64_t main_time = 0;

double sc_time_stamp()
{
   return main_time;
}

int main(int argc, char** argv, char** env)
{
   Verilated::commandArgs(argc, argv);
   Verilated::debug(0);

   const std::unique_ptr<Vbambu_testbench> top{new Vbambu_testbench{"clocked_bambu_testbench"}};

   main_time = 0;
#if VM_TRACE
   Verilated::traceEverOn(true);
   const std::unique_ptr<VerilatedVcdC> tfp{new VerilatedVcdC};
   top->trace(tfp.get(), 99);
   tfp->set_time_unit("p");
   tfp->set_time_resolution("p");
   tfp->open(VCD_OUT_FILENAME);
#endif
   top->CLOCK_PORT_NAME = 1;
   while(!Verilated::gotFinish())
   {
      top->CLOCK_PORT_NAME = !top->CLOCK_PORT_NAME;
      top->eval();
#if VM_TRACE
      tfp->dump(main_time);
#endif
      main_time += HALF_CLOCK_PERIOD;
   }
#if VM_TRACE
   tfp->dump(main_time);
   tfp->close();
#endif
   top->final();

   return 0;
}