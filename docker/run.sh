#!/bin/bash

SCRIPT=$(readlink -f $0)
SCRIPT_DIR=$(dirname $SCRIPT)

ALL_ARGUMENTS=$@

PANDA_DIR=/opt/panda

MOUNT_DIR="-v /opt:/opt:z -v $HOME:$HOME:z"

WORKING_DIR="-w $(pwd)"

DOCKER_IMAGE_NAME="bambu_ubuntu_18_04"

usage()
{
    cat << EOF
Command: $0 $ALL_ARGUMENTS

usage: $0 -c "<string>" [-v "<string>"] [-o <path>] [-h]

OPTIONS:
   -h                          Show this message
   -c "<string>"               Execute command  
   -i <string>                 Docker image name
   -v <patg>                   Mount directory
   -o <path>                   Installation path (default = $PANDA_DIR)
   -v                          Verbose

Example:

$0 -c "bash echo Hello" 
 
EOF
}

while getopts ":hc:i:o:v:" OPTION
do
     case $OPTION in
         c)
             COMMAND="$OPTARG"
             ;;
         i)
             DOCKER_IMAGE_NAME="$OPTARG"
             ;;
         v)
             MOINT_DIR="$MOUNT_DIR $OPTARG:$OPTARG:z"
             ;;
         o)
             PANDA_DIR="$OPTARG"
             ;;
         h)
             usage
             exit 1
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

source $SCRIPT_DIR/start.sh

if [ ! -w $PANDA_DIR ]; then
    error "$PANDA_DIR is not writable"
    exit 1
fi
    
docker run -it -e PATH --rm $WORKING_DIR $MOUNT_DIR $MOUNT_SCRIPT_DIR $DOCKER_IMAGE_NAME bash -c "$COMMAND"
