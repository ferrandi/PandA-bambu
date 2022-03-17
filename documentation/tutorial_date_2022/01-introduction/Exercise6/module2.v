module module2_IP
  (input wire        clock,
   input wire        reset,

   input wire        start_port,
   output reg        done_port,

   input wire [31:0] input1,

   output reg [63:0] output1,
   output reg [63:0] output2,
   output reg [15:0] output3);

   reg        done_port_reg;
   reg [63:0] output1_reg;
   reg [63:0] output2_reg;
   reg [15:0] output3_reg;


   //----------------------------------------------------------------
   // Simulate processing on input
   //----------------------------------------------------------------

   always @(posedge clock) begin
      if (!reset) begin
         done_port_reg <= 0;
         output1_reg <= 0;
         output2_reg <= 0;
         output3_reg <= 0;
      end
      else begin
         done_port_reg <= start_port;
         output1_reg <= input1 * input1;
         output2_reg <= {input1, input1};
         output3_reg <= input1[15:0];
      end
   end


   //----------------------------------------------------------------
   // Outputs, two cycle latency
   //----------------------------------------------------------------

   always @(posedge clock) begin
      if (!reset) begin
         done_port <= 0;
         output1 <= 0;
         output2 <= 0;
         output3 <= 0;
      end
      else begin
         done_port <= done_port_reg;
         output1 <= output1_reg;
         output2 <= output2_reg;
         output3 <= output3_reg;
      end
   end

endmodule

module module2 (clock, reset, start_port, input1, outputs, done_port, Min_oe_ram, Mout_oe_ram, Min_we_ram, Mout_we_ram, Min_addr_ram, Mout_addr_ram, M_Rdata_ram, Min_Wdata_ram, Mout_Wdata_ram, Min_data_ram_size, Mout_data_ram_size, M_DataRdy);
  parameter BITSIZE_outputs=1, BITSIZE_Min_addr_ram=1, BITSIZE_Mout_addr_ram=1, BITSIZE_M_Rdata_ram=8, BITSIZE_Min_Wdata_ram=8, BITSIZE_Mout_Wdata_ram=8, BITSIZE_Min_data_ram_size=1, BITSIZE_Mout_data_ram_size=1;
  // IN
  input clock;
  input reset;
  input start_port;
  input [31:0] input1;
  input [BITSIZE_outputs-1:0] outputs;
  input Min_oe_ram;
  input Min_we_ram;
  input [BITSIZE_Min_addr_ram-1:0] Min_addr_ram;
  input [BITSIZE_M_Rdata_ram-1:0] M_Rdata_ram;
  input [BITSIZE_Min_Wdata_ram-1:0] Min_Wdata_ram;
  input [BITSIZE_Min_data_ram_size-1:0] Min_data_ram_size;
  input M_DataRdy;
  // OUT
  output done_port;
  output Mout_oe_ram;
  output Mout_we_ram;
  output [BITSIZE_Mout_addr_ram-1:0] Mout_addr_ram;
  output [BITSIZE_Mout_Wdata_ram-1:0] Mout_Wdata_ram;
  output [BITSIZE_Mout_data_ram_size-1:0] Mout_data_ram_size;

  wire [63:0] output1_int;
  wire [63:0] output2_int;
  wire [15:0] output3_int;
  reg [63:0] output1_reg;
  reg [63:0] output2_reg;
  reg [15:0] output3_reg;

  reg done_port;
  wire       done_port_my_ip;
  wire       start_port_fsm;
  reg        start_port_memstore;
  wire        done_port_memstore;
  reg [63:0] data_int;
  reg [BITSIZE_outputs-1:0] addr_int;
  reg [6:0] size_int;

  reg Min_oe_ram_int;
  reg Min_we_ram_int;
  reg [BITSIZE_Min_addr_ram-1:0] Min_addr_ram_int;
  reg [BITSIZE_Min_Wdata_ram-1:0] Min_Wdata_ram_int;
  reg [BITSIZE_Min_data_ram_size-1:0] Min_data_ram_size_int;
  parameter [1:0] S_0 = 2'd0,
    S_1 = 2'd1,
    S_2 = 2'd2,
    S_3 = 2'd3;
  reg [1:0] _present_state=S_0, _next_state;

  module2_IP my_module2_IP (.done_port(done_port_my_ip), .clock(clock), .reset(reset), .start_port(start_port), .input1(input1), .output1(output1_int),  .output2(output2_int),  .output3(output3_int));
  assign start_port_fsm = done_port_my_ip;

  __builtin_memstore #(.BITSIZE_data(64), .BITSIZE_addr(BITSIZE_outputs), .BITSIZE_size(7), .BITSIZE_Min_addr_ram(BITSIZE_Min_addr_ram), .BITSIZE_Mout_addr_ram(BITSIZE_Mout_addr_ram), .BITSIZE_M_Rdata_ram(BITSIZE_M_Rdata_ram), .BITSIZE_Min_Wdata_ram(BITSIZE_Min_Wdata_ram), .BITSIZE_Mout_Wdata_ram(BITSIZE_Mout_Wdata_ram), .BITSIZE_Min_data_ram_size(BITSIZE_Min_data_ram_size), .BITSIZE_Mout_data_ram_size(BITSIZE_Mout_data_ram_size)) my__builtin_memstore (.clock(clock), .reset(reset), .start_port(start_port_memstore), .data(data_int), .addr(addr_int), .size(size_int), .done_port(done_port_memstore), .Min_oe_ram(Min_oe_ram_int), .Mout_oe_ram(Mout_oe_ram), .Min_we_ram(Min_we_ram_int), .Mout_we_ram(Mout_we_ram), .Min_addr_ram(Min_addr_ram_int), .Mout_addr_ram(Mout_addr_ram), .M_Rdata_ram(M_Rdata_ram), .Min_Wdata_ram(Min_Wdata_ram_int), .Mout_Wdata_ram(Mout_Wdata_ram), .Min_data_ram_size(Min_data_ram_size_int), .Mout_data_ram_size(Mout_data_ram_size), .M_DataRdy(M_DataRdy));

  always @(posedge clock or negedge reset)
    if (!reset)
    begin
      _present_state <= S_0;
    end
    else 
      _present_state <= _next_state;

  always @(posedge clock or negedge reset)
    if (!reset)
    begin
      output1_reg <= 0;
      output2_reg <= 0;
      output3_reg <= 0;
    end
    else if(done_port_my_ip == 1'b1)
    begin
      output1_reg <= output1_int;
      output2_reg <= output2_int;
      output3_reg <= output3_int;
    end

  always @(*)
  begin
    _next_state=S_0;
    done_port=1'b0;
    start_port_memstore=1'b0;
    addr_int=0;
    data_int=0;
    size_int=0;
    Min_oe_ram_int=Min_oe_ram;
    Min_we_ram_int=Min_we_ram;
    Min_data_ram_size_int=Min_data_ram_size;
    Min_Wdata_ram_int=Min_Wdata_ram;
    Min_addr_ram_int=Min_addr_ram;
    case (_present_state)
      S_0 :
        if(start_port_fsm != 1'b1)
        begin
          _next_state=S_0;
        end
        else
        begin
          _next_state=S_1;
        end
      S_1 :
      begin
        _next_state=S_1;
        start_port_memstore=1'b1;
        addr_int=outputs;
        data_int=output1_reg;
        size_int=64;
        if(done_port_memstore)
        begin
          _next_state=S_2;
        end
      end
      S_2 :
      begin
        _next_state=S_2;
        start_port_memstore=1'b1;
        addr_int=outputs+64/8;
        data_int=output2_reg;
        size_int=64;
        if(done_port_memstore)
        begin
          _next_state=S_3;
        end
      end
      S_3 :
      begin
        _next_state=S_3;
        start_port_memstore=1'b1;
        addr_int=outputs+(64+64)/8;
        size_int=16;
        data_int=output3_reg;
        if(done_port_memstore)
        begin
          _next_state=S_0;
          done_port=1'b1;
        end
      end
    endcase
  end
endmodule

