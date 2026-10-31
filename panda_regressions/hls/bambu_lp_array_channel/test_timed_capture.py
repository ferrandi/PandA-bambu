#!/usr/bin/env python3
"""Cycle-accurate checks for the TIMED_CAPTURE models in NC_STD_IPs.xml."""

import os
from pathlib import Path
import shutil
import subprocess
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
        """module TIMED_CAPTURE #(parameter integer LATENCY=0, parameter integer BITSIZE_out1=8) (
  input wire clock, reset, launch,
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
  generic (LATENCY : natural := 0; BITSIZE_out1 : positive := 8);
  port (clock, reset, launch : in std_logic;
        in1 : in std_logic_vector(BITSIZE_out1-1 downto 0);
        out1 : out std_logic_vector(BITSIZE_out1-1 downto 0));
end entity;
architecture rtl of TIMED_CAPTURE is
""" + vhdl + "\nend architecture;\n"
    )


VERILOG_TB = r'''module tb;
parameter integer L=2;
reg clock=0;
always #5 clock=~clock;
reg reset=0, launch=0;
reg [7:0] in1=0;
wire [7:0] out1;
integer i, n, q_before, q_after;
reg [7:0] last_value;
TIMED_CAPTURE #(.LATENCY(L), .BITSIZE_out1(8)) dut(clock, reset, launch, in1, out1);

task check;
  input [7:0] expected;
  input [8*40-1:0] phase;
  begin
    if (out1 !== expected)
      $fatal(1, "L=%0d phase=%0s time=%0t got=%h expected=%h",
             L, phase, $time, out1, expected);
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
  single_operation(8'h00, 8'h41, 8'he1);
  reset_with_pending_token(8'h22, 8'he1);
  consecutive_operations;
  $display("PASS Verilog LATENCY=%0d", L);
  $finish;
end
endmodule
'''


def vhdl_testbench(latency):
    return f'''library ieee;
use ieee.std_logic_1164.all;
entity tb_timed_capture is end entity;
architecture sim of tb_timed_capture is
  constant L : natural := {latency};
  signal clock : std_logic := '0';
  signal reset, launch : std_logic := '0';
  signal in1 : std_logic_vector(7 downto 0) := (others => '0');
  signal out1 : std_logic_vector(7 downto 0);
begin
  clock <= not clock after 5 ns;
  dut: entity work.TIMED_CAPTURE generic map(LATENCY => L, BITSIZE_out1 => 8)
    port map(clock => clock, reset => reset, launch => launch, in1 => in1, out1 => out1);
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
    report "PASS VHDL LATENCY=" & integer'image(L) severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
'''


def main():
    temp_root = ROOT / "documentation/tmp"
    temp_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="timed-capture-", dir=temp_root) as temp_dir:
        scratch = Path(temp_dir)
        env = os.environ.copy()
        env.update(TMPDIR=str(scratch), TMP=str(scratch), TEMP=str(scratch))
        modelsim_bin = resolve_modelsim_bin(env)
        env["PATH"] = modelsim_bin + os.pathsep + env.get("PATH", "")
        for tool in ("vlib", "vlog", "vcom", "vsim"):
            if not shutil.which(tool, path=env["PATH"]):
                raise SystemExit(f"Timed-capture check requires ModelSim {tool} in {modelsim_bin}")
        extract_models(scratch)
        for latency in LATENCIES:
            verilog_tb = scratch / f"tb_timed_capture_l{latency}.v"
            verilog_tb.write_text(VERILOG_TB.replace("parameter integer L=2;",
                                                     f"parameter integer L={latency};"))
            verilog_work = f"verilog_l{latency}"
            run(["vlib", verilog_work], scratch, env)
            run(["vlog", "-work", verilog_work, scratch / "timed_capture.v", verilog_tb], scratch, env)
            output = run(["vsim", "-c", "-quiet", "-L", verilog_work,
                          f"{verilog_work}.tb", "-do", "run -all; quit -f"], scratch, env)
            if f"PASS Verilog LATENCY={latency}" not in output:
                raise RuntimeError(f"Verilog L={latency} omitted PASS marker:\n{output}")
            print(f"PASS Verilog LATENCY={latency}")

            tb = scratch / f"tb_timed_capture_l{latency}.vhd"
            tb.write_text(vhdl_testbench(latency))
            work = f"work_l{latency}"
            run(["vlib", work], scratch, env)
            run(["vcom", "-2008", "-work", work, scratch / "timed_capture.vhd", tb], scratch, env)
            output = run(["vsim", "-c", "-quiet", "-L", work, f"{work}.tb_timed_capture",
                          "-do", "run -all; quit -f"], scratch, env)
            if f"PASS VHDL LATENCY={latency}" not in output:
                raise RuntimeError(f"VHDL L={latency} omitted PASS marker:\n{output}")
            print(f"PASS VHDL LATENCY={latency}")


if __name__ == "__main__":
    main()
