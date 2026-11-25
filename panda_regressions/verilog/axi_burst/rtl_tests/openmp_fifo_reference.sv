// Reference GMI and FIFO before the STD_FIFO migration.
// Keep independent of the new queue controller.
// Generated from NC_OPENMP_IPs.xml
module GMIReference #(
    parameter integer USE_FILTER = 0,
    parameter integer ROOT_SPACE_ALIGNMENT = 0,
    parameter integer EXTERNAL_BASE_ADDRESS = 0,
    parameter integer FORK_CALL_ID_START = 0,
    parameter integer FORK_CALL_ID_END = 0,
    parameter integer FORK_NUMBER = 0,
    parameter integer USE_CS = 0,
    parameter integer INNER_TAG = 0,
    parameter integer CONTEXT_NUMBER_SIZE = 0,
    parameter integer BITSIZE_Mout_addr_ram = 1,
    parameter integer BITSIZE_Mout_Wdata_ram = 1,
    parameter integer BITSIZE_Mout_data_ram_size = 1,
    parameter integer BITSIZE_Mout_tag = 1,
    parameter integer BITSIZE_Mout_addr_ram_memory = 1,
    parameter integer BITSIZE_Mout_Wdata_ram_memory = 1,
    parameter integer BITSIZE_Mout_data_ram_size_memory = 1,
    parameter integer BITSIZE_Mout_tag_memory = 1,
    parameter integer BITSIZE_Mout_addr_ram_local = 1,
    parameter integer BITSIZE_Mout_Wdata_ram_local = 1,
    parameter integer BITSIZE_Mout_data_ram_size_local = 1,
    parameter integer BITSIZE_Mout_tag_local = 1,
    parameter integer BITSIZE_M_Rdata_ram_memory = 1,
    parameter integer BITSIZE_M_tag = 1,
    parameter integer BITSIZE_M_Rdata_ram_local = 1,
    parameter integer BITSIZE_M_tag_local = 1,
    parameter integer BITSIZE_M_Rdata_ram = 1,
    parameter integer BITSIZE_M_tag_memory = 1
) (
    input wire clock,
    input wire reset,
    input wire Mout_oe_ram,
    input wire Mout_we_ram,
    input wire [BITSIZE_Mout_addr_ram-1:0] Mout_addr_ram,
    input wire [BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram,
    input wire [BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size,
    input wire [BITSIZE_Mout_tag-1:0] Mout_tag,
    output wire Mout_back_pressure,
    output wire Mout_oe_ram_memory,
    output wire Mout_we_ram_memory,
    output wire [BITSIZE_Mout_addr_ram_memory-1:0] Mout_addr_ram_memory,
    output wire [BITSIZE_Mout_Wdata_ram_memory-1:0] Mout_Wdata_ram_memory,
    output wire [BITSIZE_Mout_data_ram_size_memory-1:0] Mout_data_ram_size_memory,
    output wire [BITSIZE_Mout_tag_memory-1:0] Mout_tag_memory,
    input wire Mout_back_pressure_memory,
    output wire Mout_oe_ram_local,
    output wire Mout_we_ram_local,
    output wire [BITSIZE_Mout_addr_ram_local-1:0] Mout_addr_ram_local,
    output wire [BITSIZE_Mout_Wdata_ram_local-1:0] Mout_Wdata_ram_local,
    output wire [BITSIZE_Mout_data_ram_size_local-1:0] Mout_data_ram_size_local,
    output wire [BITSIZE_Mout_tag_local-1:0] Mout_tag_local,
    input wire M_DataRdy_memory,
    input wire [BITSIZE_M_Rdata_ram_memory-1:0] M_Rdata_ram_memory,
    output wire [BITSIZE_M_tag-1:0] M_tag,
    input wire M_DataRdy_local,
    input wire [BITSIZE_M_Rdata_ram_local-1:0] M_Rdata_ram_local,
    input wire [BITSIZE_M_tag_local-1:0] M_tag_local,
    output wire M_DataRdy,
    output wire [BITSIZE_M_Rdata_ram-1:0] M_Rdata_ram,
    input wire [BITSIZE_M_tag_memory-1:0] M_tag_memory,
    output wire M_back_pressure_local
);

reg Mout_oe_ram_temp;
reg Mout_we_ram_temp;
reg [BITSIZE_Mout_addr_ram_memory-1:0] Mout_addr_ram_memory_temp;
reg [BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram_temp;
reg [BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size_temp;
reg [BITSIZE_Mout_tag-1:0] Mout_tag_temp;

generate
   if(USE_CS)
   begin
      wire register_valid;
      reg register_valid_reg;
      wire fifo_wr;
      wire fifo_rd;
      reg fifo_rd_reg;
      wire fifo_empty;
      wire fifo_empty_next;
      wire [BITSIZE_Mout_addr_ram+BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag : 0] fifo_data_out;

      simple_fifo #(.ADDR_WIDTH(CONTEXT_NUMBER_SIZE),
      .BITSIZE_data_in(1+BITSIZE_Mout_addr_ram+BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag),
      .BITSIZE_data_out(1+BITSIZE_Mout_addr_ram+BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag)) fifo (.clock(clock),
      .reset(reset),
      .data_in({Mout_oe_ram,Mout_addr_ram,Mout_Wdata_ram,Mout_data_ram_size,Mout_tag}),
      .data_out(fifo_data_out),
      .empty(fifo_empty),
      .rd_en(fifo_rd),
      .wr_en(fifo_wr));

      assign register_valid = (Mout_back_pressure_memory && (Mout_oe_ram_memory || Mout_we_ram_memory));
      assign fifo_wr = (Mout_oe_ram || Mout_we_ram);
      assign fifo_rd = (fifo_wr || !fifo_empty) && !register_valid;
      assign Mout_back_pressure = 0;

      always @(posedge clock)
      begin
         if(reset == 1'b0)
         begin
            register_valid_reg <= 0;
            fifo_rd_reg <= 0;
         end
         else
         begin
            register_valid_reg <= register_valid;
            fifo_rd_reg <= fifo_rd;
         end
      end

      always @(*)
      begin
         if(register_valid_reg || fifo_rd_reg)
         begin
            Mout_oe_ram_temp = fifo_data_out[BITSIZE_Mout_addr_ram+BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag] == 1;
            Mout_we_ram_temp = fifo_data_out[BITSIZE_Mout_addr_ram+BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag] == 0;
            Mout_addr_ram_memory_temp = fifo_data_out[BITSIZE_Mout_addr_ram+BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag-1:BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag];
            Mout_Wdata_ram_temp = fifo_data_out[BITSIZE_Mout_Wdata_ram+BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag-1:BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag];
            Mout_data_ram_size_temp = fifo_data_out[BITSIZE_Mout_data_ram_size+BITSIZE_Mout_tag-1:BITSIZE_Mout_tag];
            Mout_tag_temp = fifo_data_out[BITSIZE_Mout_tag-1:0];
         end
         else
         begin
            Mout_oe_ram_temp = 0;
            Mout_we_ram_temp = 0;
            Mout_addr_ram_memory_temp = 0;
            Mout_Wdata_ram_temp = 0;
            Mout_data_ram_size_temp = 0;
            Mout_tag_temp = 0;
         end
      end
   end
   else
   begin

      assign Mout_back_pressure = Mout_back_pressure_memory;

      always @(*)
      begin
         Mout_oe_ram_temp = Mout_oe_ram;
         Mout_we_ram_temp = Mout_we_ram;
         Mout_addr_ram_memory_temp = Mout_addr_ram;
         Mout_Wdata_ram_temp = Mout_Wdata_ram;
         Mout_data_ram_size_temp = Mout_data_ram_size;
         Mout_tag_temp = Mout_tag;
      end
   end
endgenerate

generate
   if(USE_FILTER==1)
   begin
      kmp_bambu_memory_filter_in #(.INNER_TAG(INNER_TAG),
      .BITSIZE_M_Rdata_ram_memory(BITSIZE_M_Rdata_ram_memory),
      .BITSIZE_M_Rdata_ram_local(BITSIZE_M_Rdata_ram_local),
      .BITSIZE_M_Rdata_ram(BITSIZE_M_Rdata_ram),
      .BITSIZE_M_tag_memory(BITSIZE_M_tag_memory),
      .BITSIZE_M_tag_local(BITSIZE_M_tag_local),
      .BITSIZE_M_tag(BITSIZE_M_tag)) filter_in (.M_Rdata_ram(M_Rdata_ram),
      .M_DataRdy(M_DataRdy),
      .M_tag(M_tag),
      .M_Rdata_ram_memory(M_Rdata_ram_memory),
      .M_DataRdy_memory(M_DataRdy_memory),
      .M_tag_memory(M_tag_memory),
      .M_Rdata_ram_local(M_Rdata_ram_local),
      .M_DataRdy_local(M_DataRdy_local),
      .M_tag_local(M_tag_local),
      .M_back_pressure_local(M_back_pressure_local));
      kmp_bambu_memory_filter_out #(.INNER_TAG(INNER_TAG),
      .ROOT_SPACE_ALIGNMENT(ROOT_SPACE_ALIGNMENT),
      .EXTERNAL_BASE_ADDRESS(EXTERNAL_BASE_ADDRESS),
      .FORK_CALL_ID_START(FORK_CALL_ID_START),
      .FORK_CALL_ID_END(FORK_CALL_ID_END),
      .FORK_NUMBER(FORK_NUMBER),
      .BITSIZE_Mout_addr_ram(BITSIZE_Mout_addr_ram),
      .BITSIZE_Mout_Wdata_ram(BITSIZE_Mout_Wdata_ram),
      .BITSIZE_Mout_data_ram_size(BITSIZE_Mout_data_ram_size),
      .BITSIZE_Mout_tag(BITSIZE_Mout_tag),
      .BITSIZE_Mout_addr_ram_memory(BITSIZE_Mout_addr_ram_memory),
      .BITSIZE_Mout_Wdata_ram_memory(BITSIZE_Mout_Wdata_ram_memory),
      .BITSIZE_Mout_data_ram_size_memory(BITSIZE_Mout_data_ram_size_memory),
      .BITSIZE_Mout_tag_memory(BITSIZE_Mout_tag_memory),
      .BITSIZE_Mout_addr_ram_local(BITSIZE_Mout_addr_ram_local),
      .BITSIZE_Mout_Wdata_ram_local(BITSIZE_Mout_Wdata_ram_local),
      .BITSIZE_Mout_data_ram_size_local(BITSIZE_Mout_data_ram_size_local),
      .BITSIZE_Mout_tag_local(BITSIZE_Mout_tag_local)) filter_out (.Mout_oe_ram_memory(Mout_oe_ram_memory),
      .Mout_we_ram_memory(Mout_we_ram_memory),
      .Mout_addr_ram_memory(Mout_addr_ram_memory),
      .Mout_Wdata_ram_memory(Mout_Wdata_ram_memory),
      .Mout_data_ram_size_memory(Mout_data_ram_size_memory),
      .Mout_tag_memory(Mout_tag_memory),
      .Mout_oe_ram_local(Mout_oe_ram_local),
      .Mout_we_ram_local(Mout_we_ram_local),
      .Mout_addr_ram_local(Mout_addr_ram_local),
      .Mout_Wdata_ram_local(Mout_Wdata_ram_local),
      .Mout_data_ram_size_local(Mout_data_ram_size_local),
      .Mout_tag_local(Mout_tag_local),
      .Mout_oe_ram(Mout_oe_ram_temp),
      .Mout_we_ram(Mout_we_ram_temp),
      .Mout_addr_ram(Mout_addr_ram_memory_temp),
      .Mout_Wdata_ram(Mout_Wdata_ram_temp),
      .Mout_data_ram_size(Mout_data_ram_size_temp),
      .Mout_tag(Mout_tag_temp));
   end
   else
   begin
      assign M_DataRdy = M_DataRdy_memory;
      assign M_Rdata_ram = M_Rdata_ram_memory;
      assign M_tag = M_tag_memory;
      assign Mout_oe_ram_memory = Mout_oe_ram_temp;
      assign Mout_we_ram_memory = Mout_we_ram_temp;
      assign Mout_addr_ram_memory =  Mout_addr_ram_memory_temp;
      assign Mout_Wdata_ram_memory =  Mout_Wdata_ram_temp;
      assign Mout_data_ram_size_memory = Mout_data_ram_size_temp;
      assign Mout_tag_memory = Mout_tag_temp;
      assign M_back_pressure_local = 1'b0;
      assign Mout_oe_ram_local = 1'b0;
      assign Mout_we_ram_local = 1'b0;
      assign Mout_addr_ram_local = {BITSIZE_Mout_addr_ram{1'b0}};
      assign Mout_Wdata_ram_local = {BITSIZE_Mout_Wdata_ram{1'b0}};
      assign Mout_data_ram_size_local = {BITSIZE_Mout_data_ram_size{1'b0}};
      assign Mout_tag_local = {BITSIZE_Mout_tag_local{1'b0}};
   end
endgenerate
endmodule

// Generated from NC_OPENMP_IPs.before.xml
module simple_fifo #(
    parameter integer ADDR_WIDTH = 1,
    parameter integer BITSIZE_data_in = 1,
    parameter integer BITSIZE_data_out = 1
) (
    input wire clock,
    input wire reset,
    output wire empty,
    input wire [BITSIZE_data_in-1:0] data_in,
    output wire [BITSIZE_data_out-1:0] data_out,
    input wire rd_en,
    input wire wr_en
);

parameter RAM_DEPTH = (1 << ADDR_WIDTH);

reg [ADDR_WIDTH-1:0] wr_pointer;
reg [ADDR_WIDTH-1:0] wr_pointer_next;
reg [ADDR_WIDTH-1:0] rd_pointer;
reg [ADDR_WIDTH-1:0] rd_pointer_next;
reg [ADDR_WIDTH :0] status_cnt_reg;
wire [BITSIZE_data_in-1:0] data_ram ;

assign empty = status_cnt_reg == 0;

always @(posedge clock )
begin : WRITE_POINTER
  if(reset == 1'b0)
  begin
    wr_pointer = 0;
  end
  else
  begin
    wr_pointer = wr_pointer_next;
  end
end

always @(*)
begin
  if(wr_en)
  begin
    wr_pointer_next = wr_pointer + 1;
  end
  else
  begin
    wr_pointer_next = wr_pointer;
  end
end

always @(posedge clock )
begin : READ_POINTER
  if(reset == 1'b0)
  begin
    rd_pointer = 0;
  end
  else
  begin
    rd_pointer = rd_pointer_next;
  end
end

always @(*)
begin
  if(rd_en)
  begin
    rd_pointer_next = rd_pointer + 1;
  end
  else
  begin
    rd_pointer_next = rd_pointer;
  end
end

always @(posedge clock )
begin : STATUS_COUNTER
  if(reset == 1'b0)
  begin
    status_cnt_reg <= 0;
  end
  else if (rd_en && !wr_en  && (status_cnt_reg != 0))
  begin
    status_cnt_reg <= status_cnt_reg - 1;
  end
  else if (wr_en && !rd_en && (status_cnt_reg != RAM_DEPTH))
  begin
    status_cnt_reg <= status_cnt_reg + 1;
  end
end

std_sdp_bramfw_reference_adapter #(
  .BITSIZE_address_inr(ADDR_WIDTH),
  .BITSIZE_address_inw(ADDR_WIDTH),
  .BITSIZE_data_in(BITSIZE_data_in),
  .BITSIZE_data_out(BITSIZE_data_out),
  .MEMORY_INIT_file(""),
  .n_elements(RAM_DEPTH),
  .READ_ONLY_MEMORY(0),
  .HIGH_LATENCY(0)
  ) BRAM (
  .clock(clock),
  .write_enable(wr_en),
  .address_inr(rd_pointer_next),
  .address_inw(wr_pointer_next),
  .data_in(data_in),
  .data_out(data_out));
endmodule

module std_sdp_bramfw_reference_adapter #(
    parameter BITSIZE_address_inr = 1,
    parameter BITSIZE_address_inw = 1,
    parameter BITSIZE_data_in = 1,
    parameter BITSIZE_data_out = 1,
    parameter n_elements = 4,
    parameter MEMORY_INIT_file = "",
    parameter HIGH_LATENCY = 0,
    parameter READ_ONLY_MEMORY = 0
) (
    input wire clock,
    input wire write_enable,
    input wire [BITSIZE_address_inr-1:0] address_inr,
    input wire [BITSIZE_address_inw-1:0] address_inw,
    input wire [BITSIZE_data_in-1:0] data_in,
    output wire [BITSIZE_data_out-1:0] data_out
);
    std_sdp_bramfw_reference #(
        .DEPTH(n_elements), .INPUT_BITS(BITSIZE_data_in), .OUTPUT_BITS(BITSIZE_data_out),
        .READ_ADDRESS_BITS(BITSIZE_address_inr), .WRITE_ADDRESS_BITS(BITSIZE_address_inw),
        .HIGH_LATENCY(HIGH_LATENCY), .READ_ONLY_MEMORY(READ_ONLY_MEMORY),
        .MEMORY_INIT_file(MEMORY_INIT_file)) reference_model (
        .clock(clock), .write_enable(write_enable), .read_address(address_inr),
        .write_address(address_inw), .write_data(data_in), .read_data(data_out));
endmodule
