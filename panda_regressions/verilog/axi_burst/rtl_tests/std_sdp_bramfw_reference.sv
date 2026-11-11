// Independent cycle-level reference for STD_SDP_BRAMFW, including the legacy
// forwarding behavior when READ_ONLY_MEMORY disables writes to the array.
module std_sdp_bramfw_reference #(
    parameter DEPTH = 4,
    parameter INPUT_BITS = 8,
    parameter OUTPUT_BITS = 8,
    parameter READ_ADDRESS_BITS = 2,
    parameter WRITE_ADDRESS_BITS = 2,
    parameter HIGH_LATENCY = 0,
    parameter READ_ONLY_MEMORY = 0,
    parameter MEMORY_INIT_file = ""
) (
    input wire clock,
    input wire write_enable,
    input wire [READ_ADDRESS_BITS-1:0] read_address,
    input wire [WRITE_ADDRESS_BITS-1:0] write_address,
    input wire [INPUT_BITS-1:0] write_data,
    output wire [OUTPUT_BITS-1:0] read_data
);
    reg [OUTPUT_BITS-1:0] memory_reg [0:DEPTH-1];
    reg [READ_ADDRESS_BITS-1:0] read_address_reg;
    reg [WRITE_ADDRESS_BITS-1:0] write_address_reg;
    reg [INPUT_BITS-1:0] write_data_reg;
    reg write_enable_reg;
    reg [OUTPUT_BITS-1:0] result_reg;
    reg [OUTPUT_BITS-1:0] delayed_result_reg;
    wire [READ_ADDRESS_BITS-1:0] effective_read_address =
        HIGH_LATENCY == 2 ? read_address_reg : read_address;
    wire [WRITE_ADDRESS_BITS-1:0] effective_write_address =
        HIGH_LATENCY == 2 ? write_address_reg : write_address;
    wire [INPUT_BITS-1:0] effective_write_data =
        HIGH_LATENCY == 2 ? write_data_reg : write_data;
    wire effective_write_enable = HIGH_LATENCY == 2 ? write_enable_reg : write_enable;
    integer index;

    initial
    begin
        if (MEMORY_INIT_file != "")
        begin
            $readmemb(MEMORY_INIT_file, memory_reg, 0, DEPTH-1);
        end
        else
        begin
            for (index = 0; index < DEPTH; index = index + 1)
            begin
                memory_reg[index] = 0;
            end
        end
    end

    always @(posedge clock)
    begin
        read_address_reg <= read_address;
    end

    always @(posedge clock)
    begin
        write_address_reg <= write_address;
    end

    always @(posedge clock)
    begin
        write_data_reg <= write_data;
    end

    always @(posedge clock)
    begin
        write_enable_reg <= write_enable;
    end

    always @(posedge clock)
    begin
        if (READ_ONLY_MEMORY == 0 && effective_write_enable)
        begin
            memory_reg[effective_write_address] <= effective_write_data;
        end
    end

    always @(posedge clock)
    begin
        result_reg <= (effective_write_enable && effective_read_address == effective_write_address) ?
            effective_write_data : memory_reg[effective_read_address];
    end

    always @(posedge clock)
    begin
        delayed_result_reg <= result_reg;
    end

    assign read_data = HIGH_LATENCY == 0 ? result_reg : delayed_result_reg;
endmodule
