#!/bin/bash
for device in xc7z020-1clg484 xc5vlx50-3ff1153 xc5vlx110t-1ff1136 xc5vlx330t-2ff1738 xc6vlx240t-1ff1156 xc7vx330t-1ffg1157 xc7z020-1clg484-VVD xc7vx485t-2ffg1761-VVD xc7vx690t-3ffg1930-VVD xc7a100t-1csg324-VVD xc4vlx100-10ff1513 LFE335EA8FN484C 5CSEMA5F31C6 5SGXEA7N2F45C1 EP4SGX530KH40C2 EP2C70F896C6 EP2C70F896C6-R nx1h35S nx1h140tsp; do
$(dirname $0)/CHStone_memarch_core2.sh -c="--device-name=$device" -o output_CHStoneMemArch"$device" --table=CHStoneMemArch"$device".tex --name="_$device" $@
return_value=$?
cat */bambu_failed_output > bambu_failed_output
if test $return_value != 0; then
   exit $return_value
fi
done

