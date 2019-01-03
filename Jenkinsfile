pipeline {
  agent any
  stages {
    stage('build') {
      steps {
        withEnv(['PATH+EXTRACCACHE=/usr/lib/ccache']) {
          sh 'echo $PATH'
          sh 'make -f Makefile.init J=20 && mkdir build && cd build  && ../configure --prefix=$WORKSPACE/panda-bin --enable-flopoco --enable-xilinx --enable-modelsim --enable-icarus --enable-verilator --enable-altera --enable-lattice --enable-glpk --enable-release --enable-unordered --disable-asserts --enable-opt --with-gcc49=/distros/ubuntu_last_64/usr/bin/gcc-4.9 --with-gcc5=/distros/ubuntu_16-04_64/usr/bin/gcc-5 --with-mentor-license=2003@lmw-2d.polimi.it  && nice -n 14 make -j20  install  '
        }
      }
    }
  }
}
