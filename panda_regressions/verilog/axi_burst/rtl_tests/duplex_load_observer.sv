module duplex_load_observer #(
  parameter integer B_MAX = 16,
  parameter integer MAX_OUTSTANDING = 1,
  parameter integer FIFO_DEPTH = 64
) (
  input logic clock,
  input logic reset,
  input logic start,
  input logic [1:0] operation,
  input logic fifo_load_pop,
  input logic [30:0] consumed,
  input logic [30:0] region_count
);
  string selected_case;
  integer relative_cycle = 0;
  logic metrics_enabled = 0;
  logic selected_scenario = 0;
  logic invocation_started = 0;
  integer event_count = 0;
  logic [30:0] last_index = 0;
  logic [30:0] last_count = 0;
  integer last_cycle = 0;

  initial
  begin
    metrics_enabled = $test$plusargs("G3_METRICS");
    selected_scenario = 0;
    if ($value$plusargs("CASE=%s", selected_case))
      selected_scenario = selected_case == "COPY_PARALLELISM_256";
  end

  task automatic record_load_event(input integer event_cycle);
    begin
      event_count = event_count + 1;
      last_index = consumed;
      last_count = region_count;
      last_cycle = event_cycle;
      $display("DUPLEX_LOAD_EVENT CASE=COPY_PARALLELISM_256 INDEX=%0d COUNT=%0d CYCLE=%0d READ_B_MAX=%0d READ_OUTSTANDING=%0d READ_FIFO_DEPTH=%0d",
               consumed, region_count, event_cycle, B_MAX,
               MAX_OUTSTANDING, FIFO_DEPTH);
    end
  endtask

  always @(posedge clock)
  begin
    if (!reset)
    begin
      relative_cycle = 0;
      invocation_started = 0;
      event_count = 0;
      last_index = 0;
      last_count = 0;
      last_cycle = 0;
    end
    else if (metrics_enabled && selected_scenario)
    begin
      if (!invocation_started && start && operation == 2'd0)
      begin
        invocation_started = 1;
        relative_cycle = 0;
        if (fifo_load_pop)
          record_load_event(0);
      end
      else if (invocation_started)
      begin
        relative_cycle = relative_cycle + 1;
        if (fifo_load_pop)
          record_load_event(relative_cycle);
      end
    end
  end
endmodule

bind MinimalAXI4MasterPipelined duplex_load_observer #(
  .B_MAX(B_MAX),
  .MAX_OUTSTANDING(MAX_OUTSTANDING),
  .FIFO_DEPTH(FIFO_DEPTH)
) duplex_load_observer_instance (
  .clock(clock),
  .reset(reset),
  .start(start),
  .operation(in1),
  .fifo_load_pop(fifo_load_pop),
  .consumed_reg(consumed),
  .region_count_reg(region_count)
);
