***************************************
CHStone v1.11 (tar.gz) (Feb/04/2015)
***************************************

***************************************
NO WARRANTY
***************************************
There is no warranty for the programs and documents of this subdirectory. The entire risk as to the quality and performance of the program, fitness to a particular purpose is with you.

***************************************
Copyright Notice
***************************************
Each program in the CHStone suite is owned by the copyright holder of the program. You must follow the copyright of each benchmark program.

***************************************
CHStone
***************************************
A Suite of Benchmark Programs for C-based High-Level Synthesis 

Introduction

The CHStone benchmark suite has been developed for C-based high-level synthesis (HLS). The CHStone benchmark suite selected programs of various application domains, some of which originally belong to other benchmark suites. CHStone has several key features described as follows:

    CHStone consists of 12 programs which are selected from various application domains such as arithmetic, media processing, security and microprocessor.
    The programs in CHStone are relatively large compared with the ones which have been widely used in the past literature on HLS.
    The CHStone benchmark programs are written in the standard C language, and can be compiled and executed on a host computer.
    In our experimental environment, we used GCC 3.4.4 on Windows XP.
    All the programs in CHStone have been confirmed to be synthesizable by a state-of-the-art HLS tool.
    In our experimental environment, we used eXCite from YXI.
    CHStone is very easy to use since test vectors are self-contained and no external library is necessary.

List of Benchmark Programs

The CHStone suite includes the following programs. Source-level characteristics and synthesis results can be found in our paper.

Program 	Design Description 	Source
DFADD 	Double-precision floating-point addition 	SoftFloat [1]
DFMUL 	Double-precision floating-point multiplication 	SoftFloat [1]
DFDIV 	Double-precision floating-point division 	SoftFloat [1]
DFSIN 	Sine function for double-precision floating-point numbers 	CHStone group, SoftFloat [1]
MIPS 	Simplified MIPS processor 	CHStone group
ADPCM 	Adaptive differential pulse code modulation decoder and encoder 	SNU [2]
GSM 	Linear predictive coding analysis of global system for mobile communications 	MediaBench [3]
JPEG 	JPEG image decompression 	The Portable Video Research Group [4],
CHStone group
MOTION 	Motion vector decoding of the MPEG-2 	MediaBench [3]
AES 	Advanced encryption standard 	AILab [5]
BLOWFISH 	Data encryption standard 	MiBench [6]
SHA 	Secure hash algorithm 	MiBench [6]

[1] SoftFloat, http://www.jhauser.us/arithmetic/SoftFloat.html.
[2] SNU Real-time Benchmarks, http://archi.snu.ac.kr/realtime/benchmark/.
[3] C. Lee, M. Potkonjak, and W. H. Mangione-Smith, "MediaBench: A tool for evaluating and synthesizing multimedia and communicatons systems," MICRO, 1997.
[4] A. C. Hung, "PVRG-JPEG CODEC 1.1," Technical Report, Stanford University, 1993.
[5] AILab, http://www-ailab.elcom.nitech.ac.jp/.
[6] M. R. Guthaus, J. S. Ringenberg, and D. Ernst, "MiBench: A free, commercially representative embedded benchmark suite," WWC, 2001.
Publication

Please cite the following paper when you publish a paper where CHStone is used.

    Yuko Hara, Hiroyuki Tomiyama, Shinya Honda and Hiroaki Takada,
    "Proposal and Quantitative Analysis of the CHStone Benchmark Program Suite for Practical C-based High-level Synthesis",
    Journal of Information Processing, Vol. 17, pp.242-254, (2009).
    PDF is freely available online from Journal of Information Processing

