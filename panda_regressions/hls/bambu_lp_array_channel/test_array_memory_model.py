#!/usr/bin/env python3
"""Check that the generated synchronous array model holds q without a read."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
XML = ROOT / "etc/libtech/NC_TESTBENCH_IPs.xml"


def model_source():
    root = ET.parse(XML).getroot()
    cell = next(cell for cell in root.findall(".//cell")
                if cell.findtext("name") == "TestbenchArrayImpl")
    functionality = cell.find(".//NP_functionality[@IP_COMPONENT='if_utils']")
    if functionality is None:
        raise RuntimeError("TestbenchArrayImpl has no if_utils implementation")
    return functionality.attrib["VERILOG_PROVIDED"]


def main():
    for tool in ("vlib", "vlog", "vsim"):
        if not shutil.which(tool):
            raise RuntimeError(f"{tool} is required to simulate the extracted SystemVerilog model")
    env = os.environ.copy()
    for read_delay in (1, 2, 3):
        with tempfile.TemporaryDirectory(prefix=f"array-model-rd{read_delay}-") as temp:
            work = Path(temp)
            source = work / "array_model_tb.sv"
            source_text = (
            """module if_utils #(parameter index=0, BITSIZE=32) ();
reg [BITSIZE-1:0] mem [0:15];
function automatic [BITSIZE-1:0] read_a(input [3:0] address);
   read_a = mem[address];
endfunction
function automatic integer write_a(input [BITSIZE-1:0] data, input [3:0] address);
   begin mem[address] = data; write_a = 0; end
endfunction
endmodule

module TestbenchArrayImpl #(parameter index=0, WRITE_DELAY=1, READ_DELAY=2)
   (input clock, input setup_port, input [1:0] ce, input [1:0] we,
    input [7:0] address, input [63:0] d, output [63:0] q);
localparam PORTSIZE_ce=2, BITSIZE_ce=1, BITSIZE_we=1, BITSIZE_address=4,
           BITSIZE_d=32, BITSIZE_q=32;
typedef logic [BITSIZE_address-1:0] ptr_t;
""" + model_source() + """
endmodule

module tb_array_model;
localparam READ_DELAY=READ_DELAY_PLACEHOLDER;
reg clock=0, setup_port=0;
reg [1:0] ce=0, we=0;
reg [7:0] address=0;
reg [63:0] d=0;
wire [63:0] q;
TestbenchArrayImpl #(.READ_DELAY(READ_DELAY)) dut(clock, setup_port, ce, we, address, d, q);
always #5 clock = ~clock;
task check_q(input [31:0] expected0, input [31:0] expected1, input [255:0] label);
   begin
      if(q[31:0] !== expected0 || q[63:32] !== expected1) begin
         $display("FAIL %0s q0=%0d q1=%0d", label, q[31:0], q[63:32]);
         $fatal(1);
      end
   end
endtask
task wait_for_read;
   begin
      if(READ_DELAY == 1) #1;
      else begin repeat(READ_DELAY-1) @(posedge clock); #1; end
   end
endtask
initial begin
   dut.m_utils.mem[2] = 32'h10203040;
   dut.m_utils.mem[3] = 32'h50607080;
   dut.m_utils.mem[4] = 32'h11112222;
   dut.m_utils.mem[5] = 32'h33334444;
   repeat(2) @(posedge clock);

   @(negedge clock); ce=2'b11; we=0; address={4'd3,4'd2};
   if(READ_DELAY == 3) begin
      @(posedge clock); #1; check_q(0, 0, "read result before latency");
      @(posedge clock); #1;
   end
   else wait_for_read();
   check_q(32'h10203040, 32'h50607080, "simultaneous reads");

   @(negedge clock); ce=2'b01; address[3:0]=4'd4;
   wait_for_read(); check_q(32'h11112222, 32'h50607080, "port zero read");
   @(negedge clock); address[3:0]=4'd5;
   wait_for_read(); check_q(32'h33334444, 32'h50607080, "subsequent port zero read");

   @(negedge clock); ce=0; address={4'd9,4'd8};
   repeat(6) begin @(posedge clock); #1; check_q(32'h33334444, 32'h50607080, "CE-low hold"); end

   @(negedge clock); ce=2'b01; we=2'b01; address[3:0]=4'd5; d[31:0]=32'hdeadbeef;
   @(posedge clock); #1;
   check_q(32'h33334444, 32'h50607080, "write does not change q");
   if(dut.m_utils.mem[5] !== 32'hdeadbeef) begin $display("FAIL write did not reach memory"); $fatal(1); end

   @(negedge clock); ce=0; we=0;
   repeat(3) begin @(posedge clock); #1; check_q(32'h33334444, 32'h50607080, "post-write hold"); end
   $display("PASS array model READ_DELAY=%0d q hold and independent port reads", READ_DELAY);
   $finish;
end
endmodule
"""
            ).replace("READ_DELAY_PLACEHOLDER", str(read_delay))
            source.write_text(source_text, encoding="utf-8")
            compile_result = subprocess.run(
                ["vlib", str(work / "work")],
                cwd=work, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
            )
            if compile_result.returncode:
                raise RuntimeError(f"vlib failed ({compile_result.returncode}):\n{compile_result.stdout}")
            compile_result = subprocess.run(
                ["vlog", "-quiet", "-sv", "-work", str(work / "work"), str(source)],
                cwd=work, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
            )
            if compile_result.returncode:
                raise RuntimeError(f"vlog failed ({compile_result.returncode}):\n{compile_result.stdout}")
            simulation = subprocess.run(
                ["vsim", "-c", "-quiet", "-lib", str(work / "work"), "tb_array_model",
                 "-do", "run -all; quit -f"], cwd=work, env=env, text=True,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
            )
            if simulation.returncode or f"PASS array model READ_DELAY={read_delay} q hold" not in simulation.stdout:
                raise RuntimeError(f"array model simulation failed ({simulation.returncode}):\n{simulation.stdout}")
            print(simulation.stdout.strip())


if __name__ == "__main__":
    main()
