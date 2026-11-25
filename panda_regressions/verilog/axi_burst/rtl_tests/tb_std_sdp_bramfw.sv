module tb_std_sdp_bramfw #(
    parameter DEPTH = 4,
    parameter INPUT_BITS = 8,
    parameter OUTPUT_BITS = 8,
    parameter READ_ADDRESS_BITS = 2,
    parameter WRITE_ADDRESS_BITS = 2,
    parameter HIGH_LATENCY = 0,
    parameter READ_ONLY_MEMORY = 0,
    parameter MEMORY_INIT_file = ""
);
    reg clock = 0;
    reg write_enable = 0;
    reg [READ_ADDRESS_BITS-1:0] read_address = 0;
    reg [WRITE_ADDRESS_BITS-1:0] write_address = 0;
    reg [INPUT_BITS-1:0] write_data = 0;
    reg [31:0] stimulus_reg = 32'hc001beef;
    wire [OUTPUT_BITS-1:0] actual;
    wire [OUTPUT_BITS-1:0] expected;
    integer cycle;

    always #5 clock = ~clock;

    STD_SDP_BRAMFW #(
        .BITSIZE_address_inr(READ_ADDRESS_BITS), .BITSIZE_address_inw(WRITE_ADDRESS_BITS),
        .BITSIZE_data_in(INPUT_BITS), .BITSIZE_data_out(OUTPUT_BITS),
        .n_elements(DEPTH), .MEMORY_INIT_file(MEMORY_INIT_file),
        .HIGH_LATENCY(HIGH_LATENCY), .READ_ONLY_MEMORY(READ_ONLY_MEMORY)) dut (
        .clock(clock), .write_enable(write_enable), .address_inr(read_address),
        .address_inw(write_address), .data_in(write_data), .data_out(actual));

    std_sdp_bramfw_reference #(
        .DEPTH(DEPTH), .INPUT_BITS(INPUT_BITS), .OUTPUT_BITS(OUTPUT_BITS),
        .READ_ADDRESS_BITS(READ_ADDRESS_BITS), .WRITE_ADDRESS_BITS(WRITE_ADDRESS_BITS),
        .HIGH_LATENCY(HIGH_LATENCY), .READ_ONLY_MEMORY(READ_ONLY_MEMORY),
        .MEMORY_INIT_file(MEMORY_INIT_file)) reference_model (
        .clock(clock), .write_enable(write_enable), .read_address(read_address),
        .write_address(write_address), .write_data(write_data), .read_data(expected));

    // Compare four-state values, including initialization and pipeline warm-up.
    always @(posedge clock)
    begin
        #1;
        if (actual !== expected)
        begin
            $fatal(1, "BRAMFW_MISMATCH H=%0d RO=%0d D=%0d actual=%h expected=%h",
                   HIGH_LATENCY, READ_ONLY_MEMORY, DEPTH, actual, expected);
        end
    end

    initial
    begin
        for (cycle = 0; cycle < 1000; cycle = cycle + 1)
        begin
            @(negedge clock);
            stimulus_reg = {stimulus_reg[30:0], stimulus_reg[31] ^ stimulus_reg[21] ^ stimulus_reg[1] ^ stimulus_reg[0]};
            read_address = stimulus_reg % DEPTH;
            write_address = (stimulus_reg >> 8) % DEPTH;
            write_enable = stimulus_reg[16];
            write_data = stimulus_reg;
            // Force repeated read/write collisions, including in read-only mode.
            if (cycle % 7 == 0)
            begin
                write_address = read_address;
                write_enable = 1;
            end
            // Exercise unknown controls, addresses and payload with four-state simulation.
            if (cycle % 31 == 0)
            begin
                write_enable = 1'bx;
            end
            if (cycle % 37 == 0)
            begin
                read_address = {READ_ADDRESS_BITS{1'bx}};
            end
            if (cycle % 41 == 0)
            begin
                write_data = {INPUT_BITS{1'bx}};
            end
            if (cycle % 43 == 0 && (1 << READ_ADDRESS_BITS) > DEPTH)
            begin
                read_address = DEPTH;
            end
            if (cycle % 47 == 0 && (1 << WRITE_ADDRESS_BITS) > DEPTH)
            begin
                write_address = DEPTH;
            end
            if (cycle % 53 == 0)
            begin
                write_address = {WRITE_ADDRESS_BITS{1'bx}};
            end
            if (cycle % 59 == 0)
            begin
                write_enable = 1'bz;
            end
        end
        repeat (5) @(negedge clock);
        $display("STD_SDP_BRAMFW_PASS H=%0d RO=%0d D=%0d", HIGH_LATENCY, READ_ONLY_MEMORY, DEPTH);
        $finish;
    end
endmodule
