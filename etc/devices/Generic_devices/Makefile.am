#this is file ASIC/Makefile.am
DOX_TAGFILES =

EXTRA_DIST = Generic-yosysOpenROAD.xml PandA2OpenROAD.md nangate45-seed.xml nangate45.xml \
             nangate45/FreePDK45.lyp \
             nangate45/README.md \
             nangate45/FreePDK45.lyt \
             nangate45/OpenCellLibraryLicenseSi2.txt \
             nangate45/tracks.info \
             nangate45/template_pga.cfg \
             nangate45/lib/NangateOpenCellLibrary_typical.lib \
             nangate45/layermap.txt \
             nangate45/gds/NangateOpenCellLibrary.gds \
             nangate45/tapcell.tcl \
             nangate45/IP_global.cfg \
             nangate45/cells_latch.v \
             nangate45/config.mk \
             nangate45/grid_strategy-M1-M2-M7.cfg \
             nangate45/blackbox_map.tcl \
             nangate45/grid_strategy-M1-M4-M7.cfg \
             nangate45/magic.tech \
             nangate45/tritonCTS/sol_list.txt \
             nangate45/tritonCTS/lut.txt \
             nangate45/lef/NangateOpenCellLibrary.macro.lef \
             nangate45/lef/NangateOpenCellLibrary.macro.mod.lef \
             nangate45/lef/NangateOpenCellLibrary.tech.lef \
             nangate45/NangateOpenCellLibrary.blackbox.v \
             nangate45/pdn.cfg \
             nangate45/cells_clkgate.v \
             scripts/modifyLefSpacing.py \
             scripts/padLefMacro.py \
             scripts/def2gds.py \
             scripts/ICeWall.tcl \
             scripts/mergeLef.py \
             scripts/markDontUse.py \
             scripts/mergeLib.pl \
             scripts/fixIoPins.py


scripts_dir = $(pkgdatadir)/scripts
nangate45_dir = $(pkgdatadir)/nangate45
nangate45_gds_dir = $(nangate45_dir)/gds
nangate45_lef_dir = $(nangate45_dir)/lef
nangate45_lib_dir = $(nangate45_dir)/lib
nangate45_tritonCTS_dir = $(nangate45_dir)/tritonCTS

scripts__SCRIPTS = \
 scripts/ICeWall.tcl \
 scripts/def2gds.py\
 scripts/markDontUse.py \
 scripts/mergeLef.py \
 scripts/mergeLib.pl \
 scripts/modifyLefSpacing.py \
 scripts/padLefMacro.py \
 scripts/fixIoPins.py

nangate45__DATA = \
 nangate45/blackbox_map.tcl \
 nangate45/config.mk \
 nangate45/FreePDK45.lyt \
 nangate45/grid_strategy-M1-M2-M7.cfg \
 nangate45/IP_global.cfg \
 nangate45/magic.tech \
 nangate45/OpenCellLibraryLicenseSi2.txt \
 nangate45/README.md \
 nangate45/template_pga.cfg \
 nangate45/cells_latch.v \
 nangate45/FreePDK45.lyp \
 nangate45/grid_strategy-M1-M4-M7.cfg \
 nangate45/layermap.txt \
 nangate45/NangateOpenCellLibrary.blackbox.v \
 nangate45/pdn.cfg \
 nangate45/tapcell.tcl \
 nangate45/tracks.info \
 nangate45/cells_clkgate.v

nangate45_gds__DATA = \
 nangate45/gds/NangateOpenCellLibrary.gds 

nangate45_lef__DATA = \
 nangate45/lef/NangateOpenCellLibrary.macro.lef \
 nangate45/lef/NangateOpenCellLibrary.macro.mod.lef \
 nangate45/lef/NangateOpenCellLibrary.tech.lef

nangate45_lib__DATA = \
 nangate45/lib/NangateOpenCellLibrary_typical.lib

nangate45_tritonCTS__DATA = \
 nangate45/tritonCTS/lut.txt \
 nangate45/tritonCTS/sol_list.txt

