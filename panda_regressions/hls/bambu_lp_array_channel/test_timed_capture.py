#!/usr/bin/env python3
"""Cycle-accurate checks for the TIMED_CAPTURE models in NC_STD_IPs.xml."""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[3]
LATENCIES = (0, 1, 2, 3, 15)


def run(command, cwd, env):
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(map(str, command))}\n{result.stdout}"
        )
    return result.stdout


def resolve_modelsim_bin(env):
    vsim = shutil.which("vsim", path=env.get("PATH"))
    if vsim:
        return str(Path(vsim).parent)

    for root in env.get("BAMBU_HLS_BACKEND_PATH", "").split(os.pathsep):
        if not root:
            continue
        result = subprocess.run(
            ["find", "-L", root, "-type", "f", "-path", "*/bin/vsim", "-print", "-quit"],
            text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
        )
        if result.stdout.strip():
            return str(Path(result.stdout.strip()).parent)
    raise SystemExit("ModelSim vsim not found in PATH or BAMBU_HLS_BACKEND_PATH")


def extract_models(directory):
    technology = ET.parse(ROOT / "etc/libtech/NC_STD_IPs.xml").getroot()
    cell = next(cell for cell in technology.iter("cell") if cell.findtext("name") == "TIMED_CAPTURE")
    model = cell.find(".//NP_functionality")

    verilog = model.get("VERILOG_PROVIDED").replace("1INIT_ZERO_VALUE", "= 0")
    verilog = verilog.replace("1RESET_VALUE", "(reset == 1'b0)")
    (directory / "timed_capture.v").write_text(
        """module TIMED_CAPTURE #(parameter integer LATENCY=0, parameter integer CAPACITY=1,
  parameter integer BITSIZE_out1=8) (
  input wire clock, reset, launch, consume,
  input wire [BITSIZE_out1-1:0] in1,
  output wire [BITSIZE_out1-1:0] out1
);
""" + verilog + "\nendmodule\n"
    )
    vhdl = model.get("VHDL_PROVIDED").replace("1INIT_ZERO_VALUE", ":= (others => '0')")
    vhdl = vhdl.replace("1RESET_VALUE", "(reset = '0')")
    (directory / "timed_capture.vhd").write_text(
        """library ieee;
use ieee.std_logic_1164.all;
entity TIMED_CAPTURE is
  generic (LATENCY : natural := 0; CAPACITY : positive := 1; BITSIZE_out1 : positive := 8);
  port (clock, reset, launch, consume : in std_logic;
        in1 : in std_logic_vector(BITSIZE_out1-1 downto 0);
        out1 : out std_logic_vector(BITSIZE_out1-1 downto 0));
end entity;
architecture rtl of TIMED_CAPTURE is
""" + vhdl + "\nend architecture;\n"
    )


def positive_integer(value):
    try:
        parsed = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("parallelism must be a positive integer") from error
    if parsed <= 0:
        raise argparse.ArgumentTypeError("parallelism must be a positive integer")
    return parsed


def resolve_parallelism(argv=None, environ=None):
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("-j", "--parallel", action="append", type=positive_integer)
    args, _ = parser.parse_known_args(argv)
    env = os.environ if environ is None else environ
    default = positive_integer(env.get("J", "1"))
    return min(args.parallel) if args.parallel else default


VERILOG_TB = r'''module tb;
parameter integer L=2;
parameter integer C=1;
reg clock=0;
always #5 clock=~clock;
reg reset=0, launch=0, consume=0;
reg [7:0] in1=0;
wire [7:0] out1;
integer i, n, q_before, q_after;
reg [7:0] last_value;
TIMED_CAPTURE #(.LATENCY(L), .CAPACITY(C), .BITSIZE_out1(8)) dut(
  .clock(clock), .reset(reset), .launch(launch), .consume(consume), .in1(in1), .out1(out1));

task check;
  input [7:0] expected;
  input [8*40-1:0] phase;
  begin
    if (out1 !== expected)
      $fatal(1, "L=%0d phase=%0s time=%0t got=%h expected=%h",
             L, phase, $time, out1, expected);
  end
endtask

task multi_entry_queue;
  begin
    if (L != 3 || C != 3) $fatal(1, "queue test requires L=3 and C=3");
    // Four consecutive launches leave three completed results queued while the consumer stalls.
    for (n=0; n<4; n=n+1) begin
      @(negedge clock); launch=1; in1=(n == 3) ? 8'h61 : 8'he1;
      if (n == 3) begin #1; check(8'h61, "empty-queue capture bypass"); end
      @(posedge clock); #1;
    end
    @(negedge clock); launch=0; in1=8'h72; #1; check(8'h61, "first queued result held");
    @(posedge clock); #1;
    @(negedge clock); in1=8'h83; #1; check(8'h61, "second result enqueued");
    @(posedge clock); #1;
    // Queue is full: consume the old head as the fourth capture arrives.
    @(negedge clock); in1=8'h94; consume=1; #1; check(8'h61, "simultaneous full pop/push head");
    @(posedge clock); #1;
    @(negedge clock); in1=8'he1; #1; check(8'h72, "ordered drain result 1");
    @(posedge clock); #1;
    @(negedge clock); #1; check(8'h83, "ordered drain result 2");
    @(posedge clock); #1;
    @(negedge clock); #1; check(8'h94, "ordered drain result 3 after wrap");
    @(posedge clock); #1; consume=0;

    // A pre-reset token must not enter the newly emptied queue ahead of a fresh result.
    @(negedge clock); launch=1; in1=8'he1;
    @(posedge clock); #1;
    @(negedge clock); launch=0; in1=8'he1;
    @(posedge clock); #1;
    @(negedge clock); reset=0;
    @(posedge clock); #1;
    @(negedge clock); reset=1; launch=1;
    @(posedge clock); #1;
    @(negedge clock); launch=0; in1=8'he1;
    repeat (L-1) @(posedge clock);
    @(negedge clock); in1=8'ha5; #1; check(8'ha5, "fresh result bypass after reset");
    @(posedge clock); #1;
    @(negedge clock); consume=1; in1=8'he1; #1; check(8'ha5, "fresh result after reset");
    @(posedge clock); #1; consume=0;

    // Empty-queue consume bypass must not leave a duplicate ahead of the next result.
    @(negedge clock); launch=1; in1=8'he1;
    @(posedge clock); #1;
    @(negedge clock); launch=0; in1=8'he1;
    repeat (L-1) @(posedge clock);
    @(negedge clock); launch=1; consume=1; in1=8'hb6; #1;
    check(8'hb6, "empty simultaneous capture/consume bypass");
    @(posedge clock); #1; consume=0;
    @(negedge clock); launch=0; in1=8'he1;
    repeat (L-1) @(posedge clock);
    @(negedge clock); in1=8'hc7; #1; check(8'hc7, "next result after empty bypass");
    @(posedge clock); #1;
    @(negedge clock); consume=1; in1=8'he1; #1; check(8'hc7, "next result is not preceded by duplicate");
    @(posedge clock); #1; consume=0;
  end
endtask

function [7:0] result_for;
  input integer index;
  begin
    case (index)
      0: result_for=8'h61;
      1: result_for=8'h72;
      default: result_for=8'h83;
    endcase
  end
endfunction

task single_operation;
  input [7:0] prior, result, poison;
  begin
    if (L == 0) begin
      @(negedge clock); launch=1; in1=result; #1; check(prior, "L0 before launch edge");
      @(posedge clock); #1; launch=0; check(result, "L0 capture edge");
    end else begin
      @(negedge clock); launch=1; in1=poison; #1; check(prior, "before launch edge");
      @(posedge clock); #1; launch=0;
      if (L == 1) check(poison, "L1 completion window");
      else check(prior, "after launch edge");
      for (i=1; i<L; i=i+1) begin
        @(negedge clock); in1=poison; #1; check(prior, "before due edge");
        @(posedge clock); #1;
        if (i == L-1) check(poison, "completion window begins");
        else check(prior, "latency pipeline advance");
      end
      @(negedge clock); in1=result; #1; check(result, "raw-result bypass");
      @(posedge clock); #1; check(result, "result captured at due edge");
    end
    @(negedge clock); launch=0; in1=poison; #1; check(result, "held after input changes");
    @(posedge clock); #1; check(result, "held through another cycle");
  end
endtask

task reset_with_pending_token;
  input [7:0] result, poison;
  begin
    if (L == 0) begin
      @(negedge clock); reset=0; launch=0; in1=poison;
      @(posedge clock); #1; check(8'h00, "synchronous reset edge");
      @(negedge clock); reset=1; launch=1; in1=result; #1; check(8'h00, "L0 before relaunch edge");
      @(posedge clock); #1; launch=0; check(result, "L0 post-reset capture");
    end else begin
      @(negedge clock); reset=0; launch=0; in1=poison;
      @(posedge clock); #1; check(8'h00, "synchronous reset clears captured result");
      @(negedge clock); reset=1;
      @(negedge clock); launch=1; in1=poison;
      @(posedge clock); #1; launch=0;
      @(negedge clock); reset=0; launch=0; in1=poison;
      @(posedge clock); #1;
      @(negedge clock); reset=1; launch=1; in1=poison;
      @(posedge clock); #1; launch=0;
      for (i=1; i<L; i=i+1) begin
        @(negedge clock); in1=poison;
        @(posedge clock); #1;
      end
      @(negedge clock); in1=result; #1; check(result, "fresh post-reset bypass");
      @(posedge clock); #1; check(result, "fresh post-reset capture");
    end
    @(negedge clock); launch=0; in1=poison; #1; check(result, "post-reset hold");
    for (i=0; i<L+2; i=i+1) begin
      @(posedge clock); #1; check(result, "stale token cannot overwrite fresh result");
    end
  end
endtask

task stale_token_no_mask;
  begin
    if (L == 3) begin
      @(negedge clock); reset=1; launch=1; in1=8'he1;
      @(posedge clock); #1; launch=0;
      @(negedge clock); reset=0;
      @(posedge clock); #1;
      @(negedge clock); reset=1;
      @(posedge clock); #1;
      @(negedge clock); in1=8'h5a; #1;
      check(8'h5a, "epoch-invalid output bypass remains unmasked");
      @(posedge clock); #1;
      check(8'h00, "epoch-invalid token is not captured or enqueued");
    end
  end
endtask

task consecutive_operations;
  begin
    last_value=8'h22;
    if (L == 0) begin
      for (n=0; n<3; n=n+1) begin
        @(negedge clock); launch=1; in1=result_for(n); #1;
        check(last_value, "L0 before capture edge");
        @(posedge clock); #1; last_value=result_for(n);
        check(last_value, "L0 consecutive capture");
      end
    end else begin
      for (n=0; n<=L+2; n=n+1) begin
        q_before=n-L;
        q_after=n-L+1;
        @(negedge clock);
        launch=(n < 3);
        if (q_before >= 0 && q_before < 3) begin
          in1=result_for(q_before); #1; check(result_for(q_before), "due window before capture");
        end else begin
          in1=8'he1; #1; check(last_value, "held before due window");
        end
        @(posedge clock); #1;
        if (q_before >= 0 && q_before < 3) last_value=result_for(q_before);
        if (q_after >= 0 && q_after < 3) begin
          in1=result_for(q_after); #1; check(result_for(q_after), "next consecutive bypass");
        end else check(last_value, "consecutive result captured");
      end
    end
    @(negedge clock); launch=0; in1=8'he1; #1; check(8'h83, "last consecutive result held");
    @(posedge clock); #1; check(8'h83, "last consecutive result remains held");
  end
endtask

initial begin
  reset=0; launch=0; in1=0;
  @(posedge clock); #1; check(8'h00, "initial synchronous reset");
  @(negedge clock); reset=1;
  if (C == 1) begin
    single_operation(8'h00, 8'h41, 8'he1);
    reset_with_pending_token(8'h22, 8'he1);
    consecutive_operations;
  end else multi_entry_queue;
  stale_token_no_mask;
  $display("PASS Verilog LATENCY=%0d CAPACITY=%0d", L, C);
  $finish;
end
endmodule
'''


def vhdl_testbench(latency, capacity):
    return f'''library ieee;
use ieee.std_logic_1164.all;
entity tb_timed_capture is end entity;
architecture sim of tb_timed_capture is
  constant L : natural := {latency};
  constant C : positive := {capacity};
  signal clock : std_logic := '0';
  signal reset, launch, consume : std_logic := '0';
  signal in1 : std_logic_vector(7 downto 0) := (others => '0');
  signal out1 : std_logic_vector(7 downto 0);
begin
  clock <= not clock after 5 ns;
  dut: entity work.TIMED_CAPTURE generic map(LATENCY => L, CAPACITY => C, BITSIZE_out1 => 8)
    port map(clock => clock, reset => reset, launch => launch, consume => consume, in1 => in1, out1 => out1);
  stimulus: process
    variable last_value : std_logic_vector(7 downto 0) := x"22";
    variable q_before, q_after : integer;
    function result_for(index : integer) return std_logic_vector is
    begin
      case index is
        when 0 => return x"61";
        when 1 => return x"72";
        when others => return x"83";
      end case;
    end function;
    procedure check(expected : std_logic_vector(7 downto 0); phase : string) is
    begin
      assert out1 = expected report "L=" & integer'image(L) & " " & phase severity failure;
    end procedure;
  begin
    reset <= '0'; launch <= '0'; in1 <= x"00";
    wait until rising_edge(clock); wait for 1 ns; check(x"00", "initial synchronous reset");
    wait until falling_edge(clock); reset <= '1';

    if C = 1 then
    if L = 0 then
      wait until falling_edge(clock); launch <= '1'; in1 <= x"41"; wait for 1 ns;
      check(x"00", "L0 before launch edge");
      wait until rising_edge(clock); wait for 1 ns; launch <= '0'; check(x"41", "L0 capture edge");
    else
      wait until falling_edge(clock); launch <= '1'; in1 <= x"e1"; wait for 1 ns;
      check(x"00", "before launch edge");
      wait until rising_edge(clock); wait for 1 ns; launch <= '0';
      if L = 1 then check(x"e1", "L1 completion window");
      else check(x"00", "after launch edge"); end if;
      for i in 1 to L-1 loop
        wait until falling_edge(clock); in1 <= x"e1"; wait for 1 ns;
        check(x"00", "before due edge");
        wait until rising_edge(clock); wait for 1 ns;
        if i = L-1 then check(x"e1", "completion window begins");
        else check(x"00", "latency pipeline advance"); end if;
      end loop;
      wait until falling_edge(clock); in1 <= x"41"; wait for 1 ns;
      check(x"41", "raw-result bypass");
      wait until rising_edge(clock); wait for 1 ns; check(x"41", "result captured at due edge");
    end if;
    wait until falling_edge(clock); launch <= '0'; in1 <= x"e1"; wait for 1 ns;
    check(x"41", "held after input changes");
    wait until rising_edge(clock); wait for 1 ns; check(x"41", "held through another cycle");

    if L = 0 then
      wait until falling_edge(clock); reset <= '0'; launch <= '0'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns; check(x"00", "synchronous reset edge");
      wait until falling_edge(clock); reset <= '1'; launch <= '1'; in1 <= x"22"; wait for 1 ns;
      check(x"00", "L0 before relaunch edge");
      wait until rising_edge(clock); wait for 1 ns; launch <= '0'; check(x"22", "L0 post-reset capture");
    else
      wait until falling_edge(clock); reset <= '0'; launch <= '0'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns; check(x"00", "synchronous reset clears captured result");
      wait until falling_edge(clock); reset <= '1';
      wait until falling_edge(clock); launch <= '1'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns; launch <= '0';
      wait until falling_edge(clock); reset <= '0'; launch <= '0'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); reset <= '1'; launch <= '1'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns; launch <= '0';
      for i in 1 to L-1 loop
        wait until falling_edge(clock); in1 <= x"e1";
        wait until rising_edge(clock); wait for 1 ns;
      end loop;
      wait until falling_edge(clock); in1 <= x"22"; wait for 1 ns;
      check(x"22", "fresh post-reset bypass");
      wait until rising_edge(clock); wait for 1 ns; check(x"22", "fresh post-reset capture");
    end if;
    wait until falling_edge(clock); launch <= '0'; in1 <= x"e1"; wait for 1 ns;
    check(x"22", "post-reset hold");
    for i in 0 to L+1 loop
      wait until rising_edge(clock); wait for 1 ns;
      check(x"22", "stale token cannot overwrite fresh result");
    end loop;

    last_value := x"22";
    if L = 0 then
      for n in 0 to 2 loop
        wait until falling_edge(clock); launch <= '1'; in1 <= result_for(n); wait for 1 ns;
        check(last_value, "L0 before capture edge");
        wait until rising_edge(clock); wait for 1 ns;
        last_value := result_for(n); check(last_value, "L0 consecutive capture");
      end loop;
    else
      for n in 0 to L+2 loop
        q_before := n-L;
        q_after := n-L+1;
        wait until falling_edge(clock);
        if n < 3 then launch <= '1'; else launch <= '0'; end if;
        if q_before >= 0 and q_before < 3 then
          in1 <= result_for(q_before); wait for 1 ns;
          check(result_for(q_before), "due window before capture");
        else
          in1 <= x"e1"; wait for 1 ns;
          check(last_value, "held before due window");
        end if;
        wait until rising_edge(clock); wait for 1 ns;
        if q_before >= 0 and q_before < 3 then last_value := result_for(q_before); end if;
        if q_after >= 0 and q_after < 3 then
          in1 <= result_for(q_after); wait for 1 ns;
          check(result_for(q_after), "next consecutive bypass");
        else
          check(last_value, "consecutive result captured");
        end if;
      end loop;
    end if;
    wait until falling_edge(clock); launch <= '0'; in1 <= x"e1"; wait for 1 ns;
    check(x"83", "last consecutive result held");
    wait until rising_edge(clock); wait for 1 ns; check(x"83", "last consecutive result remains held");
    else
      assert L = 3 and C = 3 report "queue test requires L=3 and C=3" severity failure;
      for n in 0 to 3 loop
        wait until falling_edge(clock); launch <= '1';
        if n = 3 then in1 <= x"61"; else in1 <= x"e1"; end if;
        if n = 3 then wait for 1 ns; check(x"61", "empty-queue capture bypass"); end if;
        wait until rising_edge(clock); wait for 1 ns;
      end loop;
      wait until falling_edge(clock); launch <= '0'; in1 <= x"72"; wait for 1 ns;
      check(x"61", "first queued result held");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); in1 <= x"83"; wait for 1 ns;
      check(x"61", "second result enqueued");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); in1 <= x"94"; consume <= '1'; wait for 1 ns;
      check(x"61", "simultaneous full pop/push head");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); in1 <= x"e1"; wait for 1 ns;
      check(x"72", "ordered drain result 1");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); wait for 1 ns; check(x"83", "ordered drain result 2");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); wait for 1 ns; check(x"94", "ordered drain result 3 after wrap");
      wait until rising_edge(clock); wait for 1 ns; consume <= '0';

      wait until falling_edge(clock); launch <= '1'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); launch <= '0'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); reset <= '0';
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); reset <= '1'; launch <= '1';
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); launch <= '0'; in1 <= x"e1";
      for i in 1 to L-1 loop wait until rising_edge(clock); end loop;
      wait until falling_edge(clock); in1 <= x"a5"; wait for 1 ns;
      check(x"a5", "fresh result bypass after reset");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); consume <= '1'; in1 <= x"e1"; wait for 1 ns;
      check(x"a5", "fresh result after reset");
      wait until rising_edge(clock); wait for 1 ns; consume <= '0';

      wait until falling_edge(clock); launch <= '1'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); launch <= '0'; in1 <= x"e1";
      for i in 1 to L-1 loop wait until rising_edge(clock); end loop;
      wait until falling_edge(clock); launch <= '1'; consume <= '1'; in1 <= x"b6"; wait for 1 ns;
      check(x"b6", "empty simultaneous capture/consume bypass");
      wait until rising_edge(clock); wait for 1 ns; consume <= '0';
      wait until falling_edge(clock); launch <= '0'; in1 <= x"e1";
      for i in 1 to L-1 loop wait until rising_edge(clock); end loop;
      wait until falling_edge(clock); in1 <= x"c7"; wait for 1 ns;
      check(x"c7", "next result after empty bypass");
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); consume <= '1'; in1 <= x"e1"; wait for 1 ns;
      check(x"c7", "next result is not preceded by duplicate");
      wait until rising_edge(clock); wait for 1 ns; consume <= '0';
    end if;

    if L = 3 then
      wait until falling_edge(clock); reset <= '1'; launch <= '1'; in1 <= x"e1";
      wait until rising_edge(clock); wait for 1 ns; launch <= '0';
      wait until falling_edge(clock); reset <= '0';
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); reset <= '1';
      wait until rising_edge(clock); wait for 1 ns;
      wait until falling_edge(clock); in1 <= x"5a"; wait for 1 ns;
      check(x"5a", "epoch-invalid output bypass remains unmasked");
      wait until rising_edge(clock); wait for 1 ns;
      check(x"00", "epoch-invalid token is not captured or enqueued");
    end if;
    report "PASS VHDL LATENCY=" & integer'image(L) & " CAPACITY=" & integer'image(C) severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
'''


def timed_capture_cases():
    configurations = [(latency, 1) for latency in LATENCIES] + [(3, 3)]
    return [(language, latency, capacity) for latency, capacity in configurations
            for language in ("Verilog", "VHDL")]


def run_timed_capture_case(language, latency, capacity, modelsim_bin, temp_root):
    with tempfile.TemporaryDirectory(
        prefix=f"timed-capture-{language.lower()}-l{latency}-c{capacity}-", dir=temp_root
    ) as temp_dir:
        scratch = Path(temp_dir)
        env = os.environ.copy()
        env.update(TMPDIR=str(scratch), TMP=str(scratch), TEMP=str(scratch))
        env["PATH"] = modelsim_bin + os.pathsep + env.get("PATH", "")
        extract_models(scratch)

        if language == "Verilog":
            tb = scratch / f"tb_timed_capture_l{latency}_c{capacity}.v"
            testbench = VERILOG_TB.replace("parameter integer L=2;", f"parameter integer L={latency};")
            tb.write_text(testbench.replace("parameter integer C=1;", f"parameter integer C={capacity};"))
            work = f"verilog_l{latency}_c{capacity}"
            run(["vlib", work], scratch, env)
            run(["vlog", "-work", work, scratch / "timed_capture.v", tb], scratch, env)
            top = f"{work}.tb"
        else:
            tb = scratch / f"tb_timed_capture_l{latency}_c{capacity}.vhd"
            tb.write_text(vhdl_testbench(latency, capacity))
            work = f"work_l{latency}_c{capacity}"
            run(["vlib", work], scratch, env)
            run(["vcom", "-2008", "-work", work, scratch / "timed_capture.vhd", tb], scratch, env)
            top = f"{work}.tb_timed_capture"

        output = run(["vsim", "-c", "-quiet", "-L", work, top, "-do", "run -all; quit -f"], scratch, env)
        marker = f"PASS {language} LATENCY={latency} CAPACITY={capacity}"
        if marker not in output:
            raise RuntimeError(f"{language} L={latency}, C={capacity} omitted PASS marker:\n{output}")
        return marker


def run_timed_capture_cases(jobs, modelsim_bin, temp_root):
    cases = timed_capture_cases()
    results = {}
    with ThreadPoolExecutor(max_workers=min(jobs, len(cases))) as executor:
        futures = {
            executor.submit(run_timed_capture_case, *case, modelsim_bin, temp_root): case
            for case in cases
        }
        for future in as_completed(futures):
            case = futures[future]
            try:
                results[case] = (future.result(), None)
            except Exception as error:
                results[case] = (None, f"{type(error).__name__}: {error}")
    return [(case, *results[case]) for case in cases]


def main(argv=None):
    jobs = resolve_parallelism(sys.argv[1:] if argv is None else argv)
    temp_root = ROOT / "documentation/tmp"
    temp_root.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    modelsim_bin = resolve_modelsim_bin(env)
    env["PATH"] = modelsim_bin + os.pathsep + env.get("PATH", "")
    for tool in ("vlib", "vlog", "vcom", "vsim"):
        if not shutil.which(tool, path=env["PATH"]):
            raise SystemExit(f"Timed-capture check requires ModelSim {tool} in {modelsim_bin}")

    failures = 0
    for case, marker, error in run_timed_capture_cases(jobs, modelsim_bin, temp_root):
        if error is not None:
            failures += 1
            language, latency, capacity = case
            print(f"FAIL {language} LATENCY={latency} CAPACITY={capacity}:\n{error}", file=sys.stderr)
        else:
            print(marker)
    return int(failures != 0)


if __name__ == "__main__":
    raise SystemExit(main())
