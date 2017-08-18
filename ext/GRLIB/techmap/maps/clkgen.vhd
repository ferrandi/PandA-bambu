------------------------------------------------------------------------------
--  This file is a part of the GRLIB VHDL IP LIBRARY
--  Copyright (C) 2003 - 2008, Gaisler Research
--  Copyright (C) 2008 - 2010, Aeroflex Gaisler
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
-----------------------------------------------------------------------------
-- Entity: 	clkgen
-- File:	clkgen.vhd
-- Author:	Jiri Gaisler Gaisler Research
-- Description:	Clock generator with tech selection
------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
library techmap;
use techmap.gencomp.all;
use techmap.allclkgen.all;

entity clkgen is
  generic (
    tech     : integer := DEFFABTECH;
    clk_mul  : integer := 1;
    clk_div  : integer := 1;
    sdramen  : integer := 0;
    noclkfb  : integer := 1;
    pcien    : integer := 0;
    pcidll   : integer := 0;
    pcisysclk: integer := 0;
    freq     : integer := 25000;	-- clock frequency in KHz
    clk2xen  : integer := 0;
    clksel   : integer := 0;             -- enable clock select
    clk_odiv : integer := 0);             -- Proasic3 output divider
  port (
    clkin   : in  std_logic;
    pciclkin: in  std_logic;
    clk     : out std_logic;			-- main clock
    clkn    : out std_logic;			-- inverted main clock
    clk2x   : out std_logic;			-- 2x clock
    sdclk   : out std_logic;			-- SDRAM clock
    pciclk  : out std_logic;			-- PCI clock
    cgi     : in clkgen_in_type;
    cgo     : out clkgen_out_type;
    clk4x   : out std_logic;			-- 4x clock
    clk1xu  : out std_logic;			-- unscaled 1X clock
    clk2xu  : out std_logic);			-- unscaled 2X clock
end;

architecture struct of clkgen is
signal intclk, sdintclk : std_ulogic;
signal lock : std_ulogic;
begin
  gen : if (is_unisim(tech) /= 1) and 
	(tech /= altera) and (tech /= stratix1) and (tech /= apa3) and
	(tech /= stratix2) and (tech /= stratix3) and
	(tech /= cyclone3) and (tech /= axcel) and
	(tech /= axdsp) and (tech /= proasic) and (tech /= rhlib18t) and
	(tech /= rhumc) and (tech /= easic90) generate
    sdintclk <= pciclkin when (PCISYSCLK = 1 and PCIEN /= 0) else clkin;
    sdclk <= sdintclk; intclk <= sdintclk
-- pragma translate_off
	after 1 ns	-- create 1 ns skew between clk and sdclk
-- pragma translate_on
    ;
    clk1xu <= intclk; pciclk <= pciclkin; clk <= intclk; clkn <= not intclk;
    cgo.clklock <= '1'; cgo.pcilock <= '1'; clk2x <= '0';
  end generate;
  xc2v : if (tech = virtex2) or (tech = virtex4) generate
    v : clkgen_virtex2
    generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen, clksel)
    port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo, clk1xu, clk2xu);
  end generate;
  xc5l : if (tech = virtex5) or (tech = virtex6) generate
    v : clkgen_virtex5
    generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen, clksel)
    port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo, clk1xu, clk2xu);
  end generate;
  xc3s : if (tech = spartan3) or (tech = spartan3e) or (tech = spartan6) generate
    v : clkgen_spartan3
    generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen, clksel)
    port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo, clk1xu, clk2xu);
  end generate;
  alt : if (tech = altera) or (tech = stratix1) generate
   v : clkgen_altera_mf
   generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen)
   port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo);
  end generate;
  strat2 : if (tech = stratix2) generate
   v : clkgen_stratixii
   generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen)
   port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo);
  end generate;
  cyc3 : if (tech = cyclone3)  generate
   v : clkgen_cycloneiii
   generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen)
   port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo);
  end generate;
  stra3 : if (tech = stratix3) generate
   v : clkgen_stratixiii
   generic map (clk_mul, clk_div, sdramen, noclkfb, pcien, pcidll, pcisysclk, freq, clk2xen)
   port map (clkin, pciclkin, clk, clkn, clk2x, sdclk, pciclk, cgi, cgo);
  end generate;
  act : if (tech = axcel) or (tech = axdsp) or (tech = proasic) generate
    intclk <= pciclkin when (PCISYSCLK = 1 and PCIEN /= 0) else clkin;
    sdclk <= '0'; pciclk <= pciclkin; clk <= intclk; clkn <= '0';
    cgo.clklock <= '1'; cgo.pcilock <= '1'; clk2x <= '0';
  end generate;
  lib18t : if (tech = rhlib18t) generate
    v : clkgen_rh_lib18t
    generic map (clk_mul, clk_div)
    port map (cgi.pllrst, intclk, clk, sdclk, clk2x, clk4x);
    intclk <= pciclkin when (PCISYSCLK = 1 and PCIEN /= 0) else clkin;
    pciclk <= pciclkin; clkn <= '0';
    cgo.clklock <= '1'; cgo.pcilock <= '1';
  end generate;
  ap3 : if tech = apa3 generate
    v : clkgen_proasic3
    generic map (clk_mul, clk_div, clk_odiv, pcien, pcisysclk, freq)
    port map (clkin, pciclkin, clk, sdclk, pciclk, cgi, cgo);
  end generate;
  dr : if (tech = rhumc)  generate
   v : clkgen_dare
   port map (clkin, clk, clk2x, sdclk, pciclk,
	cgi, cgo, clk4x, clk1xu, clk2xu);
  end generate;

  nextreme90 : if tech = easic90 generate
    pll0 : clkgen_easic90
      generic map (
        clk_mul   => clk_mul,
        clk_div   => clk_div,
        freq      => freq,
        pcisysclk => pcisysclk,
        pcien     => pcien)
      port map (clkin, pciclkin, clk,  clk2x, clk4x, clkn, lock);
    cgo.clklock <= lock;
    cgo.pcilock <= lock;
  end generate;
end;
