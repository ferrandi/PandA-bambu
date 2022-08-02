echo $PWD
mkdir $4
mkdir "$4/$1x$2x$3"
./cachegen -a -h $1 $2 $3 "$4/$1x$2x$3/ass1C"
./cachegen -a -eh $1 $2 $3 "$4/$1x$2x$3/ass2C"
./cachegen -d -h $1 $2 $3 "$4/$1x$2x$3/direct1C"
./cachegen -d -eh $1 $2 $3 "$4/$1x$2x$3/direct2C"
./cachegen -t -h $1 $2 $3 "$4/$1x$2x$3/twoway1C1C"
./cachegen -t -eh $1 $2 $3 "$4/$1x$2x$3/twoway2C1C"
./cachegen -t -wh $1 $2 $3 "$4/$1x$2x$3/twoway1C2C"
./cachegen -t -ewh $1 $2 $3 "$4/$1x$2x$3/twoway2C2C"
