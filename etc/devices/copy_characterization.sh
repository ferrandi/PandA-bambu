#!/bin/bash
abs_script=$(readlink -e $0)
root_dir=$(dirname $abs_script)
cp 5CSEMA5F31C6/output/5CSEMA5F31C6.xml $root_dir/Altera_devices
cp 5SGXEA7N2F45C1/output/5SGXEA7N2F45C1.xml $root_dir/Altera_devices
cp EP2C70F896C6/output/EP2C70F896C6.xml $root_dir/Altera_devices
cp EP2C70F896C6-R/output/EP2C70F896C6-R.xml $root_dir/Altera_devices
cp EP4SGX530KH40C2/output/EP4SGX530KH40C2.xml $root_dir/Altera_devices
cp LFE335EA8FN484C/output/LFE335EA8FN484C.xml $root_dir/Lattice_devices
cp LFE5UM85F8BG756C/output/LFE5UM85F8BG756C.xml $root_dir/Lattice_devices
cp LFE5U85F8BG756C/output/LFE5U85F8BG756C.xml $root_dir/Lattice_devices
cp xc4vlx100-10ff1513/output/xc4vlx100-10ff1513.xml $root_dir/Xilinx_devices
cp xc5vlx110t-1ff1136/output/xc5vlx110t-1ff1136.xml $root_dir/Xilinx_devices
cp xc5vlx330t-2ff1738/output/xc5vlx330t-2ff1738.xml $root_dir/Xilinx_devices
cp xc5vlx50-3ff1153/output/xc5vlx50-3ff1153.xml $root_dir/Xilinx_devices
cp xc6vlx240t-1ff1156/output/xc6vlx240t-1ff1156.xml $root_dir/Xilinx_devices
cp xc7a100t-1csg324-VVD/output/xc7a100t-1csg324-VVD.xml $root_dir/Xilinx_devices
cp xc7vx330t-1ffg1157/output/xc7vx330t-1ffg1157.xml $root_dir/Xilinx_devices
cp xc7vx485t-2ffg1761-VVD/output/xc7vx485t-2ffg1761-VVD.xml $root_dir/Xilinx_devices
cp xc7vx690t-3ffg1930-VVD/output/xc7vx690t-3ffg1930-VVD.xml $root_dir/Xilinx_devices
cp xc7z020-1clg484/output/xc7z020-1clg484.xml $root_dir/Xilinx_devices
cp xc7z020-1clg484-VVD/output/xc7z020-1clg484-VVD.xml $root_dir/Xilinx_devices
cp xc7z045-2ffg900-VVD/output/xc7z045-2ffg900-VVD.xml $root_dir/Xilinx_devices
cp xc7z020-1clg484-YOSYS-VVD/output/xc7z020-1clg484-YOSYS-VVD.xml $root_dir/Xilinx_devices
cp nx1h35S/output/nx1h35S.xml $root_dir/NanoXplore_devices
cp nx1h140tsp/output/nx1h140tsp.xml $root_dir/NanoXplore_devices
cp nx2h540tsc/output/nx2h540tsc.xml $root_dir/NanoXplore_devices
