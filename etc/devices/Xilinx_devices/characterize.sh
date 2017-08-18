#! /bin/bash

for devices in xc7z020-1clg484; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../Zynq.xml --target-datafile="../$devices-seed.xml" -v2 >& "$devices.log" &
   cd ..
done


for devices in xc5vlx50-3ff1153 xc5vlx110t-1ff1136 xc5vlx330t-2ff1738; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../Virtex-5.xml --target-datafile="../$devices-seed.xml" -v2 >& "$devices.log" &
   cd ..
done

for devices in xc6vlx240t-1ff1156; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../Virtex-6.xml --target-datafile="../$devices-seed.xml" -v2 >& "$devices.log" &
   cd ..
done

for devices in xc7vx330t-1ffg1157; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../Virtex-7.xml --target-datafile="../$devices-seed.xml" -v2 >& "$devices.log" &
   cd ..
done


