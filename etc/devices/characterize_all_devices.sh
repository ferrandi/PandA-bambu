#!/bin/bash
abs_script=$(readlink -e $0)
root_dir=$(dirname $abs_script)
device_dir=$root_dir/../devices
for file in $(find $device_dir -name *-seed.xml*); do
   filename=$(basename $file);
   device=${filename::-9}
   echo $device
   if [ "$device" = "Nangate_device" ]; then
      continue
   fi
   if [ ! -d $device ]; then
      mkdir $device
   fi
   cd $device
   $root_dir/characterize_device.sh --devices=$device "$@"
   return_value=$?
   if test $return_value != 0; then
      exit $return_value
   fi
   cd ..
done


