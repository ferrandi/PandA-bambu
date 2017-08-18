#! /bin/bash

for devices in EP2C70F896C6; do
   mkdir -p "$devices-DIR-DSP"
   cd "$devices-DIR-DSP"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices-DSP.xml" --target-scriptfile=../CycloneII.xml --target-datafile="../$devices-seed.xml" -v4 >& "$devices.log" &
   cd ..
done

for devices in EP2C70F896C6; do
   mkdir -p "$devices-DIR-R"
   cd "$devices-DIR-R"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices-R.xml" --target-scriptfile=../CycloneII-R.xml --target-datafile="../$devices-R-seed.xml" -v4 >& "$devices.log" &
   cd ..
done

for devices in 5CSEMA5F31C6; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../CycloneV.xml --target-datafile="../$devices-seed.xml" -v4 >& "$devices.log" &
   cd ..
done

for devices in 5SGXEA7N2F45C2; do
   mkdir -p "$devices-DIR"
   cd "$devices-DIR"
   /opt/panda/bin/eucalyptus --estimate-library="../$devices.xml" --target-scriptfile=../StratixV.xml --target-datafile="../$devices-seed.xml" -v4 >& "$devices.log" &
   cd ..
done

