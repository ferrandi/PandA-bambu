# PandA Bambu HLS Framework
![](style/img/panda.png.in)

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

----

The primary objective of the PandA project is to develop a usable framework
that will enable the research of new ideas in the HW-SW Co-Design field.

The PandA framework includes methodologies supporting the research on high-level 
synthesis of hardware accelerators, on parallelism extraction for embedded systems, 
on hardware/software partitioning and mapping, on metrics for performance estimation 
of embedded software applications and on dynamic reconfigurable devices.

PandA is free software, free in the sense that it respects the user’s freedom, 
released under the GNU General Public License version 3 and being 
developed at Politecnico di Milano (Italy).

The source files currently distributed mainly cover the high-level synthesis 
of C/C++ based descriptions. In particular, the tool Bambu provides a research environment to experiment with new ideas across HLS, high-level verification and debugging, FPGA/ASIC design, design flow space exploration, and parallel hardware accelerator design.
Bambu accepts as input standard C/C++ specifications and compiler intermediate representations coming from the well-known Clang/LLVM and GCC compilers.
The broad spectrum and flexibility of input formats allow the electronic design automation research community to explore and integrate new transformations and optimizations.

If you use Bambu in your research, please cite:
```
@INPROCEEDINGS{ferrandi2021bambu,
  author={Ferrandi, Fabrizio and Castellana, Vito Giovanni 
          and Curzel, Serena and Fezzardi, Pietro and Fiorito, Michele 
          and Lattuada, Marco and Minutoli, Marco and Pilato, Christian 
          and Tumeo, Antonino},
  booktitle={2021 58th ACM/IEEE Design Automation Conference (DAC)}, 
  title={Invited: Bambu: an Open-Source Research Framework for the 
         High-Level Synthesis of Complex Applications}, 
  year={2021},
  pages={1327-1330},
  abstract = {This paper presents the open-source high-level synthesis (HLS) research 
              framework Bambu. Bambu provides a research environment to experiment with 
              new ideas across HLS, high-level verification and debugging, FPGA/ASIC design,
              design flow space exploration, and parallel hardware accelerator design. The 
              tool accepts as input standard C/C++ specifications and compiler intermediate 
              representations (IRs) coming from the well-known Clang/LLVM and GCC compilers. 
              The broad spectrum and flexibility of input formats allow the electronic 
              design automation (EDA) research community to explore and integrate new 
              transformations and optimizations. The easily extendable modular framework 
              already includes many optimizations and HLS benchmarks used to evaluate 
              the QoR of the tool against existing approaches [1]. The integration with 
              synthesis and verification backends (commercial and open-source) allows 
              researchers to quickly test any new finding and easily obtain performance 
              and resource usage metrics for a given application. Different FPGA devices 
              are supported from several different vendors: AMD/Xilinx, Intel/Altera, 
              Lattice Semiconductor, and NanoXplore. Finally, integration with the OpenRoad 
              open-source end-to-end silicon compiler perfectly fits with the recent push 
              towards open-source EDA.},
  publisher={{IEEE}},
  doi={10.1109/DAC18074.2021.9586110},
  ISSN={0738-100X},
  month={Dec},
  pdf={https://re.public.polimi.it/retrieve/668507/dac21_bambu.pdf}
}
```
<br>

# Installation instructions
Installation instructions for many different operation systems are available at 
https://panda.dei.polimi.it/?page_id=88 or in the [*INSTALL*](INSTALL) file in this repository.

You may also download pre-compiled AppImage distributions at https://release.bambuhls.eu/ or you can generate them yourself following the instructions at https://docs.bambuhls.eu/da/df6/install_docker.html.

Dockerfiles are also available under *etc/containers* and pre-built docker images can be downloaded at [Docker Hub](https://hub.docker.com/u/bambuhls).

A Google Colab notebook with many examples to play with Bambu is available. [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/ferrandi/PandA-bambu/blob/main/documentation/tutorial_date_2022/bambu.ipynb)


<br>

# Contacts
Issues, patches, and pull requests are welcome at https://github.com/ferrandi/PandA-bambu.<br>
For further information send an e-mail to panda-info@polimi.it, visit [PandA website](https://panda.dei.polimi.it/) or the Google [group page](https://groups.google.com/forum/#!forum/panda-project-discussions-questions).

<br>

# Acknowledgements
Bambu has been supported throughout its history by the following projects.

European Union projects:
 - Grant agreement ID 004452 ICODES – Interface and Communication based Design of Embedded Systems
 - Grant agreement ID 035143 hArtes – Holistic Approach to Reconfigurable Real-Time Embedded Systems
 - Grant agreement ID 248538 Synaptic – SYNthesis using Advanced Process Technology Integrated in regular Cells, IPs, architectures, and design platforms
 - Grant agreement ID 287804 Faster – Facilitating Analysis and Synthesis Technologies for Effective Reconfiguration
 - Grant agreement ID 101004203 HERMES – qualification of High pErformance pRogrammable Microprocessor and dEvelopment of Software ecosystem
 - Grant agreement ID 957269 EVEREST – dEsign enVironmEnt foR Extreme-Scale big data analytics on heterogeneous platforms

European Space Agency contracts:
 - ESA/ESTEC/Contract N. 4000100797 – Development of methodologies and tools for predictable, real-time LEON-DSP-based embedded systems.
 - ESA/ESTEC/Contract No. 22167/09/NL/JK. Cache Optimization for LEON Analysis (COLA).
 - ESA/ESTEC/Contract Call-Off Order 4 “Multicore and Schedulability Analysis” for TASTE project.
 - ESA/ESTEC/Contract No. 4000121154/17/NL/LF Compact Reconfigurable Avionics Model-Based Avionic Design (CORA-MBAD)