module tb_duplex_iteration_observer;
  logic clock = 1'b0;
  always #5 clock = ~clock;
  logic reset = 1'b0;
  logic capture = 1'b0;
  string state_name = "";
  logic [31:0] loop_index = 0;
  logic [31:0] source_address = 32'h1000;
  integer event_count;

  duplex_iteration_observer #(
    .MAPPING_ID("unit-mapping"),
    .EXPECTED_EVENTS(3),
    .SOURCE_BASE_BYTES(4096)
  ) observer (
    .clock(clock), .reset(reset), .capture(capture), .state_name(state_name),
    .loop_index(loop_index), .source_address(source_address), .event_count(event_count)
  );

  integer index;
  initial begin
    repeat (2) @(negedge clock);
    reset = 1'b1;
    capture = 1'b1;
    state_name = "S_7";
    for (index = 0; index < 3; index = index + 1) begin
      loop_index = index;
      source_address = 32'h1000 + 4 * index;
      @(negedge clock);
    end
    capture = 1'b0;
    if (event_count != 3)
      $fatal(1, "DUPLEX_ITERATION_OBSERVER_UNIT_COUNT");
    $display("DUPLEX_ITERATION_OBSERVER_UNIT PASS COUNT=3 STATE=S_7");
    $finish;
  end
endmodule
