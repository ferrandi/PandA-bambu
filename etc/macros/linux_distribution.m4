dnl
dnl Return a string containing the system linux distribution
dnl
AC_DEFUN([AC_LINUX_DISTRO],[

   DISTRO="unknown"
   if test -f /etc/centos-release; then
      LONG_DISTRO=$(cat /etc/centos-release)
      DISTRO_VERSION=$(echo "$LONG_DISTRO" | awk '{print $[3]}')
      MAJOR=$(echo $DISTRO_VERSION | awk -F. '{print $[1]}')
      MINOR=$(echo $DISTRO_VERSION | awk -F. '{print $[2]}')
      DISTRO="centos_${MAJOR}_${MINOR}"
   fi
   if test -f /etc/oracle-release; then
      LONG_DISTRO=$(cat /etc/oracle-release)
      DISTRO_VERSION=$(echo "$LONG_DISTRO" | awk '{print $[5]}')
      MAJOR=$(echo $DISTRO_VERSION | awk -F. '{print $[1]}')
      MINOR=$(echo $DISTRO_VERSION | awk -F. '{print $[2]}')
      DISTRO="oracle_${MAJOR}_${MINOR}"
   fi
   if test -f /etc/redhat-release; then
      LONG_DISTRO=$(cat /etc/redhat-release)
      DISTRO_VERSION=$(echo "$LONG_DISTRO" | awk '{print $[7]}')
      MAJOR=$(echo $DISTRO_VERSION | awk -F. '{print $[1]}')
      MINOR=$(echo $DISTRO_VERSION | awk -F. '{print $[2]}')
      DISTRO="rhel_${MAJOR}_${MINOR}"
   fi
   if test -f /etc/lsb-release; then
      DISTRO_NAME=$(cat /etc/lsb-release | grep DISTIB_ID | awk -F= '{print $[2]}')
      DISTRO_VERSION=$(cat /etc/lsb-release | grep DISTRIB_RELEASE | awk -F= '{print $[2]}')
      DISTRO="ubuntu_${DISTRO_VERSION}"
   fi
   if test -f /etc/issue; then
      if cat /etc/issue | grep -q "Arch"; then
         DISTRO="arch"
      fi
   fi
   echo "checking linux distribution... $DISTRO"
])
