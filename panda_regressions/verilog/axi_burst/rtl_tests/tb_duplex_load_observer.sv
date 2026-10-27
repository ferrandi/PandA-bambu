`timescale 1ns/1ps

module MinimalAXI4MasterPipelined #(
  parameter integer B_MAX = 16,
  parameter integer MAX_OUTSTANDING = 1,
  parameter integer FIFO_DEPTH = 64
) (
  input logic clock,
  input logic reset,
  input logic start,
  input logic [1:0] in1,
  input logic fifo_load_pop,
  input logic [30:0] consumed,
  input logic [30:0] region_count
);
endmodule

module tb_duplex_load_observer;
  logic clock = 0;
  logic reset = 0;
  logic start = 0;
  logic [1:0] operation = 0;
  logic fifo_load_pop = 0;
  logic [30:0] consumed = 0;
  logic [30:0] region_count = 256;

  always #5 clock = !clock;

  MinimalAXI4MasterPipelined dut (
    .clock(clock),
    .reset(reset),
    .start(start),
    .in1(operation),
    .fifo_load_pop(fifo_load_pop),
    .consumed_reg(consumed),
    .region_count_reg(region_count)
  );

  task automatic require(input logic condition, input string message);
    if (!condition)
      $fatal(1, "DUPLEX_LOAD_OBSERVER_UNIT_FAIL %s", message);
  endtask

  initial
  begin
    repeat (2) @(posedge clock);
    @(negedge clock);
    reset = 1;
    start = 1;
    fifo_load_pop = 1;
    consumed = 0;
    @(posedge clock);
    #1;
    require(dut.duplex_load_observer_instance.event_count == 1,
            "immediate completion event count");
    require(dut.duplex_load_observer_instance.last_index == 0,
            "immediate completion consumed-before-edge index");
    require(dut.duplex_load_observer_instance.last_count == 256,
            "immediate completion region count");
    require(dut.duplex_load_observer_instance.last_cycle == 0,
            "immediate completion cycle");

    @(negedge clock);
    reset = 0;
    start = 0;
    fifo_load_pop = 0;
    @(posedge clock);
    #1;
    require(dut.duplex_load_observer_instance.event_count == 0,
            "reset clears event count");

    @(negedge clock);
    reset = 1;
    start = 1;
    consumed = 0;
    @(posedge clock);
    #1;
    require(dut.duplex_load_observer_instance.event_count == 0,
            "pending request does not emit before completion");
    @(negedge clock);
    start = 0;
    @(posedge clock);
    #1;
    @(negedge clock);
    consumed = 7;
    fifo_load_pop = 1;
    @(posedge clock);
    #1;
    require(dut.duplex_load_observer_instance.event_count == 1,
            "pending completion event count");
    require(dut.duplex_load_observer_instance.last_index == 7,
            "pending completion consumed-before-edge index");
    require(dut.duplex_load_observer_instance.last_count == 256,
            "pending completion region count");
    require(dut.duplex_load_observer_instance.last_cycle == 2,
            "pending completion positive relative cycle");
    $display("DUPLEX_LOAD_OBSERVER_UNIT PASS IMMEDIATE_COUNT=1 INDEX=0 CYCLE=0 PENDING_COUNT=1 INDEX=7 CYCLE=2");
    $finish;
  end
endmodule
