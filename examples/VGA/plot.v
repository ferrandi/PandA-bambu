`define MONOCHROME_FALSE 0
`define MONOCHROME_TRUE 1
`define RESOLUTION_320x240 0 
`define RESOLUTION_160x120 1
`define USING_DE1_FALSE 0
`define USING_DE1_TRUE 1

  // parameter BITS_PER_COLOUR_CHANNEL = 1;
  /* The number of bits per colour channel used to represent the colour of each pixel. A value
   * of 1 means that Red, Green and Blue colour channels will use 1 bit each to represent the intensity
   * of the respective colour channel. For BITS_PER_COLOUR_CHANNEL=1, the adapter can display 8 colours.
   * In general, the adapter is able to use 2^(3*BITS_PER_COLOUR_CHANNEL) colours. The number of colours is
   * limited by the screen resolution and the amount of on-chip memory available on the target device.
   */
  
  // parameter MONOCHROME = `MONOCHROME_FALSE;
  /* Set this parameter to MONOCHROME_TRUE if you only wish to use black and white colours. Doing so will reduce
   * the amount of memory you will use by a factor of 3. */
  
  // parameter RESOLUTION = `RESOLUTION_320x240;
  /* Set this parameter to RESOLUTION_160x120 or RESOLUTION_320x240. It will cause the VGA adapter to draw each dot on
   * the screen by using a block of 4x4 pixels ("160x120" resolution) or 2x2 pixels ("320x240" resolution).
   * It effectively reduces the screen resolution to an integer fraction of 640x480. It was necessary
   * to reduce the resolution for the Video Memory to fit within the on-chip memory limits.
   */
  
  // parameter USING_DE1 = `USING_DE1_TRUE;
  /* If set to USING_DE1_TRUE it adjust the offset of the drawing mechanism to account for the differences
   * between the DE2 and DE1 VGA digital to analogue converters. Set to USING_DE1_TRUE if and only if
   * you are running your circuit on a DE1 board. */


// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2015-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
module frequency_divider_byX(reset, clk_in, clk_div_by_X);
parameter DIVIDE_BY=2;
  input reset;
  input clk_in;
  output clk_div_by_X;
  reg clk_div_by_X=1'b0;

  generate
  if (DIVIDE_BY == 2)
  begin
    always @ (posedge clk_in or negedge reset)
    begin
      if (~reset) begin
        clk_div_by_X <= 1'b0;
      end
      else
      begin
        clk_div_by_X <= ~clk_div_by_X;
      end
    end
  end
  else if(DIVIDE_BY == 4)
  begin
    reg clk_div_by_X0=1'b0;
    always @ (posedge clk_in or negedge reset)
    begin
      if (~reset) begin
        clk_div_by_X0 <= 1'b0;
      end
      else
      begin
        clk_div_by_X0 <= ~clk_div_by_X0;
      end
    end
    always @ (posedge clk_div_by_X0 or negedge reset)
    begin
      if (~reset) begin
        clk_div_by_X <= 1'b0;
      end
      else
      begin
        clk_div_by_X <= ~clk_div_by_X;
      end
    end
  end
  endgenerate

endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2015-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
module VideoMemory(clk_w,clk_r,we_w,addr_w,addr_r,data_w,data_r);
  parameter BITSIZE_addr=1, BITSIZE_data=1, BITSIZE_nwords=1;
  input clk_w;
  input clk_r;
  input we_w;
  input [BITSIZE_addr-1:0] addr_w;
  input [BITSIZE_addr-1:0] addr_r;
  input [BITSIZE_data-1:0] data_w;
  output [BITSIZE_data-1:0] data_r;

  (* ramstyle = "no_rw_check" *) reg[BITSIZE_data-1:0] ram [BITSIZE_nwords-1:0];
  reg[BITSIZE_data-1:0] data_r;
  reg [BITSIZE_addr-1:0] addr_w_reg;
  reg [BITSIZE_data-1:0] data_w_reg;
  reg we_w_reg;

  always @(posedge clk_w) 
    begin
      if (we_w_reg)
        ram[addr_w_reg] <= data_w_reg;
      we_w_reg <= we_w;
      addr_w_reg <= addr_w;
      data_w_reg <= data_w;
    end
  always @(posedge clk_r) 
  begin
    data_r <= ram[addr_r];
  end
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2013-2020 Politecnico di Milano
// Author(s): Edoardo Giacomello <edoardo.giacomello@mail.polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module vga_address_translator(x, y, mem_address);
  parameter RESOLUTION = `RESOLUTION_320x240;
  input [((RESOLUTION == `RESOLUTION_320x240) ? (8) : (7)):0] x;
  input [((RESOLUTION == `RESOLUTION_320x240) ? (7) : (6)):0] y;
  output [((RESOLUTION == `RESOLUTION_320x240) ? (16) : (14)):0] mem_address;
  
  reg [((RESOLUTION == `RESOLUTION_320x240) ? (16) : (14)):0] mem_address;
  /* Set this parameter to RESOLUTION_160x120 or RESOLUTION_320x240. It will cause the VGA adapter to draw each dot on
   * the screen by using a block of 4x4 pixels ("160x120" resolution) or 2x2 pixels ("320x240" resolution).
   * It effectively reduces the screen resolution to an integer fraction of 640x480. It was necessary
   * to reduce the resolution for the Video Memory to fit within the on-chip memory limits.
   */
  
  
  /* The basic formula is address = y*WIDTH + x;
   * For 320x240 resolution we can write 320 as (256 + 64). Memory address becomes
   * (y*256) + (y*64) + x;
   * This simplifies multiplication a simple shift and add operation.
   * A leading 0 bit is added to each operand to ensure that they are treated as unsigned
   * inputs. By default the use a '+' operator will generate a signed adder.
   * Similarly, for 160x120 resolution we write 160 as 128+32.
   */
  wire [16:0] res_320x240 = ({1'b0, y, 8'd0} + {1'b0, y, 6'd0} + {1'b0, x});
  wire [15:0] res_160x120 = ({1'b0, y, 7'd0} + {1'b0, y, 5'd0} + {1'b0, x});
  
  always @(*)
  begin
    if (RESOLUTION == `RESOLUTION_320x240)
      mem_address = res_320x240;
  else
      mem_address = res_160x120[14:0];
  end
endmodule


// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2013-2020 Politecnico di Milano
// Author(s): Edoardo Giacomello <edoardo.giacomello@mail.polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module vga_controller(resetn, vga_clock, pixel_colour, memory_address, VGA_R, VGA_G, VGA_B, VGA_HS, VGA_VS, VGA_BLANK, VGA_SYNC, VGA_CLK);
parameter BITS_PER_COLOUR_CHANNEL = 1, MONOCHROME = `MONOCHROME_FALSE, RESOLUTION = `RESOLUTION_320x240, USING_DE1 = `USING_DE1_TRUE;
  input resetn;
  input vga_clock;
  input [((MONOCHROME == `MONOCHROME_TRUE) ? (0) : (BITS_PER_COLOUR_CHANNEL*3-1)):0] pixel_colour;
  output [((RESOLUTION == `RESOLUTION_320x240) ? (16) : (14)):0] memory_address;
  output [9:0] VGA_R;
  output [9:0] VGA_G;
  output [9:0] VGA_B;
  output VGA_HS;
  output VGA_VS;
  output VGA_BLANK;
  output VGA_SYNC;
  output VGA_CLK;
  reg [9:0] VGA_R;
  reg [9:0] VGA_G;
  reg [9:0] VGA_B;
  reg VGA_HS;
  reg VGA_VS;
  reg VGA_BLANK;
  
  //--- Timing parameters.
  /* Recall that the VGA specification requires a few more rows and columns are drawn
   * when refreshing the screen than are actually present on the screen. This is necessary to
   * generate the vertical and the horizontal syncronization signals. If you wish to use a
   * display mode other than 640x480 you will need to modify the parameters below as well
   * as change the frequency of the clock driving the monitor (VGA_CLK).
   */
  parameter C_VERT_NUM_PIXELS  = 11'd480;
  parameter C_VERT_SYNC_START  = 11'd493;
  parameter C_VERT_SYNC_END    = 11'd494; //(C_VERT_SYNC_START + 2 - 1); 
  parameter C_VERT_TOTAL_COUNT = 11'd525;
  
  parameter C_HORZ_NUM_PIXELS  = 11'd640;
  parameter C_HORZ_SYNC_START  = 11'd659;
  parameter C_HORZ_SYNC_END    = 11'd754; //(C_HORZ_SYNC_START + 96 - 1); 
  parameter C_HORZ_TOTAL_COUNT = 11'd800;
    
  
  /*****************************************************************************/
  /* Local Signals.                                                            */
  /*****************************************************************************/
  
  reg VGA_HS1;
  reg VGA_VS1;
  reg VGA_BLANK1; 
  reg [9:0] xCounter, yCounter;
  wire xCounter_clear;
  wire yCounter_clear;
  wire vcc;
  
  reg [((RESOLUTION == `RESOLUTION_320x240) ? (8) : (7)):0] x; 
  reg [((RESOLUTION == `RESOLUTION_320x240) ? (7) : (6)):0] y;
  /* Inputs to the converter. */
  
  /*****************************************************************************/
  /* Controller implementation.                                                */
  /*****************************************************************************/
  
  assign vcc =1'b1;
  
  /* A counter to scan through a horizontal line. */
  always @(posedge vga_clock or negedge resetn)
  begin
    if (!resetn)
      xCounter <= 10'd0;
    else if (xCounter_clear)
      xCounter <= 10'd0;
    else
    begin
      xCounter <= xCounter + 1'b1;
    end
  end
  assign xCounter_clear = (xCounter == (C_HORZ_TOTAL_COUNT-1));
  
  /* A counter to scan vertically, indicating the row currently being drawn. */
  always @(posedge vga_clock or negedge resetn)
  begin
    if (!resetn)
      yCounter <= 10'd0;
    else if (xCounter_clear && yCounter_clear)
      yCounter <= 10'd0;
    else if (xCounter_clear) //Increment when x counter resets
      yCounter <= yCounter + 1'b1;
  end
  assign yCounter_clear = (yCounter == (C_VERT_TOTAL_COUNT-1)); 
  
  /* Convert the xCounter/yCounter location from screen pixels (640x480) to our
   * local dots (320x240 or 160x120). Here we effectively divide x/y coordinate by 2 or 4,
   * depending on the resolution. */
  always @(*)
  begin
    if (RESOLUTION == `RESOLUTION_320x240)
    begin
      x = xCounter[9:1];
      y = yCounter[8:1];
    end
    else
    begin
      x = xCounter[9:2];
      y = yCounter[8:2];
    end
   end
  
  /* Change the (x,y) coordinate into a memory address. */
  vga_address_translator #(.RESOLUTION(RESOLUTION)) controller_translator(
            .x(x), .y(y), .mem_address(memory_address) );
  
  
  /* Generate the vertical and horizontal synchronization pulses. */
  always @(posedge vga_clock)
  begin
    //- Sync Generator (ACTIVE LOW)
    if (USING_DE1 == `USING_DE1_TRUE)
      VGA_HS1 <= ~((xCounter >= C_HORZ_SYNC_START-2) && (xCounter <= C_HORZ_SYNC_END-2));
    else
      VGA_HS1 <= ~((xCounter >= C_HORZ_SYNC_START) && (xCounter <= C_HORZ_SYNC_END));
    VGA_VS1 <= ~((yCounter >= C_VERT_SYNC_START) && (yCounter <= C_VERT_SYNC_END));
    
    //- Current X and Y is valid pixel range
    VGA_BLANK1 <= ((xCounter < C_HORZ_NUM_PIXELS) && (yCounter < C_VERT_NUM_PIXELS));
  
    //- Add 1 cycle delay
    VGA_HS <= VGA_HS1;
    VGA_VS <= VGA_VS1;
    VGA_BLANK <= VGA_BLANK1;
  end
  
  /* VGA sync should be 1 at all times. */
  assign VGA_SYNC = vcc;
  
  /* Generate the VGA clock signal. */
  assign VGA_CLK = vga_clock;
  
  /* Brighten the colour output. */
  // The colour input is first processed to brighten the image a little. Setting the top
  // bits to correspond to the R,G,B colour makes the image a bit dull. To brighten the image,
  // each bit of the colour is replicated through the 10 DAC colour input bits. For example,
  // when BITS_PER_COLOUR_CHANNEL is 2 and the red component is set to 2'b10, then the
  // VGA_R input to the DAC will be set to 10'b1010101010.
  
  integer index;
  integer sub_index;
  
  wire on_screen;
  
  assign on_screen = (USING_DE1 == `USING_DE1_TRUE) ?
    (({1'b0, xCounter} >= 2) & ({1'b0, xCounter} < C_HORZ_NUM_PIXELS+2) & ({1'b0, yCounter} < C_VERT_NUM_PIXELS)) :
    (({1'b0, xCounter} >= 0) & ({1'b0, xCounter} < C_HORZ_NUM_PIXELS+2) & ({1'b0, yCounter} < C_VERT_NUM_PIXELS));
  
  always @(pixel_colour or on_screen)
  begin
    VGA_R = 0;
    VGA_G = 0;
    VGA_B = 0;
    if (MONOCHROME == `MONOCHROME_FALSE)
    begin
      for (index = 10-BITS_PER_COLOUR_CHANNEL; index >= 0; index = index - BITS_PER_COLOUR_CHANNEL)
      begin
        for (sub_index = BITS_PER_COLOUR_CHANNEL - 1; sub_index >= 0; sub_index = sub_index - 1)
        begin
          VGA_R[sub_index+index] = on_screen & pixel_colour[sub_index + BITS_PER_COLOUR_CHANNEL*2];
          VGA_G[sub_index+index] = on_screen & pixel_colour[sub_index + BITS_PER_COLOUR_CHANNEL];
          VGA_B[sub_index+index] = on_screen & pixel_colour[sub_index];
        end
      end
    end
    else
    begin
      for (index = 0; index < 10; index = index + 1)
      begin
        VGA_R[index] = on_screen & pixel_colour[0:0];
        VGA_G[index] = on_screen & pixel_colour[0:0];
        VGA_B[index] = on_screen & pixel_colour[0:0];
      end
    end
  end
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2013-2020 Politecnico di Milano
// Author(s): Edoardo Giacomello <edoardo.giacomello@mail.polimi.it>, Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module plot(clock, reset, start_port, color, x, y, sel_plot, sel___plot, done_port, VGA_R, VGA_G, VGA_B, VGA_HS, VGA_VS, VGA_BLANK, VGA_SYNC, VGA_CLK);
parameter BITS_PER_COLOUR_CHANNEL = 1, MONOCHROME = `MONOCHROME_FALSE, RESOLUTION = `RESOLUTION_160x120, USING_DE1 = `USING_DE1_TRUE;
  input clock;
  input reset;
  input start_port;
  input [((MONOCHROME == `MONOCHROME_TRUE) ? (0) : (BITS_PER_COLOUR_CHANNEL*3-1)):0] color;
  input [8:0] x;
  input [7:0] y;
  input sel_plot;
  input sel___plot;
  output done_port;
  output [9:0] VGA_R;
  output [9:0] VGA_G;
  output [9:0] VGA_B;
  output VGA_HS;
  output VGA_VS;
  output VGA_BLANK;
  output VGA_SYNC;
  output VGA_CLK;
  
    /*****************************************************************************/
    /* Declare local signals here.                                               */
    /*****************************************************************************/
  
    wire valid_160x120;
  
    wire valid_320x240;
    /* Set to 1 if the specified coordinates are in a valid range for a given resolution.*/
  
    wire writeEn;
    /* This is a local signal that allows the Video Memory contents to be changed.
     * It depends on the screen resolution, the values of X and Y inputs, as well as 
     * the state of the plot signal.
     */
  
    wire [((MONOCHROME == `MONOCHROME_TRUE) ? (0) : (BITS_PER_COLOUR_CHANNEL*3-1)):0] to_ctrl_colour;
    /* Pixel color read by the VGA controller */
  
    wire [((RESOLUTION == `RESOLUTION_320x240) ? (16) : (14)):0] user_to_video_memory_addr;
    /* This bus specifies the address in memory the user must write
     * data to in order for the pixel intended to appear at location (X,Y) to be displayed
     * at the correct location on the screen.
     */
  
    wire [((RESOLUTION == `RESOLUTION_320x240) ? (16) : (14)):0] controller_to_video_memory_addr;  
    /* This bus specifies the address in memory the vga controller must read data from
     * in order to determine the color of a pixel located at coordinate (X,Y) of the screen.
     */
  
    wire clock_25;
    /* 25MHz clock generated by dividing the input clock frequency by 2. */
  
    /*****************************************************************************/
  
    /* Instances of modules for the VGA adapter.                                 */
  
    /*****************************************************************************/    
  
    vga_address_translator #(.RESOLUTION(RESOLUTION)) user_input_translator(
            .x(x), .y(y), .mem_address(user_to_video_memory_addr) );  
    /* Convert user coordinates into a memory address. */
  
    assign valid_160x120 = (({1'b0, x} >= 0) & ({1'b0, x} < 160) & ({1'b0, y} >= 0) & ({1'b0, y} < 120)) & (RESOLUTION == `RESOLUTION_160x120);
    assign valid_320x240 = (({1'b0, x} >= 0) & ({1'b0, x} < 320) & ({1'b0, y} >= 0) & ({1'b0, y} < 240)) & (RESOLUTION == `RESOLUTION_320x240);
    assign writeEn = (sel_plot) & (valid_160x120 | valid_320x240);
    /* Allow the user to plot a pixel if and only if the (X,Y) coordinates supplied are in a valid range. */
  
  
    VideoMemory #(.BITSIZE_addr((RESOLUTION == `RESOLUTION_320x240) ? (17) : (15)), .BITSIZE_data(MONOCHROME == `MONOCHROME_FALSE ? (BITS_PER_COLOUR_CHANNEL*3) : 1), .BITSIZE_nwords(((RESOLUTION == `RESOLUTION_320x240) ? (76800) : (19200)))) VideoMemory_i (.clk_w(clock), .clk_r(clock_25), .we_w(writeEn), .addr_w(user_to_video_memory_addr), .addr_r(controller_to_video_memory_addr), .data_w(color), .data_r(to_ctrl_colour));
    /* Video Memory. */

   frequency_divider_byX #(.DIVIDE_BY(2)) frequency_divider_byX_i(.reset(reset), .clk_in(clock), .clk_div_by_X(clock_25));
   /* This module generates a clock with half the frequency of the input clock.
    * For the VGA adapter to operate correctly the clock signal 'clock' must be
    * a 50MHz clock. The derived clock, which will then operate at 25MHz, is
    * required to set the monitor into the 640x480@60Hz display mode (also known as
    * the VGA mode).
    */
  
    vga_controller #(.BITS_PER_COLOUR_CHANNEL(BITS_PER_COLOUR_CHANNEL), .MONOCHROME(MONOCHROME), .RESOLUTION(RESOLUTION), .USING_DE1(USING_DE1)) controller(
        .vga_clock(clock_25),
        .resetn(reset),
        .pixel_colour(to_ctrl_colour),
        .memory_address(controller_to_video_memory_addr), 
        .VGA_R(VGA_R),
        .VGA_G(VGA_G),
        .VGA_B(VGA_B),
        .VGA_HS(VGA_HS),
        .VGA_VS(VGA_VS),
        .VGA_BLANK(VGA_BLANK),
        .VGA_SYNC(VGA_SYNC),
        .VGA_CLK(VGA_CLK)
      );
    

endmodule

