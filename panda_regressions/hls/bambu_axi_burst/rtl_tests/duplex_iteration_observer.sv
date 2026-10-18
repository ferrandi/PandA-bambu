module duplex_iteration_observer #(
  parameter string MAPPING_ID = "",
  parameter integer EXPECTED_EVENTS = 256,
  parameter integer SOURCE_BASE_BYTES = 8192
) (
  input logic clock,
  input logic reset,
  input logic capture,
  input string state_name,
  input logic [31:0] loop_index,
  input logic [31:0] source_address,
  output integer event_count
);
  integer event_file;
  integer sample_cycle = 0;
  logic first_event = 1'b1;
  logic file_open = 1'b0;

  initial begin
    event_count = 0;
    event_file = $fopen("iteration-events.json", "w");
    if (event_file == 0) $fatal(1, "DUPLEX_ITERATION_EVENT_FILE_OPEN_FAILED");
    file_open = 1'b1;
    $fwrite(event_file, "{\"schema_version\":1,\"mapping_id\":\"%s\",\"event_type\":\"controller_iteration_start\",\"source_base_bytes\":%0d,\"events\":[\n",
            MAPPING_ID, SOURCE_BASE_BYTES);
  end

  always @(posedge clock) begin
    if (!reset) begin
      sample_cycle = 0;
    end else begin
      sample_cycle = sample_cycle + 1;
      if (capture && state_name != "") begin
        if (event_count >= EXPECTED_EVENTS)
          $fatal(1, "DUPLEX_ITERATION_EXTRA_EVENT");
        if (loop_index !== event_count[31:0])
          $fatal(1, "DUPLEX_ITERATION_INDEX_MISMATCH expected=%0d actual=%0d", event_count, loop_index);
        if (source_address !== SOURCE_BASE_BYTES + 4 * event_count)
          $fatal(1, "DUPLEX_ITERATION_ADDRESS_MISMATCH index=%0d expected=%0d actual=%0d",
                 event_count, SOURCE_BASE_BYTES + 4 * event_count, source_address);
        if (!first_event)
          $fwrite(event_file, ",\n");
        $fwrite(event_file,
                "{\"iteration\":%0d,\"cycle\":%0d,\"state\":\"%s\",\"loop_index\":%0d,\"source_address\":%0d}",
                event_count, sample_cycle, state_name, loop_index, source_address);
        first_event = 1'b0;
        event_count = event_count + 1;
      end
    end
  end

  final begin
    if (file_open) begin
      $fwrite(event_file, "]}\n");
      $fclose(event_file);
    end
  end
endmodule
