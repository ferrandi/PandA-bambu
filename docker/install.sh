#!/bin/bash

SCRIPT=$(readlink -f $0)
SCRIPTPATH=$(dirname $SCRIPT)
ALL_ARGUMENTS=$@

function error {
    local message=$1
    echo "#ERROR($SCRIPT): $message" 1>&2
}

PANDA_DIR=/opt/panda

INSTALL_ARGUMENTS="--prefix=$PANDA_DIR"

DOCKER_IMAGE_NAME="bambu_ubuntu_18_04"

EXECUTE="install,configure,compile,documentation"

usage()
{
    cat << EOF
Command: $0 $ALL_ARGUMENTS

usage: $0 [-i "<string>"] [-o <path>] [-v] [-h]

OPTIONS:
   -h                          Show this message
   -e <list>                   Execute (default = $EXECUTE) 
   -i "<text>"                 Configure parameters (default = "$INSTALL_ARGUMENTS") 
   -n <text>                   Docker image name (default = $DOCKER_IMAGE_NAME)
   -o <path>                   Installation path (default = $PANDA_DIR)
   -v                          Verbose

Currently accepted configure parameters are:
*******************************************

  --enable-flopoco: add floating point support by leveraging FloPoCo Library 
                   http://flopoco.gforge.inria.fr/
  --enable-xilinx: enable Xilinx RTL synthesis and simulation tools execution 
                   within the framework
  --enable-altera: enable Altera RTL synthesis tool execution within the framework
  --enable-lattice: enable Lattice RTL synthesis tool execution within the framework
  --enable-nanoxplore: enable BRAVE FPGA NanoXplore RTL synthesis tool execution within the framework
  --enable-modelsim: enable Modelsim-based simulation directly within the framework
  --with-mentor-license=<license-file/license-server>
  --with-nanoxplore-license=<license-file/license-server>
  --enable-icarus: enable Icarus-based simulation
                   http://iverilog.icarus.com/
  --enable-verilator: enable Verilator-based simulation
                   http://www.veripool.org/wiki/verilator
  --enable-debug: add debugging symbols to the binaries 
  --enable-opt: Compile the framework with GCC optimizations enabled
  --enable-release: Remove further tests not useful in a production environment

Note that the scripts for synthesis and simulation are generated even if the 
corresponding tools are not configured as described above. In this case, the 
synthesis/simulation has to be performed using the standard shell command line.
Since FloPoCo generates a VHDL-based description, co-simulation requires a
mixed-language simulator. Both Modelsim from Mentor and XSIM/ISIM from Xilinx
have such support.

The suggested configuration is
../configure --prefix=/opt/panda --enable-flopoco --enable-icarus --enable-verilator --enable-xilinx --enable-modelsim --enable-altera --enable-nanoxplore --enable-opt --enable-lattice --enable-release --with-mentor-license=<license-string> --with-nanoxplore-license=<license-string>

Options --enable-xilinx, --enable-modelsim, --enable-altera, --enable-lattice, --enable-nanoxplore, --with-mentor-license=<license-string>, and --with-nanoxplore-license=<license-string> require that third-party tools have been installed before PandA is configured. Details on where these third-party tools are searched are provided at the end of this file.

Example:

$0 -c $EXECUTE -i "--prefix=/opt/panda --enable-flopoco --enable-icarus"
 
EOF
}

while getopts ":he:i:n:o:v" OPTION
do
     case $OPTION in
         e)
             EXECUTE="$OPTARG"
             ;;
         i)
             INSTALL_ARGUMENTS="$OPTARG"
             ;;
         n)
             DOCKER_IMAGE_NAME="$OPTARG"
             ;;
         o)
             PANDA_DIR="$OPTARG"
             ;;
         h)
             usage
             exit 1
             ;;
         v)
             set -x
             ;;
         ?)
             error "Unknown option $OPTARG"
             usage
             exit 1
             ;;
         :)
             error "No argument value for option $OPTARG"
             usage
             exit 1
             ;;
     esac
done

function install_docker {
    $SCRIPTPATH/rootless
}

function build_docker_image {
    local DOCKER_IMAGE_EXISTS=$(docker images -q $DOCKER_IMAGE_NAME)

    if [ -z "$DOCKER_IMAGE_EXISTS" ]; then
        docker build -t $DOCKER_IMAGE_NAME -f $SCRIPTPATH/Dockerfile .
    fi
}

SPACE_SEPARATED_EXECUTE=$(echo "$EXECUTE" | tr ',' ' ')

for i in $SPACE_SEPARATED_EXECUTE; do
     case $i in
         install)
             install_docker
             source $SCRIPTPATH/start.sh || exit $?
             build_docker_image
             if [ ! -e $HOME/bin/bambu ]; then
                 ln -s $SCRIPTPATH/bambu $HOME/bin/bambu
             fi
             ;;
         configure)
             $SCRIPTPATH/run.sh -i $DOCKER_IMAGE_NAME -c "make -f Makefile.init; mkdir -p obj; cd obj; ../configure $INSTALL_ARGUMENTS"
             ;;
         compile)
             $SCRIPTPATH/run.sh -i $DOCKER_IMAGE_NAME -c "cd obj; make; make install"
             ;;
         documentation)
             $SCRIPTPATH/run.sh -i $DOCKER_IMAGE_NAME -c "cd obj; make documentation"
             ;;
         bash)
             $SCRIPTPATH/run.sh -i $DOCKER_IMAGE_NAME -c "bash"
             ;;
         *)
             error "Unknown execute command: $i"
             usage
             exit 1
             ;;
     esac
done








