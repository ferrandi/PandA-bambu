# Read Me for PandA framework

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ff8e7dcac46d4117a3adf12b7fb8e50e)](https://www.codacy.com/app/fabrizio.ferrandi/PandA-bambu?utm_source=github.com&utm_medium=referral&utm_content=ferrandi/PandA-bambu&utm_campaign=badger)
[![Build Status](https://travis-ci.org/ferrandi/PandA-bambu.svg)](https://travis-ci.org/ferrandi/PandA-bambu)
[![CircleCI](https://circleci.com/gh/ferrandi/PandA-bambu.svg?style=svg)](https://circleci.com/gh/ferrandi/PandA-bambu)
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
of C based descriptions. In particular, the tool Bambu provides a research environment to experiment with new ideas across HLS, high-level verification and debugging, FPGA/ASIC design, design flow space exploration, and parallel hardware accelerator design.
Bambu accepts as input standard C/C++ specifications and compiler intermediate representations coming from the well-known Clang/LLVM and GCC compilers.
The broad spectrum and flexibility of input formats allow the electronic design automation research community to explore and integrate new transformations and optimizations.

If you use Bambu in your research, please cite:
```
@INPROCEEDINGS{ferrandi2021bambu,
  author={Ferrandi, Fabrizio and Castellana, Vito Giovanni and Curzel, Serena and Fezzardi, Pietro and Fiorito, Michele and Lattuada, Marco and Minutoli, Marco and Pilato, Christian and Tumeo, Antonino},
  booktitle={2021 58th ACM/IEEE Design Automation Conference (DAC)}, 
  title={Invited: Bambu: an Open-Source Research Framework for the High-Level Synthesis of Complex Applications}, 
  year={2021},
  volume={},
  number={},
  pages={1327-1330},
  abstract = {This paper presents the open-source high-level synthesis (HLS) research framework Bambu. Bambu provides a research environment to experiment with new ideas across HLS, high-level verification and debugging, FPGA/ASIC design, design flow space exploration, and parallel hardware accelerator design. The tool accepts as input standard C/C++ specifications and compiler intermediate representations (IRs) coming from the well-known Clang/LLVM and GCC compilers. The broad spectrum and flexibility of input formats allow the electronic design automation (EDA) research community to explore and integrate new transformations and optimizations. The easily extendable modular framework already includes many optimizations and HLS benchmarks used to evaluate the QoR of the tool against existing approaches [1]. The integration with synthesis and verification backends (commercial and open-source) allows researchers to quickly test any new finding and easily obtain performance and resource usage metrics for a given application. Different FPGA devices are supported from several different vendors: AMD/Xilinx, Intel/Altera, Lattice Semiconductor, and NanoXplore. Finally, integration with the OpenRoad open-source end-to-end silicon compiler perfectly fits with the recent push towards open-source EDA.},
  publisher={{IEEE}},
  doi={10.1109/DAC18074.2021.9586110},
  pdf={https://re.public.polimi.it/retrieve/668507/dac21_bambu.pdf}
}
```

Patches and pull request could be submitted at https://github.com/ferrandi/PandA-bambu while stable releases are available for download through this 
page http://panda.dei.polimi.it/?page_id=81. Documentation and Install instructions could be found on this website: http://panda.dei.polimi.it/ .

For further information send an e-mail to panda-info@polimi.it or visit the google
group page (https://groups.google.com/forum/#!forum/panda-project-discussions-questions) 



