
VIVADO_ENV_SETTINGS=`find -L ${BAMBU_HLS_BACKEND_PATH//:/ } -type f -path "*/Vivado/*/settings64.sh" 2> /dev/null | head -n1`
if [ -z "${VIVADO_ENV_SETTINGS}" ]; then
   echo "Vivado tool not found"
   exit -1
fi
echo "Vivado environment settings: ${VIVADO_ENV_SETTINGS}"
source "${VIVADO_ENV_SETTINGS}"
