module plus(clock, reset, start_port, done_port, in1, in2, retval);
	input clock;
	input reset;
	input start_port;
	input [32:0] in1;
	input [32:0] in2;
	output done_port;
	output [32:0] retval;
	assign done_port = start_port;
	assign retval = in1 + in2;
endmodule

module minus(clock, reset, start_port, done_port, in1, in2, retval);
	input clock;
	input reset;
	input start_port;
	input [32:0] in1;
	input [32:0] in2;
	output done_port;
	output [32:0] retval;
	assign done_port = start_port;
	assign retval = in1 - in2;
endmodule

module times(clock, reset, start_port, done_port, in1, in2, retval);
	input clock;
	input reset;
	input start_port;
	input [32:0] in1;
	input [32:0] in2;
	output done_port;
	output [32:0] retval;
	assign done_port = start_port;
	assign retval = in1 * in2;
endmodule

module divide(clock, reset, start_port, done_port, in1, in2, retval);
	input clock;
	input reset;
	input start_port;
	input [32:0] in1;
	input [32:0] in2;
	output done_port;
	output [32:0] retval;
	assign done_port = start_port;
	assign retval = in1 / in2;
endmodule

module times2(clock, reset, start_port, done_port, in1, retval);
	input clock;
	input reset;
	input start_port;
	input [32:0] in1;
	output done_port;
	output [32:0] retval;
	assign done_port = start_port;
	assign retval = (in1 << 1) ^ 32'b01;
endmodule
