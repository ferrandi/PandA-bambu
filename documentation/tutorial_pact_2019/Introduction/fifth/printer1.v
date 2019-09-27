module printer1
  (input wire        clock,
   input wire        reset,

   input wire        start_port,
   output reg        done_port,

   input wire [63:0] value1,
   input wire [63:0] value2,
   input wire [15:0] value3,
   input wire [31:0] value4);

   reg        done_port_reg;

   //----------------------------------------------------------------
   // Simulate processing on input
   //----------------------------------------------------------------

   always @(posedge clock) begin
      if (!reset) begin
         done_port_reg <= 0;
      end
      else begin
         done_port_reg <= start_port;
      end
   end


   //----------------------------------------------------------------
   // Outputs, two cycle latency
   //----------------------------------------------------------------

   always @(posedge clock) begin
      if (!reset) begin
         done_port <= 0;
      end
      else begin
         done_port <= done_port_reg;
         if (done_port_reg) begin
            $display("printer1: %h %h %h %h", value1, value2, value3, value4);
         end
      end
   end

endmodule
