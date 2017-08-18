#! /bin/bash

for device in xc7z020-1clg484-VVD; do
   mkdir -p "$device-DIR"
   cd "$device-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$device.xml" --target-scriptfile=../Zynq-VVD.xml --target-datafile="../$device-seed.xml" -v4 >& "$device.log" &
   cd ..
done

for device in xc7vx485t-2ffg1761-VVD xc7vx690t-3ffg1930-VVD; do
   mkdir -p "$device-DIR"
   cd "$device-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$device.xml" --target-scriptfile=../Virtex-7-VVD.xml --target-datafile="../$device-seed.xml" -v4 >& "$device.log" &
   cd ..
done

for device in xc7a100t-1csg324-VVD; do
   mkdir -p "$device-DIR"
   cd "$device-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$device.xml" --target-scriptfile=../Artix-7-VVD.xml --target-datafile="../$device-seed.xml" -v4 >& "$device.log" &
   cd ..
done

