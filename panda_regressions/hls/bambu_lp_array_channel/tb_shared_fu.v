module tb_shared_fu;
   reg clock = 1'b0;
   always #5 clock = ~clock;

   reg reset = 1'b0;
   reg start_port = 1'b0;
   reg [31:0] a = 0, b = 0, c = 0;
   wire done_port, idle_port;
   wire [31:0] return_port;

   p_Z9shared_fuiii dut (
      .clock(clock), .reset(reset), .start_port(start_port),
      .a(a), .b(b), .c(c), .done_port(done_port), .idle_port(idle_port), .return_port(return_port)
   );

   task transact;
      input [31:0] av, bv, cv, expected;
      begin
         @(negedge clock);
         a = av;
         b = bv;
         c = cv;
         start_port = 1'b1;
         @(posedge clock);
         #1 start_port = 1'b0;
         wait(done_port === 1'b1);
         #1;
         if(return_port !== expected)
            $fatal(1, "shared FU result mismatch: %0d * %0d * %0d got %0d expected %0d",
                   av, bv, cv, return_port, expected);
         wait(done_port === 1'b0);
      end
   endtask

   initial begin
      #2000 $fatal(1, "shared FU transaction timeout");
   end

   initial begin
      @(posedge clock);
      #1 reset = 1'b1;
      transact(3, 7, 5, 105);
      transact(5, 9, 3, 135);
      transact(11, 4, 7, 308);
      transact(13, 2, 6, 156);
      $display("PASS shared FU transactions=4");
      $finish;
   end
endmodule
