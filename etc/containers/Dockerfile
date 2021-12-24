ARG DISTRO=focal
FROM ubuntu:${DISTRO}
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
   build-essential ca-certificates clang curl doxygen gfortran git graphviz libbdd-dev lsb-release pkg-config python3-distutils python3-defusedxml texlive-latex-recommended texlive-latex-extra time xz-utils \
   iverilog verilator locales \
   && rm -rf /var/lib/apt/lists/* \
   && locale-gen en_US.UTF-8
ARG GCC_VER=4.9
ARG CLANG_VER=4.0
SHELL [ "/bin/bash", "-c" ]
RUN if [ "$(lsb_release -rs | cut -f1 -d'.')" -gt "16" ]; then echo "deb http://dk.archive.ubuntu.com/ubuntu/ xenial main universe" >> /etc/apt/sources.list; fi \
   && if [ "$(lsb_release -rs | cut -f1 -d'.')" -gt "18" ]; then echo "deb http://dk.archive.ubuntu.com/ubuntu/ bionic main universe" >> /etc/apt/sources.list; fi \
   && if [ "$(lsb_release -rs | cut -f1 -d'.')" -gt "20" ]; then echo "deb http://dk.archive.ubuntu.com/ubuntu/ focal main universe" >> /etc/apt/sources.list; fi \
   && dpkg --add-architecture i386 \
   && apt-get update && apt-get install -y --no-install-recommends \
   libc6:i386 libncurses5:i386 libstdc++6:i386 libxft2 libxft2:i386 lib32ncurses5 libxext6 libxext6:i386 linux-libc-dev:i386 \
   libtinfo5 \
   gcc-${GCC_VER} gcc-${GCC_VER}-multilib g++-${GCC_VER} g++-${GCC_VER}-multilib gfortran-${GCC_VER} gfortran-${GCC_VER}-multilib \
   clang-${CLANG_VER} libclang-${CLANG_VER}-dev \
   && rm -rf /var/lib/apt/lists/* \
   && adduser --disabled-password --gecos '' --gid 0 jenkins \
   && mkdir -p /opt/panda \
   && chown -R jenkins /opt/panda
USER jenkins
WORKDIR /home/jenkins

CMD [ "/bin/bash" ]
