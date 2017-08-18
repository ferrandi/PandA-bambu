module sum(start_port, done_port, return_port, clock, a, b);

input [31:0] a;
input [31:0] b;
input start_port;
input clock;

output [31:0] return_port;
output done_port;

assign return_port = a + b;

endmodule

