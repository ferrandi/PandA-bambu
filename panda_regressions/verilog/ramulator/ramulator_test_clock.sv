// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: MIT
// Exercise the production SystemMEM clock even when the fake accelerator uses AXI only.
module ramulator_test_clock(input wire clock, reset, output wire ramulator_phase);
  reg setup_reg = 1;
  always @(negedge clock)
  begin
    setup_reg <= reset;
  end

  TestbenchMEMRamulatorIntegrated clock_model(
      .clock(clock), .reset(reset), .setup_port(setup_reg), .done_port(1'b0),
      .ramulator_phase(ramulator_phase), .Mout_oe_ram(1'b0), .Mout_we_ram(1'b0),
      .Mout_addr_ram(1'b0), .Mout_Wdata_ram(8'b0), .Mout_data_ram_size(4'b0),
      .Mout_tag(1'b0), .M_back_pressure(1'b0), .M_DataRdy(), .M_Rdata_ram(),
      .Mout_back_pressure(), .M_tag());

  reg last_phase_reg = 0;
  integer phase_count_reg = 0;
  always @(ramulator_phase or reset)
  begin
    if(reset)
    begin
      last_phase_reg = ramulator_phase;
      phase_count_reg = 0;
    end
    else if(ramulator_phase != last_phase_reg)
    begin
      if(clock !== 1'b0 || clock_model.memory_cycle_reg != phase_count_reg + 1)
      begin
        $fatal(1, "Ramulator phase was not released after its falling-edge tick");
      end
      last_phase_reg = ramulator_phase;
      phase_count_reg = phase_count_reg + 1;
    end
  end

  always @(posedge clock)
  begin
    if(!reset && phase_count_reg != clock_model.memory_cycle_reg)
    begin
      $fatal(1, "Missing or duplicate Ramulator phase after a tick");
    end
  end
endmodule
