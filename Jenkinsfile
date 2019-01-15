pipeline {
  agent any
  stages {
    stage('build') {
      steps {
        withEnv(['PATH+EXTRACCACHE=/usr/lib/ccache']) {
          sh 'make -f Makefile.init J=20 && mkdir build && cd build  && ../configure --prefix=$WORKSPACE/panda-bin --enable-flopoco --enable-xilinx --enable-modelsim --enable-icarus --enable-verilator --enable-altera --enable-lattice --enable-glpk --enable-release --enable-unordered --disable-asserts --enable-opt --with-gcc49=/distros/ubuntu_last_64/usr/bin/gcc-4.9 --with-gcc5=/distros/ubuntu_16-04_64/usr/bin/gcc-5 --with-mentor-license=2003@lmw-2d.polimi.it  && nice -n 17 make -j20  install  '
        }
      }
    }
    stage('tests') {
      parallel {
        stage('list based') {
          steps {
           sh 'cd $WORKSPACE/panda_regressions && nice -n 17 ./panda_regression_hls.sh -j24 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider '
           sh 'cd $WORKSPACE/panda_regressions && nice -n 17 ./panda_regression_hls.sh -j24 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider --junitdir="$WORKSPACE/test-reports" --returnfail --restart '
          }
        }
        stage('sdc scheduling') {
          steps {
           sh 'mkdir $WORKSPACE/panda_regressions/sdc_tests && cd $WORKSPACE/panda_regressions/sdc_tests && nice -n 17 $WORKSPACE/panda_regressions/panda_regression_hls.sh -j24 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider --junitdir="$WORKSPACE/test-reports" -t 120m -c="--speculative-sdc-scheduling" --returnfail '
          }
        }
        stage('VHDL') {
          steps {
           sh 'mkdir $WORKSPACE/panda_regressions/vhdl_tests && cd $WORKSPACE/panda_regressions/vhdl_tests && nice -n 17 $WORKSPACE/panda_regressions/panda_regression_hls.sh -j24 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider -c="-wH" --name="_VHDL" '
           sh 'cd $WORKSPACE/panda_regressions/vhdl_tests && nice -n 17 $WORKSPACE/panda_regressions/panda_regression_hls.sh -j24 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider -c="-wH" --name="_VHDL" --restart '
           sh 'cd $WORKSPACE/panda_regressions/vhdl_tests && nice -n 17 $WORKSPACE/panda_regressions/panda_regression_hls.sh -j24 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider --junitdir="$WORKSPACE/test-reports" -c="-wH" --name="_VHDL" --returnfail --restart '
          }
        }
      }
    }
    stage('Synthesis Step') {
      steps {
        sh 'cd $WORKSPACE/examples && nice -n 17 ./example.sh -j8 --bambu $WORKSPACE/panda-bin/bin/bambu --spider $WORKSPACE/panda-bin/bin/spider --junitdir="$WORKSPACE/test-reports" --perfpublisherdir="$WORKSPACE/pp-reports" '
      }
    }
    stage('Copy data Step') {
      steps {
        sh 'cd $WORKSPACE/examples && cp CHStone*tex CHStone '
        sh 'cd $WORKSPACE/examples && cp hls_study*tex hls_study '
        sh 'cd $WORKSPACE/examples && cp libm*tex libm '
        sh 'cd $WORKSPACE/examples && cp MachSuite*tex MachSuite '
        sh 'cd $WORKSPACE/examples && cp softfloat*tex softfloat '
        sh 'cd $WORKSPACE/examples && cp omp_simd*tex omp_simd '
      }
    }
    stage('Push results Step') {
      steps {
        withCredentials([usernamePassword(credentialsId: 'git-pass-credentials-ID', passwordVariable: 'GIT_PASSWORD', usernameVariable: 'GIT_USERNAME')]) {
          sh 'git commit --author="Jenkins CI <jenkins-ci@example.com>" -a -m "Updated synthesis results"  '
          sh 'git push origin'
        }
      }
    }

    stage('JunitCollect') {
      steps {
        junit allowEmptyResults: true, testResults: '$WORKSPACE/test-reports/*.xml'
      }
    }
  }
}
