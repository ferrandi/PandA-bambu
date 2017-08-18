#! /bin/bash

for devices in Nangate_device; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --target-device-type=ASIC --estimate-library="../$devices.xml" --target-scriptfile=../Nangate.xml --target-datafile="../$devices-seed.xml" -v2 >& "$devices.log" &
   cd ..
done

