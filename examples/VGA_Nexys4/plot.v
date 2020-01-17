// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module vga_address_translator(x, y, mem_address);
  parameter horizontal_display_width = 640, BITSIZE_memory_address=32;
  input [BITSIZE_memory_address-1:0] x;
  input [BITSIZE_memory_address-1:0] y;
  output [BITSIZE_memory_address-1:0] mem_address;
  
  reg [BITSIZE_memory_address-1:0] mem_address;
  
  /* The basic formula is address = y*WIDTH + x;
   * For 320x240 resolution we can write 320 as (256 + 64). Memory address becomes
   * (y*256) + (y*64) + x;
   * This simplifies multiplication a simple shift and add operation.
   */  
  always @(*)
  begin
    if (horizontal_display_width == 640)
      mem_address = (((y << 2) + y) << 7) + x;
  else if(horizontal_display_width == 320)
      mem_address = (((y << 2) + y) << 6) + x;
  else if(horizontal_display_width == 160)
      mem_address = (((y << 2) + y) << 5) + x;
  else
      mem_address = y*horizontal_display_width + x;
  end
endmodule

// This component is part of the BAMBU/PANDA IP LIBRARY
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
module generic_vga_controller(reset, clk_in, pixel_colour, memory_address, VGA_R, VGA_G, VGA_B, VGA_HS, VGA_VS, VGA_BLANK, VGA_SYNC, VGA_CLK);
parameter horizontal_sync_pulse_width =96, //horizontal sync pulse width in pixels
    horizontal_back_porch_width = 48,      //horizontal back porch width in pixels
    horizontal_display_width = 640,        //horizontal display width in pixels
    horizontal_front_porch_width = 16,     //horizontal front porch width in pixels
    horizontal_sync_pulse_polarity= 0,     //horizontal sync pulse polarity (1 = positive, 0 = negative)
    vertical_sync_pulse_width = 2,         //vertical sync pulse width in rows
    vertical_back_porch_width = 33,        //vertical back porch width in rows
    vertical_display_width = 480,          //vertical display width in rows
    vertical_front_porch_width = 10,       //vertical front porch width in rows
    vertical_sync_pulse_polarity = 0,      //vertical sync pulse polarity (1 = positive, 0 = negative)
    MONOCHROME = 0,                        //monocromatic display of colors (1 = only black and white, 0 use RGB coloring encoding)
    BITS_PER_COLOUR_CHANNEL = 4,           //number of bits used by the RGB channels
    RGB_DIGITAL_BITSIZE = 4,               //number of bits for the RGB DAC
    BITSIZE_memory_address = 32;           //number of bits used to address the VideoMemory
    
  input reset;
  input clk_in;
  input [(MONOCHROME==1 ? 0 : BITS_PER_COLOUR_CHANNEL*3-1):0] pixel_colour;
  output [BITSIZE_memory_address-1:0] memory_address;
  output [RGB_DIGITAL_BITSIZE-1:0] VGA_R;
  output [RGB_DIGITAL_BITSIZE-1:0] VGA_G;
  output [RGB_DIGITAL_BITSIZE-1:0] VGA_B;
  output VGA_HS;
  output VGA_VS;
  output VGA_BLANK;
  output VGA_SYNC;
  output VGA_CLK;

  reg [RGB_DIGITAL_BITSIZE-1:0] VGA_R;
  reg [RGB_DIGITAL_BITSIZE-1:0] VGA_G;
  reg [RGB_DIGITAL_BITSIZE-1:0] VGA_B;
  reg [RGB_DIGITAL_BITSIZE-1:0] VGA_R_int;
  reg [RGB_DIGITAL_BITSIZE-1:0] VGA_G_int;
  reg [RGB_DIGITAL_BITSIZE-1:0] VGA_B_int;
  reg VGA_HS;
  reg VGA_VS;
  reg VGA_BLANK;

  wire on_screen;
  reg on_screen_delayed;
  reg [BITSIZE_memory_address-1:0] horizontal_counter=0;
  reg [BITSIZE_memory_address-1:0] vertical_counter=0;
  reg [BITSIZE_memory_address-1:0] horizontal_counter_delayed;
  reg [BITSIZE_memory_address-1:0] vertical_counter_delayed;
  wire [BITSIZE_memory_address-1:0] memory_address_temp;

  always @(posedge clk_in)
    on_screen_delayed <= on_screen;
  always @(posedge clk_in)
    horizontal_counter_delayed <= horizontal_counter;
  always @(posedge clk_in)
    vertical_counter_delayed <= vertical_counter;

  assign on_screen = horizontal_counter < horizontal_display_width && vertical_counter < vertical_display_width;
  /* Change the (x,y) coordinate into a memory address. */
  vga_address_translator #(.horizontal_display_width(horizontal_display_width), .BITSIZE_memory_address(BITSIZE_memory_address)) controller_translator(
            .x(horizontal_counter), .y(vertical_counter), .mem_address(memory_address_temp) );

  assign memory_address = on_screen ? memory_address_temp : 0;
  assign VGA_CLK = clk_in;
  
  always @(posedge clk_in)
    VGA_BLANK <= on_screen_delayed;
  assign VGA_SYNC = 1'b0;

  always @(posedge clk_in)
  begin
    if (~reset)
    begin
      horizontal_counter <= 0;
      vertical_counter <= 0;
    end
    else
    begin
      if(horizontal_counter < horizontal_sync_pulse_width + horizontal_back_porch_width + horizontal_display_width + horizontal_front_porch_width - 1)
        horizontal_counter <= horizontal_counter + 1;
      else
      begin
        horizontal_counter <= 0;
        if(vertical_counter < vertical_sync_pulse_width + vertical_back_porch_width + vertical_display_width + vertical_front_porch_width - 1)
          vertical_counter <= vertical_counter + 1;
        else
          vertical_counter <= 0;
      end
    end
  end
  always @(posedge clk_in)
    VGA_HS <= (horizontal_counter_delayed < horizontal_display_width + horizontal_front_porch_width || horizontal_counter_delayed >= horizontal_display_width + horizontal_front_porch_width + horizontal_sync_pulse_width)
                  ? ~horizontal_sync_pulse_polarity
                  : horizontal_sync_pulse_polarity;
  always @(posedge clk_in)
    VGA_VS <= (vertical_counter_delayed < vertical_display_width + vertical_front_porch_width || vertical_counter_delayed >= vertical_display_width + vertical_front_porch_width + vertical_sync_pulse_width)
                  ? ~vertical_sync_pulse_polarity
                  : vertical_sync_pulse_polarity;

  always @(*)
  begin : RGB_controller
    integer index;
    integer sub_index;
    VGA_R_int = 0;
    VGA_G_int = 0;
    VGA_B_int = 0;
    if (MONOCHROME == 0)
    begin
      for (index = RGB_DIGITAL_BITSIZE-BITS_PER_COLOUR_CHANNEL; index >= 0; index = index - BITS_PER_COLOUR_CHANNEL)
      begin
        for (sub_index = BITS_PER_COLOUR_CHANNEL - 1; sub_index >= 0; sub_index = sub_index - 1)
        begin
          VGA_R_int[sub_index+index] = on_screen_delayed & pixel_colour[sub_index + BITS_PER_COLOUR_CHANNEL*2];
          VGA_G_int[sub_index+index] = on_screen_delayed & pixel_colour[sub_index + BITS_PER_COLOUR_CHANNEL];
          VGA_B_int[sub_index+index] = on_screen_delayed & pixel_colour[sub_index];
        end
      end
    end
    else
    begin
      for (index = 0; index < RGB_DIGITAL_BITSIZE; index = index + 1)
      begin
        VGA_R_int[index] = on_screen_delayed & pixel_colour[0:0];
        VGA_G_int[index] = on_screen_delayed & pixel_colour[0:0];
        VGA_B_int[index] = on_screen_delayed & pixel_colour[0:0];
      end
    end
  end
  always @(posedge clk_in)
    VGA_R <= VGA_R_int;
  always @(posedge clk_in)
    VGA_G <= VGA_G_int;
  always @(posedge clk_in)
    VGA_B <= VGA_B_int;
 
endmodule

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
    always @ (posedge clk_in)
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
    always @ (posedge clk_in)
    begin
      if (~reset) begin
        clk_div_by_X0 <= 1'b0;
      end
      else
      begin
        clk_div_by_X0 <= ~clk_div_by_X0;
      end
    end
    always @ (posedge clk_div_by_X0)
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


module clk_generator_25_170MHz(reset, clk_in1, clk_out1);
  input reset;
  // Clock in ports
  input         clk_in1;
  // Clock out ports
  output        clk_out1;

  // Instantiation of the MMCM PRIMITIVE
  //    * Unused inputs are tied off
  //    * Unused outputs are labeled unused
  wire [15:0] do_unused;
  wire drdy_unused;
  wire psdone_unused;
  wire locked_int;
  wire clkfbout_int1;
  wire clkfbout_buf_int1;
  wire clkfboutb_unused;
  wire clkout0b_unused;
  wire clkout1_unused;
  wire clkout1b_unused;
  wire clkout2_unused;
  wire clkout2b_unused;
  wire clkout3_unused;
  wire clkout3b_unused;
  wire clkout4_unused;
  wire clkout5_unused;
  wire clkout6_unused;
  wire clkfbstopped_unused;
  wire clkinstopped_unused;
  wire reset_high;


  // Input buffering
  //------------------------------------
  assign clk_in1_int1 = clk_in1;

  // Output buffering
  //-----------------------------------

  BUFG clkf_buf
   (.O (clkfbout_buf_int1),
    .I (clkfbout_int1));


  BUFG clkout1_buf
   (.O   (clk_out1),
    .I   (clk_out1_int1));

  assign reset_high = ~reset; 

  MMCME2_ADV
  #(.BANDWIDTH            ("OPTIMIZED"),
    .CLKOUT4_CASCADE      ("FALSE"),
    .COMPENSATION         ("ZHOLD"),
    .STARTUP_WAIT         ("FALSE"),
    .DIVCLK_DIVIDE        (4),
    .CLKFBOUT_MULT_F      (36.375),
    .CLKFBOUT_PHASE       (0.000),
    .CLKFBOUT_USE_FINE_PS ("FALSE"),
    .CLKOUT0_DIVIDE_F     (36.125),
    .CLKOUT0_PHASE        (0.000),
    .CLKOUT0_DUTY_CYCLE   (0.500),
    .CLKOUT0_USE_FINE_PS  ("FALSE"),
    .CLKIN1_PERIOD        (10.0))
  mmcm_adv_inst
    // Output clocks
   (
    .CLKFBOUT            (clkfbout_int1),
    .CLKFBOUTB           (clkfboutb_unused),
    .CLKOUT0             (clk_out1_int1),
    .CLKOUT0B            (clkout0b_unused),
    .CLKOUT1             (clkout1_unused),
    .CLKOUT1B            (clkout1b_unused),
    .CLKOUT2             (clkout2_unused),
    .CLKOUT2B            (clkout2b_unused),
    .CLKOUT3             (clkout3_unused),
    .CLKOUT3B            (clkout3b_unused),
    .CLKOUT4             (clkout4_unused),
    .CLKOUT5             (clkout5_unused),
    .CLKOUT6             (clkout6_unused),
     // Input clock control
    .CLKFBIN             (clkfbout_buf_int1),
    .CLKIN1              (clk_in1_int1),
    .CLKIN2              (1'b0),
     // Tied to always select the primary input clock
    .CLKINSEL            (1'b1),
    // Ports for dynamic reconfiguration
    .DADDR               (7'h0),
    .DCLK                (1'b0),
    .DEN                 (1'b0),
    .DI                  (16'h0),
    .DO                  (do_unused),
    .DRDY                (drdy_unused),
    .DWE                 (1'b0),
    // Ports for dynamic phase shift
    .PSCLK               (1'b0),
    .PSEN                (1'b0),
    .PSINCDEC            (1'b0),
    .PSDONE              (psdone_unused),
    // Other control and status signals
    .LOCKED              (locked_int),
    .CLKINSTOPPED        (clkinstopped_unused),
    .CLKFBSTOPPED        (clkfbstopped_unused),
    .PWRDWN              (1'b0),
    .RST                 (reset_high));


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
// Copyright (C) 2016-2020 Politecnico di Milano
// Author(s): Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
// License: PANDA_LGPLv3
`timescale 1ns / 1ps
module plot(clock, reset, start_port, color, x, y, sel_plot, sel___plot, done_port, VGA_R, VGA_G, VGA_B, VGA_HS, VGA_VS);
parameter horizontal_sync_pulse_width =96, //horizontal sync pulse width in pixels
    horizontal_back_porch_width = 48,      //horizontal back porch width in pixels
    horizontal_display_width = 640,        //horizontal display width in pixels
    horizontal_front_porch_width = 16,     //horizontal front porch width in pixels
    horizontal_sync_pulse_polarity= 0,     //horizontal sync pulse polarity (1 = positive, 0 = negative)
    vertical_sync_pulse_width = 2,         //vertical sync pulse width in rows
    vertical_back_porch_width = 33,        //vertical back porch width in rows
    vertical_display_width = 480,          //vertical display width in rows
    vertical_front_porch_width = 10,       //vertical front porch width in rows
    vertical_sync_pulse_polarity = 0,      //vertical sync pulse polarity (1 = positive, 0 = negative)
    MONOCHROME = 0,                        //monocromatic display of colors (1 = only black and white, 0 use RGB color encoding)
    BITS_PER_COLOUR_CHANNEL = 1,           //number of bits used by the RGB channels
    RGB_DIGITAL_BITSIZE = 4,               //number of bits for the RGB DAC
    BITSIZE_memory_address = 19,           //number of bits used to address the VideoMemory
    USE_PLL = 1;                           // how to generate VGA clock_in (1 = use a PLL, 0 = use simple logic)
  input clock;
  input reset;
  input start_port;
  input [(MONOCHROME==1 ? 0 : BITS_PER_COLOUR_CHANNEL*3-1):0] color;
  input [BITSIZE_memory_address-1:0] x;
  input [BITSIZE_memory_address-1:0] y;
  input sel_plot;
  input sel___plot;
  output done_port;
  output [RGB_DIGITAL_BITSIZE-1:0] VGA_R;
  output [RGB_DIGITAL_BITSIZE-1:0] VGA_G;
  output [RGB_DIGITAL_BITSIZE-1:0] VGA_B;
  output VGA_HS;
  output VGA_VS;
  
  /*****************************************************************************/
  /* Declare local signals here.                                               */
  /*****************************************************************************/
  
  
  
  /* This is a local signal that allows the Video Memory contents to be changed.
   * It depends on the screen resolution, the values of X and Y inputs, as well as 
   * the state of the plot signal.
   */
  wire writeEn;
  
  /* Pixel color read by the VGA controller */
  wire [(MONOCHROME==1 ? 0 : BITS_PER_COLOUR_CHANNEL*3-1):0] to_ctrl_colour;
  
  /* This bus specifies the address in memory the user must write
   * data to in order for the pixel intended to appear at location (X,Y) to be displayed
   * at the correct location on the screen.
   */
  wire [BITSIZE_memory_address-1:0] user_to_video_memory_addr;
  
  /* This bus specifies the address in memory the vga controller must read data from
   * in order to determine the color of a pixel located at coordinate (X,Y) of the screen.
   */
  wire [BITSIZE_memory_address-1:0] controller_to_video_memory_addr;  
  
  /* 25MHz clock generated by dividing the input clock frequency by 4. */
  wire clock_25;
  
  /* Convert user coordinates into a memory address. */
  vga_address_translator #(.horizontal_display_width(horizontal_display_width), .BITSIZE_memory_address(BITSIZE_memory_address)) controller_translator(
            .x(x), .y(y), .mem_address(user_to_video_memory_addr) );
  
  /* Allow the user to plot a pixel if and only if the (X,Y) coordinates supplied are in a valid range. */
  assign writeEn = (sel_plot) && (x < horizontal_display_width && y < vertical_display_width);

  
  /* Video Memory. */
  VideoMemory #(.BITSIZE_addr(BITSIZE_memory_address), .BITSIZE_data(MONOCHROME ? 1 : BITS_PER_COLOUR_CHANNEL*3), .BITSIZE_nwords(horizontal_display_width*vertical_display_width)) VideoMemory_i (.clk_w(clock), .clk_r(clock_25), .we_w(writeEn), .addr_w(user_to_video_memory_addr), .addr_r(controller_to_video_memory_addr), .data_w(color), .data_r(to_ctrl_colour));

  /* This module generates a clock with a quarter of the frequency of the input clock.
   * For the VGA adapter to operate correctly the clock signal 'clock' must be
   * a 100MHz clock. The derived clock, which will then operate at 25MHz, is
   * required to set the monitor into the 640x480@60Hz display mode (also known as
   * the VGA mode).
  */
  generate
  if (USE_PLL == 1)
    clk_generator_25_170MHz clk_generator_25_170MHz_i(.reset(reset), .clk_in1(clock), .clk_out1(clock_25));
  else
    frequency_divider_byX #(.DIVIDE_BY(4)) frequency_divider_byX_i(.reset(reset), .clk_in(clock), .clk_div_by_X(clock_25));
  endgenerate

  generic_vga_controller #(.horizontal_sync_pulse_width(horizontal_sync_pulse_width), .horizontal_back_porch_width(horizontal_back_porch_width), 
                           .horizontal_display_width(horizontal_display_width), .horizontal_front_porch_width(horizontal_front_porch_width), 
                           .horizontal_sync_pulse_polarity(horizontal_sync_pulse_polarity), .vertical_sync_pulse_width(vertical_sync_pulse_width),
                           .vertical_back_porch_width(vertical_back_porch_width), .vertical_display_width(vertical_display_width),
                           .vertical_front_porch_width(vertical_front_porch_width), .vertical_sync_pulse_polarity(vertical_sync_pulse_polarity),
                           .MONOCHROME(MONOCHROME), .BITS_PER_COLOUR_CHANNEL(BITS_PER_COLOUR_CHANNEL),
                           .RGB_DIGITAL_BITSIZE(RGB_DIGITAL_BITSIZE), .BITSIZE_memory_address(BITSIZE_memory_address))

  controller(
        .reset(reset),
        .clk_in(clock_25),
        .pixel_colour(to_ctrl_colour),
        .memory_address(controller_to_video_memory_addr), 
        .VGA_R(VGA_R),
        .VGA_G(VGA_G),
        .VGA_B(VGA_B),
        .VGA_HS(VGA_HS),
        .VGA_VS(VGA_VS)
      );
    

endmodule

