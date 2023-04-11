#! /bin/bash

for devices in nx1h35S nx1h140tsp nx2h540tsc; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../NG.xml --target-datafile="../$devices-seed.xml" -v2 >& "$devices.log" &
   cd ..
done


